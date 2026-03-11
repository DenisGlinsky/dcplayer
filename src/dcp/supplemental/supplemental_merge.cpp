#include "supplemental_merge.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace dcplayer::dcp::supplemental {
namespace {

using ov_vf_resolver::CompositionKind;
using ov_vf_resolver::DependencyKind;
using ov_vf_resolver::Reel;
using ov_vf_resolver::TrackFile;

struct Candidate {
    const Package* package{nullptr};
    const pkl::AssetEntry* pkl_asset{nullptr};
    const assetmap::AssetEntry* asset_map_asset{nullptr};
};

enum class ResolutionState {
    absent,
    resolved,
    broken,
};

struct PackageResolution {
    ResolutionState state{ResolutionState::absent};
    Candidate candidate;
};

struct OwnerCandidate {
    const Package* package{nullptr};
    const cpl::CompositionPlaylist* composition{nullptr};
};

struct OverrideDecision {
    TrackFile track;
    std::string policy_id;
};

using OverrideKey = std::pair<std::size_t, cpl::TrackType>;

[[nodiscard]] bool is_error(const Diagnostic& diagnostic) noexcept {
    return diagnostic.severity == DiagnosticSeverity::error;
}

[[nodiscard]] ValidationStatus validation_status_for(const std::vector<Diagnostic>& diagnostics) noexcept {
    if (std::any_of(diagnostics.begin(), diagnostics.end(), is_error)) {
        return ValidationStatus::error;
    }
    if (!diagnostics.empty()) {
        return ValidationStatus::warning;
    }
    return ValidationStatus::ok;
}

void sort_diagnostics(std::vector<Diagnostic>& diagnostics) {
    std::sort(diagnostics.begin(), diagnostics.end(), [](const Diagnostic& lhs, const Diagnostic& rhs) {
        const auto severity_rank = [](DiagnosticSeverity severity) noexcept {
            return severity == DiagnosticSeverity::error ? 0 : 1;
        };

        return std::tuple{severity_rank(lhs.severity), lhs.code, lhs.path, lhs.message} <
               std::tuple{severity_rank(rhs.severity), rhs.code, rhs.path, rhs.message};
    });
}

void add_diagnostic(std::vector<Diagnostic>& diagnostics,
                    std::string code,
                    DiagnosticSeverity severity,
                    std::string path,
                    std::string message) {
    diagnostics.push_back(Diagnostic{
        .code = std::move(code),
        .severity = severity,
        .path = std::move(path),
        .message = std::move(message),
    });
}

[[nodiscard]] const pkl::AssetEntry* find_pkl_asset(const Package& package, std::string_view asset_id) {
    for (const auto& asset : package.packing_list.assets) {
        if (asset.asset_id == asset_id) {
            return &asset;
        }
    }
    return nullptr;
}

[[nodiscard]] const assetmap::AssetEntry* find_asset_map_asset(const Package& package, std::string_view asset_id) {
    for (const auto& asset : package.asset_map.assets) {
        if (asset.asset_id == asset_id) {
            return &asset;
        }
    }
    return nullptr;
}

[[nodiscard]] const cpl::CompositionPlaylist* find_composition(const Package& package, std::string_view composition_id) {
    for (const auto& composition : package.composition_playlists) {
        if (composition.composition_id == composition_id) {
            return &composition;
        }
    }
    return nullptr;
}

[[nodiscard]] std::optional<OwnerCandidate> resolve_owner_candidate(const Package& package,
                                                                    std::string_view composition_id) {
    const auto* composition = find_composition(package, composition_id);
    if (composition == nullptr) {
        return std::nullopt;
    }

    const auto* pkl_asset = find_pkl_asset(package, composition_id);
    if (pkl_asset == nullptr || pkl_asset->asset_type != pkl::AssetType::composition_playlist) {
        return std::nullopt;
    }

    const auto* asset_map_asset = find_asset_map_asset(package, composition_id);
    if (asset_map_asset == nullptr || asset_map_asset->chunk_list.empty()) {
        return std::nullopt;
    }

    return OwnerCandidate{
        .package = &package,
        .composition = composition,
    };
}

[[nodiscard]] PackageResolution resolve_in_package(const Package& package, std::string_view asset_id) {
    const auto* pkl_asset = find_pkl_asset(package, asset_id);
    if (pkl_asset == nullptr) {
        return {};
    }

    if (pkl_asset->asset_type != pkl::AssetType::track_file) {
        return PackageResolution{
            .state = ResolutionState::broken,
        };
    }

    const auto* asset_map_asset = find_asset_map_asset(package, asset_id);
    if (asset_map_asset == nullptr || asset_map_asset->chunk_list.empty()) {
        return PackageResolution{
            .state = ResolutionState::broken,
        };
    }

    return PackageResolution{
        .state = ResolutionState::resolved,
        .candidate =
            Candidate{
                .package = &package,
                .pkl_asset = pkl_asset,
                .asset_map_asset = asset_map_asset,
            },
    };
}

[[nodiscard]] std::string track_element_name(cpl::TrackType track_type) {
    switch (track_type) {
        case cpl::TrackType::picture:
            return "MainPicture";
        case cpl::TrackType::sound:
            return "MainSound";
        case cpl::TrackType::subtitle:
            return "MainSubtitle";
    }
    return "MainPicture";
}

[[nodiscard]] std::string track_reference_path(std::size_t reel_index, cpl::TrackType track_type) {
    return "/CompositionPlaylist/ReelList/Reel[" + std::to_string(reel_index + 1U) + "]/AssetList/" +
           track_element_name(track_type) + "/Id";
}

[[nodiscard]] std::string policy_field_path(std::size_t policy_index, std::string_view field_name) {
    return "/SupplementalMergePolicyList/Policy[" + std::to_string(policy_index + 1U) + "]/" + std::string(field_name);
}

[[nodiscard]] std::string allowed_override_path(std::size_t policy_index, std::size_t override_index) {
    return "/SupplementalMergePolicyList/Policy[" + std::to_string(policy_index + 1U) + "]/AllowedOverrides[" +
           std::to_string(override_index + 1U) + "]";
}

[[nodiscard]] const Reel* find_reel(const CompositionGraph& graph, std::string_view reel_id) {
    for (const auto& reel : graph.resolved_reels) {
        if (reel.reel_id == reel_id) {
            return &reel;
        }
    }
    return nullptr;
}

[[nodiscard]] TrackFile* select_track(Reel& reel, cpl::TrackType track_type) {
    switch (track_type) {
        case cpl::TrackType::picture:
            return reel.picture_track.has_value() ? &*reel.picture_track : nullptr;
        case cpl::TrackType::sound:
            return reel.sound_track.has_value() ? &*reel.sound_track : nullptr;
        case cpl::TrackType::subtitle:
            return reel.subtitle_track.has_value() ? &*reel.subtitle_track : nullptr;
    }
    return nullptr;
}

[[nodiscard]] const TrackFile* select_track(const Reel& reel, cpl::TrackType track_type) {
    switch (track_type) {
        case cpl::TrackType::picture:
            return reel.picture_track.has_value() ? &*reel.picture_track : nullptr;
        case cpl::TrackType::sound:
            return reel.sound_track.has_value() ? &*reel.sound_track : nullptr;
        case cpl::TrackType::subtitle:
            return reel.subtitle_track.has_value() ? &*reel.subtitle_track : nullptr;
    }
    return nullptr;
}

[[nodiscard]] bool is_supported_merge_mode(MergeMode merge_mode) noexcept {
    switch (merge_mode) {
        case MergeMode::replace_lane:
            return true;
    }
    return false;
}

[[nodiscard]] bool is_valid_track_type(cpl::TrackType track_type) noexcept {
    switch (track_type) {
        case cpl::TrackType::picture:
        case cpl::TrackType::sound:
        case cpl::TrackType::subtitle:
            return true;
    }
    return false;
}

[[nodiscard]] bool override_allowed(const MergePolicy& policy, cpl::TrackType track_type) {
    return std::find(policy.allowed_overrides.begin(), policy.allowed_overrides.end(), track_type) !=
           policy.allowed_overrides.end();
}

[[nodiscard]] bool validate_policy(const MergePolicy& policy,
                                   std::size_t policy_index,
                                   std::vector<Diagnostic>& diagnostics) {
    bool valid = true;

    if (!is_supported_merge_mode(policy.merge_mode)) {
        add_diagnostic(diagnostics,
                       "supplemental.unsupported_merge_mode",
                       DiagnosticSeverity::error,
                       policy_field_path(policy_index, "MergeMode"),
                       "MergePolicy.merge_mode is not supported by this implementation");
        valid = false;
    }

    std::set<int> seen_overrides;
    for (std::size_t override_index = 0U; override_index < policy.allowed_overrides.size(); ++override_index) {
        const auto track_type = policy.allowed_overrides[override_index];
        const auto path = allowed_override_path(policy_index, override_index);
        if (!is_valid_track_type(track_type)) {
            add_diagnostic(diagnostics,
                           "supplemental.invalid_allowed_override",
                           DiagnosticSeverity::error,
                           path,
                           "MergePolicy.allowed_overrides contains an invalid lane value");
            valid = false;
            continue;
        }

        const int stable_value = static_cast<int>(track_type);
        if (!seen_overrides.insert(stable_value).second) {
            add_diagnostic(diagnostics,
                           "supplemental.duplicate_allowed_override",
                           DiagnosticSeverity::error,
                           path,
                           "MergePolicy.allowed_overrides contains a duplicate lane value");
            valid = false;
        }
    }

    return valid;
}

[[nodiscard]] TrackFile build_track_file(const cpl::AssetReference& reference,
                                         const Candidate& candidate,
                                         std::string_view origin_package_id) {
    return TrackFile{
        .asset_id = reference.asset_id,
        .track_type = reference.track_type,
        .edit_rate = reference.edit_rate,
        .resolved_path = candidate.asset_map_asset->chunk_list.front().path,
        .source_package_id = candidate.package->packing_list.pkl_id,
        .dependency_kind = candidate.package->packing_list.pkl_id == origin_package_id ? DependencyKind::local
                                                                                       : DependencyKind::external,
    };
}

void record_override(const OverrideKey& key,
                     TrackFile track,
                     const MergePolicy& policy,
                     const std::string& path,
                     std::vector<Diagnostic>& diagnostics,
                     std::map<OverrideKey, OverrideDecision>& override_decisions) {
    const auto existing = override_decisions.find(key);
    if (existing == override_decisions.end()) {
        override_decisions.emplace(key,
                                   OverrideDecision{
                                       .track = std::move(track),
                                       .policy_id = policy.policy_id,
                                   });
        return;
    }

    if (existing->second.track == track) {
        return;
    }

    add_diagnostic(diagnostics,
                   "supplemental.conflicting_override",
                   DiagnosticSeverity::error,
                   path,
                   "Supplemental lane override conflicts with a different resolved track");
}

void process_reference(const CompositionGraph& base_graph,
                       const Reel& base_reel,
                       const cpl::AssetReference& reference,
                       const Package& owner_package,
                       const MergePolicy& policy,
                       std::size_t reel_index,
                       std::size_t base_reel_index,
                       std::vector<Diagnostic>& diagnostics,
                       std::map<OverrideKey, OverrideDecision>& override_decisions) {
    const std::string path = track_reference_path(reel_index, reference.track_type);
    const auto* base_track = select_track(base_reel, reference.track_type);
    if (base_track == nullptr) {
        add_diagnostic(diagnostics,
                       "supplemental.broken_base_dependency",
                       DiagnosticSeverity::error,
                       path,
                       "Supplemental reference does not match any base CompositionGraph track");
        return;
    }

    const auto local_resolution = resolve_in_package(owner_package, reference.asset_id);
    if (local_resolution.state == ResolutionState::resolved) {
        if (!override_allowed(policy, reference.track_type)) {
            add_diagnostic(diagnostics,
                           "supplemental.override_not_allowed",
                           DiagnosticSeverity::error,
                           path,
                           "Supplemental lane override is not allowed by policy");
            return;
        }

        record_override(OverrideKey{base_reel_index, reference.track_type},
                        build_track_file(reference, local_resolution.candidate, base_graph.origin_package_id),
                        policy,
                        path,
                        diagnostics,
                        override_decisions);
        return;
    }

    if (local_resolution.state == ResolutionState::broken) {
        add_diagnostic(diagnostics,
                       "supplemental.asset_without_valid_backing",
                       DiagnosticSeverity::error,
                       path,
                       "Supplemental asset reference does not have a valid backing PKL/AssetMap entry");
        return;
    }

    if (base_track->asset_id != reference.asset_id) {
        add_diagnostic(diagnostics,
                       "supplemental.broken_base_dependency",
                       DiagnosticSeverity::error,
                       path,
                       "Supplemental reference does not match any base CompositionGraph track");
        return;
    }

    if (base_track->edit_rate != reference.edit_rate) {
        add_diagnostic(diagnostics,
                       "supplemental.base_edit_rate_mismatch",
                       DiagnosticSeverity::error,
                       path,
                       "Supplemental explicit base dependency does not match the base track edit_rate");
    }
}

void process_optional_reference(const CompositionGraph& base_graph,
                                const Reel& base_reel,
                                const std::optional<cpl::AssetReference>& reference,
                                const Package& owner_package,
                                const MergePolicy& policy,
                                std::size_t reel_index,
                                std::size_t base_reel_index,
                                std::vector<Diagnostic>& diagnostics,
                                std::map<OverrideKey, OverrideDecision>& override_decisions) {
    if (!reference.has_value()) {
        return;
    }

    process_reference(base_graph,
                      base_reel,
                      *reference,
                      owner_package,
                      policy,
                      reel_index,
                      base_reel_index,
                      diagnostics,
                      override_decisions);
}

void recompute_graph_metadata(CompositionGraph& graph) {
    std::set<std::string> source_packages;
    source_packages.insert(graph.origin_package_id);

    bool uses_external_dependency = false;
    for (auto& reel : graph.resolved_reels) {
        for (TrackFile* track : std::array{
                 select_track(reel, cpl::TrackType::picture),
                 select_track(reel, cpl::TrackType::sound),
                 select_track(reel, cpl::TrackType::subtitle),
             }) {
            if (track == nullptr) {
                continue;
            }

            track->dependency_kind =
                track->source_package_id == graph.origin_package_id ? DependencyKind::local : DependencyKind::external;
            uses_external_dependency = uses_external_dependency || track->dependency_kind == DependencyKind::external;
            source_packages.insert(track->source_package_id);
        }
    }

    graph.composition_kind = uses_external_dependency ? CompositionKind::vf : CompositionKind::ov;
    graph.source_packages.assign(source_packages.begin(), source_packages.end());
}

}  // namespace

