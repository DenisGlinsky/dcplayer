#include "ov_vf_resolver.hpp"

#include <algorithm>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace dcplayer::dcp::ov_vf_resolver {
namespace {

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
    const pkl::AssetEntry* pkl_asset{nullptr};
    const assetmap::AssetEntry* asset_map_asset{nullptr};
};

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
        .pkl_asset = pkl_asset,
        .asset_map_asset = asset_map_asset,
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

[[nodiscard]] TrackFile build_track_file(const cpl::AssetReference& reference,
                                         const Candidate& candidate,
                                         std::string origin_package_id) {
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

void assign_track(Reel& reel, TrackFile track) {
    switch (track.track_type) {
        case cpl::TrackType::picture:
            reel.picture_track = std::move(track);
            break;
        case cpl::TrackType::sound:
            reel.sound_track = std::move(track);
            break;
        case cpl::TrackType::subtitle:
            reel.subtitle_track = std::move(track);
            break;
    }
}

void resolve_reference(const ResolveRequest& request,
                       const Package& origin_package,
                       const cpl::AssetReference& reference,
                       std::size_t reel_index,
                       std::set<std::string>& source_packages,
                       std::set<std::string>& missing_assets,
                       std::vector<Diagnostic>& diagnostics,
                       Reel& output_reel,
                       bool& uses_external_dependency,
                       bool& has_broken_dependency) {
    const std::string path = track_reference_path(reel_index, reference.track_type);

    const auto local_resolution = resolve_in_package(origin_package, reference.asset_id);
    if (local_resolution.state == ResolutionState::resolved) {
        assign_track(output_reel, build_track_file(reference, local_resolution.candidate, origin_package.packing_list.pkl_id));
        return;
    }

    if (local_resolution.state == ResolutionState::broken) {
        add_diagnostic(diagnostics,
                       "ov_vf.broken_reference",
                       DiagnosticSeverity::error,
                       path,
                       "Asset reference does not have a valid backing PKL/AssetMap entry");
        return;
    }

    uses_external_dependency = true;

    std::vector<Candidate> external_candidates;
    bool external_broken = false;
    for (const auto& package : request.packages) {
        if (package.packing_list.pkl_id == origin_package.packing_list.pkl_id) {
            continue;
        }

        const auto resolution = resolve_in_package(package, reference.asset_id);
        if (resolution.state == ResolutionState::resolved) {
            external_candidates.push_back(resolution.candidate);
        } else if (resolution.state == ResolutionState::broken) {
            external_broken = true;
        }
    }

    if (external_candidates.empty()) {
        if (external_broken) {
            add_diagnostic(diagnostics,
                           "ov_vf.broken_reference",
                           DiagnosticSeverity::error,
                           path,
                           "Asset reference does not have a valid backing PKL/AssetMap entry");
        } else {
            add_diagnostic(diagnostics,
                           "ov_vf.missing_asset_id",
                           DiagnosticSeverity::error,
                           path,
                           "Asset reference does not resolve to any PKL asset");
            missing_assets.insert(reference.asset_id);
        }
        has_broken_dependency = true;
        return;
    }

    if (external_candidates.size() > 1U) {
        add_diagnostic(diagnostics,
                       "ov_vf.conflicting_asset_resolution",
                       DiagnosticSeverity::error,
                       path,
                       "Asset reference resolves to multiple external packages");
        has_broken_dependency = true;
        return;
    }

    const auto& candidate = external_candidates.front();
    source_packages.insert(candidate.package->packing_list.pkl_id);
    assign_track(output_reel, build_track_file(reference, candidate, origin_package.packing_list.pkl_id));
}

void resolve_optional_reference(const ResolveRequest& request,
                                const Package& origin_package,
                                const std::optional<cpl::AssetReference>& reference,
                                std::size_t reel_index,
                                std::set<std::string>& source_packages,
                                std::set<std::string>& missing_assets,
                                std::vector<Diagnostic>& diagnostics,
                                Reel& output_reel,
                                bool& uses_external_dependency,
                                bool& has_broken_dependency) {
    if (!reference.has_value()) {
        return;
    }

    resolve_reference(request,
                      origin_package,
                      *reference,
                      reel_index,
                      source_packages,
                      missing_assets,
                      diagnostics,
                      output_reel,
                      uses_external_dependency,
                      has_broken_dependency);
}

}  // namespace

