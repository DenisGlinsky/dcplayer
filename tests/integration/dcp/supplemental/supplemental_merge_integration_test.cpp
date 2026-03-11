#include "asset_map.hpp"
#include "composition_playlist.hpp"
#include "ov_vf_resolver.hpp"
#include "packing_list.hpp"
#include "supplemental_merge.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using dcplayer::dcp::ov_vf_resolver::CompositionKind;
using dcplayer::dcp::ov_vf_resolver::Package;
using dcplayer::dcp::ov_vf_resolver::ResolveRequest;
using dcplayer::dcp::ov_vf_resolver::ValidationStatus;
using dcplayer::dcp::supplemental::ApplyRequest;
using dcplayer::dcp::supplemental::MergeMode;
using dcplayer::dcp::supplemental::MergePolicy;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open fixture: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

Package load_package(const std::filesystem::path& package_dir) {
    const auto asset_map_result = dcplayer::dcp::assetmap::parse(read_file(package_dir / "ASSETMAP.xml"));
    expect(asset_map_result.ok(), "AssetMap fixture must parse");
    expect(asset_map_result.asset_map.has_value(), "Expected normalized AssetMap");

    const auto pkl_result = dcplayer::dcp::pkl::parse(read_file(package_dir / "PKL.xml"));
    expect(pkl_result.ok(), "PKL fixture must parse");
    expect(pkl_result.packing_list.has_value(), "Expected normalized PKL");

    std::vector<dcplayer::dcp::cpl::CompositionPlaylist> compositions;
    const auto cpl_path = package_dir / "CPL.xml";
    if (std::filesystem::exists(cpl_path)) {
        const auto cpl_result = dcplayer::dcp::cpl::parse(read_file(cpl_path));
        expect(cpl_result.ok(), "CPL fixture must parse");
        expect(cpl_result.composition_playlist.has_value(), "Expected normalized CPL");
        compositions.push_back(*cpl_result.composition_playlist);
    }

    return Package{
        .asset_map = *asset_map_result.asset_map,
        .packing_list = *pkl_result.packing_list,
        .composition_playlists = std::move(compositions),
    };
}

dcplayer::dcp::ov_vf_resolver::CompositionGraph resolve_base_graph(const std::filesystem::path& base_package_dir) {
    const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
        .composition_id = "30000000-0000-0000-0000-000000000101",
        .packages = {
            load_package(base_package_dir),
        },
    });

    expect(result.ok(), "Base OV fixture must resolve");
    expect(result.composition_graph.has_value(), "Expected base CompositionGraph");
    return *result.composition_graph;
}

}  // namespace