ApplyResult apply(const ApplyRequest& request) {
    ApplyResult result;

    std::vector<MergePolicy> policies = request.policies;
    std::sort(policies.begin(), policies.end(), [](const MergePolicy& lhs, const MergePolicy& rhs) {
        return std::tuple{lhs.base_composition_id, lhs.supplemental_composition_id, lhs.policy_id} <
               std::tuple{rhs.base_composition_id, rhs.supplemental_composition_id, rhs.policy_id};
    });

    std::map<OverrideKey, OverrideDecision> override_decisions;

    for (std::size_t policy_index = 0U; policy_index < policies.size(); ++policy_index) {
        const auto& policy = policies[policy_index];
        if (!validate_policy(policy, policy_index, result.diagnostics)) {
            continue;
        }

        if (policy.base_composition_id != request.base_graph.composition_id) {
            add_diagnostic(result.diagnostics,
                           "supplemental.base_composition_mismatch",
                           DiagnosticSeverity::error,
                           policy_field_path(policy_index, "BaseCompositionId"),
                           "Policy base_composition_id does not match input CompositionGraph");
            continue;
        }

        std::vector<OwnerCandidate> owner_candidates;
        for (const auto& package : request.packages) {
            const auto owner_candidate = resolve_owner_candidate(package, policy.supplemental_composition_id);
            if (owner_candidate.has_value()) {
                owner_candidates.push_back(*owner_candidate);
            }
        }

        const auto supplemental_path = policy_field_path(policy_index, "SupplementalCompositionId");
        if (owner_candidates.empty()) {
            add_diagnostic(result.diagnostics,
                           "supplemental.target_composition_not_found",
                           DiagnosticSeverity::error,
                           supplemental_path,
                           "Supplemental composition_id was not found in resolver input");
            continue;
        }

        std::sort(owner_candidates.begin(),
                  owner_candidates.end(),
                  [](const OwnerCandidate& lhs, const OwnerCandidate& rhs) {
                      return lhs.package->packing_list.pkl_id < rhs.package->packing_list.pkl_id;
                  });

        if (owner_candidates.size() > 1U) {
            add_diagnostic(result.diagnostics,
                           "supplemental.conflicting_composition_owner",
                           DiagnosticSeverity::error,
                           supplemental_path,
                           "Supplemental composition_id belongs to multiple packages");
            continue;
        }

        const auto& owner = owner_candidates.front();
        const auto& composition = *owner.composition;
        const auto& owner_package = *owner.package;

        for (std::size_t reel_index = 0U; reel_index < composition.reels.size(); ++reel_index) {
            const auto& supplemental_reel = composition.reels[reel_index];
            const Reel* base_reel = find_reel(request.base_graph, supplemental_reel.reel_id);
            if (base_reel == nullptr) {
                add_diagnostic(result.diagnostics,
                               "supplemental.broken_base_dependency",
                               DiagnosticSeverity::error,
                               "/CompositionPlaylist/ReelList/Reel[" + std::to_string(reel_index + 1U) + "]/Id",
                               "Supplemental reel does not exist in the base CompositionGraph");
                continue;
            }

            const auto base_reel_index = static_cast<std::size_t>(
                std::distance(request.base_graph.resolved_reels.begin(),
                              std::find_if(request.base_graph.resolved_reels.begin(),
                                           request.base_graph.resolved_reels.end(),
                                           [&](const Reel& candidate) {
                                               return candidate.reel_id == supplemental_reel.reel_id;
                                           })));

            process_optional_reference(request.base_graph,
                                       *base_reel,
                                       supplemental_reel.picture,
                                       owner_package,
                                       policy,
                                       reel_index,
                                       base_reel_index,
                                       result.diagnostics,
                                       override_decisions);
            process_optional_reference(request.base_graph,
                                       *base_reel,
                                       supplemental_reel.sound,
                                       owner_package,
                                       policy,
                                       reel_index,
                                       base_reel_index,
                                       result.diagnostics,
                                       override_decisions);
            process_optional_reference(request.base_graph,
                                       *base_reel,
                                       supplemental_reel.subtitle,
                                       owner_package,
                                       policy,
                                       reel_index,
                                       base_reel_index,
                                       result.diagnostics,
                                       override_decisions);
        }
    }

    sort_diagnostics(result.diagnostics);
    result.validation_status = validation_status_for(result.diagnostics);
    if (result.validation_status == ValidationStatus::error) {
        return result;
    }

    CompositionGraph merged_graph = request.base_graph;
    for (const auto& [key, decision] : override_decisions) {
        auto& reel = merged_graph.resolved_reels[key.first];
        TrackFile* target_track = select_track(reel, key.second);
        if (target_track != nullptr) {
            *target_track = decision.track;
        }
    }

    recompute_graph_metadata(merged_graph);
    result.composition_graph = std::move(merged_graph);
    return result;
}

const char* to_string(MergeMode merge_mode) noexcept {
    switch (merge_mode) {
        case MergeMode::replace_lane:
            return "replace_lane";
    }
    return "replace_lane";
}

}  // namespace dcplayer::dcp::supplemental