ResolveResult resolve(const ResolveRequest& request) {
    ResolveResult result;

    std::vector<OwnerCandidate> owner_candidates;
    for (const auto& package : request.packages) {
        const auto owner_candidate = resolve_owner_candidate(package, request.composition_id);
        if (owner_candidate.has_value()) {
            owner_candidates.push_back(*owner_candidate);
        }
    }

    if (owner_candidates.empty()) {
        add_diagnostic(result.diagnostics,
                       "ov_vf.target_composition_not_found",
                       DiagnosticSeverity::error,
                       "/CompositionPlaylist/Id",
                       "Target composition_id was not found in resolver input");
        sort_diagnostics(result.diagnostics);
        result.validation_status = validation_status_for(result.diagnostics);
        return result;
    }

    std::sort(owner_candidates.begin(), owner_candidates.end(), [](const OwnerCandidate& lhs, const OwnerCandidate& rhs) {
        return lhs.package->packing_list.pkl_id < rhs.package->packing_list.pkl_id;
    });

    if (owner_candidates.size() > 1U) {
        add_diagnostic(result.diagnostics,
                       "ov_vf.conflicting_composition_owner",
                       DiagnosticSeverity::error,
                       "/CompositionPlaylist/Id",
                       "Target composition_id belongs to multiple packages");
        sort_diagnostics(result.diagnostics);
        result.validation_status = validation_status_for(result.diagnostics);
        return result;
    }

    const OwnerCandidate& owner = owner_candidates.front();
    const Package& origin_package = *owner.package;
    const auto* composition = owner.composition;

    std::set<std::string> source_packages;
    std::set<std::string> missing_assets;
    source_packages.insert(origin_package.packing_list.pkl_id);

    std::vector<Reel> resolved_reels;
    resolved_reels.reserve(composition->reels.size());

    bool uses_external_dependency = false;
    bool has_broken_dependency = false;

    for (std::size_t reel_index = 0U; reel_index < composition->reels.size(); ++reel_index) {
        const auto& reel = composition->reels[reel_index];
        Reel resolved_reel{
            .reel_id = reel.reel_id,
        };

        resolve_optional_reference(request,
                                   origin_package,
                                   reel.picture,
                                   reel_index,
                                   source_packages,
                                   missing_assets,
                                   result.diagnostics,
                                   resolved_reel,
                                   uses_external_dependency,
                                   has_broken_dependency);
        resolve_optional_reference(request,
                                   origin_package,
                                   reel.sound,
                                   reel_index,
                                   source_packages,
                                   missing_assets,
                                   result.diagnostics,
                                   resolved_reel,
                                   uses_external_dependency,
                                   has_broken_dependency);
        resolve_optional_reference(request,
                                   origin_package,
                                   reel.subtitle,
                                   reel_index,
                                   source_packages,
                                   missing_assets,
                                   result.diagnostics,
                                   resolved_reel,
                                   uses_external_dependency,
                                   has_broken_dependency);

        resolved_reels.push_back(std::move(resolved_reel));
    }

    if (has_broken_dependency) {
        add_diagnostic(result.diagnostics,
                       "ov_vf.broken_dependency",
                       DiagnosticSeverity::error,
                       "/CompositionPlaylist",
                       "Composition contains an unresolved OV/VF dependency");
    }

    sort_diagnostics(result.diagnostics);
    result.validation_status = validation_status_for(result.diagnostics);

    if (result.validation_status == ValidationStatus::error) {
        return result;
    }

    CompositionGraph graph;
    graph.composition_id = composition->composition_id;
    graph.content_title_text = composition->content_title_text;
    graph.origin_package_id = origin_package.packing_list.pkl_id;
    graph.composition_kind = uses_external_dependency ? CompositionKind::vf : CompositionKind::ov;
    graph.source_packages.assign(source_packages.begin(), source_packages.end());
    graph.resolved_reels = std::move(resolved_reels);
    graph.missing_assets.assign(missing_assets.begin(), missing_assets.end());

    result.composition_graph = std::move(graph);
    return result;
}

const char* to_string(DiagnosticSeverity severity) noexcept {
    switch (severity) {
        case DiagnosticSeverity::error:
            return "error";
        case DiagnosticSeverity::warning:
            return "warning";
    }
    return "warning";
}

const char* to_string(ValidationStatus status) noexcept {
    switch (status) {
        case ValidationStatus::ok:
            return "ok";
        case ValidationStatus::warning:
            return "warning";
        case ValidationStatus::error:
            return "error";
    }
    return "error";
}

const char* to_string(CompositionKind composition_kind) noexcept {
    switch (composition_kind) {
        case CompositionKind::ov:
            return "ov";
        case CompositionKind::vf:
            return "vf";
    }
    return "ov";
}

const char* to_string(DependencyKind dependency_kind) noexcept {
    switch (dependency_kind) {
        case DependencyKind::local:
            return "local";
        case DependencyKind::external:
            return "external";
    }
    return "local";
}

}  // namespace dcplayer::dcp::ov_vf_resolver