int main() {
    const std::filesystem::path ov_fixture_root{OV_VF_FIXTURE_DIR};
    const std::filesystem::path supplemental_fixture_root{SUPPLEMENTAL_FIXTURE_DIR};
    const auto base_package_dir = ov_fixture_root / "valid" / "ov_single" / "ov_pkg";

    {
        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = resolve_base_graph(base_package_dir),
            .packages = {
                load_package(supplemental_fixture_root / "valid" / "override_picture" / "supp_pkg"),
            },
            .policies =
                {
                    MergePolicy{
                        .policy_id = "override-picture",
                        .base_composition_id = "30000000-0000-0000-0000-000000000101",
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000301",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {dcplayer::dcp::cpl::TrackType::picture},
                    },
                },
        });

        expect(result.ok(), "Valid picture override fixture should resolve");
        expect(result.validation_status == ValidationStatus::ok, "Expected ok validation status");
        expect(result.composition_graph.has_value(), "Expected merged graph");
        expect(result.composition_graph->resolved_reels[0].picture_track->resolved_path == "MXF/supp_picture.mxf",
               "Unexpected picture override path");
        expect(result.composition_graph->composition_kind == CompositionKind::vf,
               "Supplemental external override must mark graph as vf");
    }

    {
        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = resolve_base_graph(base_package_dir),
            .packages = {
                load_package(supplemental_fixture_root / "valid" / "explicit_base_dependency" / "supp_pkg"),
            },
            .policies =
                {
                    MergePolicy{
                        .policy_id = "explicit-base-dependency",
                        .base_composition_id = "30000000-0000-0000-0000-000000000101",
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000302",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {},
                    },
                },
        });

        expect(result.ok(), "Explicit base dependency fixture should resolve");
        expect(result.composition_graph.has_value(), "Expected merged graph");
        expect(result.composition_graph->source_packages ==
                   std::vector<std::string>{
                       "20000000-0000-0000-0000-000000000101",
                   },
               "Explicit base dependency must not add supplemental source packages");
        expect(result.composition_graph->composition_kind == CompositionKind::ov,
               "No external override should keep graph as ov");
    }

    {
        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = resolve_base_graph(base_package_dir),
            .packages = {
                load_package(supplemental_fixture_root / "invalid" / "base_edit_rate_mismatch" / "supp_pkg"),
            },
            .policies =
                {
                    MergePolicy{
                        .policy_id = "base-edit-rate-mismatch",
                        .base_composition_id = "30000000-0000-0000-0000-000000000101",
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000308",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {},
                    },
                },
        });

        expect(!result.ok(), "Explicit base dependency with edit_rate mismatch must fail");
        expect(result.diagnostics.size() == 1U, "Expected one base-edit-rate diagnostic");
        expect(result.diagnostics[0].code == "supplemental.base_edit_rate_mismatch",
               "Expected base edit_rate mismatch diagnostic");
    }

    {
        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = resolve_base_graph(base_package_dir),
            .packages = {
                load_package(supplemental_fixture_root / "invalid" / "target_not_found" / "stray_pkg"),
            },
            .policies =
                {
                    MergePolicy{
                        .policy_id = "target-not-found",
                        .base_composition_id = "30000000-0000-0000-0000-000000000101",
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000303",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {dcplayer::dcp::cpl::TrackType::picture},
                    },
                },
        });

        expect(!result.ok(), "Missing backed supplemental composition must fail");
        expect(result.diagnostics.size() == 1U, "Expected one target-not-found diagnostic");
        expect(result.diagnostics[0].code == "supplemental.target_composition_not_found",
               "Expected target-not-found diagnostic");
    }

    {
        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = resolve_base_graph(base_package_dir),
            .packages = {
                load_package(supplemental_fixture_root / "valid" / "override_picture" / "supp_pkg"),
            },
            .policies =
                {
                    MergePolicy{
                        .policy_id = "unsupported-mode",
                        .base_composition_id = "30000000-0000-0000-0000-000000000101",
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000301",
                        .merge_mode = static_cast<MergeMode>(99),
                        .allowed_overrides = {dcplayer::dcp::cpl::TrackType::picture},
                    },
                },
        });

        expect(!result.ok(), "Unsupported merge_mode must fail");
        expect(result.diagnostics.size() == 1U, "Expected one unsupported-merge-mode diagnostic");
        expect(result.diagnostics[0].code == "supplemental.unsupported_merge_mode",
               "Expected unsupported merge mode diagnostic");
    }

    {
        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = resolve_base_graph(base_package_dir),
            .packages = {
                load_package(supplemental_fixture_root / "invalid" / "broken_base_dependency" / "supp_pkg"),
            },
            .policies =
                {
                    MergePolicy{
                        .policy_id = "broken-base-dependency",
                        .base_composition_id = "30000000-0000-0000-0000-000000000101",
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000304",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {dcplayer::dcp::cpl::TrackType::picture},
                    },
                },
        });

        expect(!result.ok(), "Broken base dependency fixture must fail");
        expect(result.diagnostics.size() == 1U, "Expected one broken-base diagnostic");
        expect(result.diagnostics[0].code == "supplemental.broken_base_dependency",
               "Expected broken base dependency diagnostic");
    }

    {
        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = resolve_base_graph(base_package_dir),
            .packages = {
                load_package(supplemental_fixture_root / "invalid" / "conflicting_override" / "supp_b_pkg"),
                load_package(supplemental_fixture_root / "invalid" / "conflicting_override" / "supp_a_pkg"),
            },
            .policies =
                {
                    MergePolicy{
                        .policy_id = "policy-b",
                        .base_composition_id = "30000000-0000-0000-0000-000000000101",
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000306",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {dcplayer::dcp::cpl::TrackType::picture},
                    },
                    MergePolicy{
                        .policy_id = "policy-a",
                        .base_composition_id = "30000000-0000-0000-0000-000000000101",
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000305",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {dcplayer::dcp::cpl::TrackType::picture},
                    },
                },
        });

        expect(!result.ok(), "Conflicting override fixtures must fail");
        expect(result.diagnostics.size() == 1U, "Expected one conflicting override diagnostic");
        expect(result.diagnostics[0].code == "supplemental.conflicting_override",
               "Expected conflicting override diagnostic");
    }

    {
        const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
            .base_graph = resolve_base_graph(base_package_dir),
            .packages = {
                load_package(supplemental_fixture_root / "invalid" / "asset_without_valid_backing" / "supp_pkg"),
            },
            .policies =
                {
                    MergePolicy{
                        .policy_id = "asset-without-backing",
                        .base_composition_id = "30000000-0000-0000-0000-000000000101",
                        .supplemental_composition_id = "30000000-0000-0000-0000-000000000307",
                        .merge_mode = MergeMode::replace_lane,
                        .allowed_overrides = {dcplayer::dcp::cpl::TrackType::picture},
                    },
                },
        });

        expect(!result.ok(), "Supplemental asset without valid backing fixture must fail");
        expect(result.diagnostics.size() == 1U, "Expected one broken-backing diagnostic");
        expect(result.diagnostics[0].code == "supplemental.asset_without_valid_backing",
               "Expected asset without valid backing diagnostic");
    }

    return 0;
}
