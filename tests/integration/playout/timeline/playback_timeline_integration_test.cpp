#include "asset_map.hpp"
#include "composition_playlist.hpp"
#include "ov_vf_resolver.hpp"
#include "packing_list.hpp"
#include "playback_timeline.hpp"
#include "supplemental_merge.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

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

dcplayer::dcp::ov_vf_resolver::CompositionGraph resolve_ov_single_graph(const std::filesystem::path& fixture_root) {
    const auto result = dcplayer::dcp::ov_vf_resolver::resolve(ResolveRequest{
        .composition_id = "30000000-0000-0000-0000-000000000101",
        .packages = {
            load_package(fixture_root / "valid" / "ov_single" / "ov_pkg"),
        },
    });

    expect(result.ok(), "OV fixture should resolve");
    expect(result.composition_graph.has_value(), "Expected resolved OV graph");
    return *result.composition_graph;
}

dcplayer::dcp::ov_vf_resolver::CompositionGraph resolve_supplemental_graph(const std::filesystem::path& ov_fixture_root,
                                                                            const std::filesystem::path& supplemental_fixture_root) {
    const auto base_graph = resolve_ov_single_graph(ov_fixture_root);

    const auto result = dcplayer::dcp::supplemental::apply(ApplyRequest{
        .base_graph = base_graph,
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

    expect(result.ok(), "Supplemental override fixture should resolve");
    expect(result.validation_status == ValidationStatus::ok, "Expected ok supplemental validation status");
    expect(result.composition_graph.has_value(), "Expected merged graph");
    return *result.composition_graph;
}

void expect_fixture_json(const dcplayer::playout::timeline::PlaybackTimeline& playback_timeline,
                         const std::filesystem::path& fixture_path) {
    expect(dcplayer::playout::timeline::to_json(playback_timeline) == read_file(fixture_path),
           "Unexpected timeline JSON at " + fixture_path.string());
}

}  // namespace

int main() {
    const std::filesystem::path ov_fixture_root{OV_VF_FIXTURE_DIR};
    const std::filesystem::path supplemental_fixture_root{SUPPLEMENTAL_FIXTURE_DIR};
    const std::filesystem::path timeline_fixture_root{TIMELINE_FIXTURE_DIR};

    {
        const auto build_result = dcplayer::playout::timeline::build(resolve_ov_single_graph(ov_fixture_root));

        expect(build_result.ok(), "OV graph must build a timeline");
        expect(build_result.validation_status == ValidationStatus::ok, "Expected ok timeline validation status");
        expect(build_result.playback_timeline.has_value(), "Expected normalized timeline for OV graph");
        expect(build_result.playback_timeline->lane_summary.picture == 1U, "Unexpected OV picture summary");
        expect(build_result.playback_timeline->lane_summary.sound == 1U, "Unexpected OV sound summary");
        expect_fixture_json(*build_result.playback_timeline, timeline_fixture_root / "valid" / "ov_single.timeline.json");
    }

    {
        const auto build_result =
            dcplayer::playout::timeline::build(resolve_supplemental_graph(ov_fixture_root, supplemental_fixture_root));

        expect(build_result.ok(), "Supplemental graph must build a timeline");
        expect(build_result.playback_timeline.has_value(), "Expected normalized supplemental timeline");
        expect(build_result.playback_timeline->composition_kind == dcplayer::dcp::ov_vf_resolver::CompositionKind::vf,
               "Supplemental override must keep vf composition kind");
        expect(build_result.playback_timeline->reel_entries[0].picture->source_package_id ==
                   "20000000-0000-0000-0000-000000000301",
               "Expected supplemental picture provenance");
        expect_fixture_json(*build_result.playback_timeline,
                            timeline_fixture_root / "valid" / "supplemental_override_picture.timeline.json");
    }

    return 0;
}
