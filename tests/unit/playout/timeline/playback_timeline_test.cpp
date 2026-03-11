#include "composition_playlist.hpp"
#include "ov_vf_resolver.hpp"
#include "playback_timeline.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using dcplayer::dcp::cpl::Rational;
using dcplayer::dcp::cpl::TrackType;
using dcplayer::dcp::ov_vf_resolver::CompositionGraph;
using dcplayer::dcp::ov_vf_resolver::CompositionKind;
using dcplayer::dcp::ov_vf_resolver::DependencyKind;
using dcplayer::dcp::ov_vf_resolver::Reel;
using dcplayer::dcp::ov_vf_resolver::TrackFile;
using dcplayer::dcp::ov_vf_resolver::ValidationStatus;
using dcplayer::playout::timeline::BuildResult;

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

TrackFile make_track(std::string asset_id,
                     TrackType track_type,
                     Rational edit_rate,
                     std::string resolved_path,
                     std::string source_package_id,
                     DependencyKind dependency_kind) {
    return TrackFile{
        .asset_id = std::move(asset_id),
        .track_type = track_type,
        .edit_rate = edit_rate,
        .resolved_path = std::move(resolved_path),
        .source_package_id = std::move(source_package_id),
        .dependency_kind = dependency_kind,
    };
}

CompositionGraph make_valid_graph() {
    return CompositionGraph{
        .composition_id = "30000000-0000-0000-0000-000000000901",
        .content_title_text = "Timeline_Test_901",
        .origin_package_id = "20000000-0000-0000-0000-000000000901",
        .composition_kind = CompositionKind::vf,
        .source_packages =
            {
                "20000000-0000-0000-0000-000000000901",
                "20000000-0000-0000-0000-000000000902",
            },
        .resolved_reels =
            {
                Reel{
                    .reel_id = "31000000-0000-0000-0000-000000000901",
                    .picture_track =
                        make_track("32000000-0000-0000-0000-000000000901",
                                   TrackType::picture,
                                   Rational{24U, 1U},
                                   "MXF/reel1_picture.mxf",
                                   "20000000-0000-0000-0000-000000000901",
                                   DependencyKind::local),
                    .sound_track =
                        make_track("33000000-0000-0000-0000-000000000901",
                                   TrackType::sound,
                                   Rational{24U, 1U},
                                   "MXF/reel1_sound.mxf",
                                   "20000000-0000-0000-0000-000000000901",
                                   DependencyKind::local),
                },
                Reel{
                    .reel_id = "31000000-0000-0000-0000-000000000902",
                    .picture_track =
                        make_track("32000000-0000-0000-0000-000000000902",
                                   TrackType::picture,
                                   Rational{24U, 1U},
                                   "MXF/reel2_picture.mxf",
                                   "20000000-0000-0000-0000-000000000902",
                                   DependencyKind::external),
                    .subtitle_track =
                        make_track("34000000-0000-0000-0000-000000000902",
                                   TrackType::subtitle,
                                   Rational{24U, 1U},
                                   "XML/reel2_subtitle.xml",
                                   "20000000-0000-0000-0000-000000000901",
                                   DependencyKind::local),
                },
            },
    };
}

CompositionGraph make_single_lane_graph() {
    return CompositionGraph{
        .composition_id = "30000000-0000-0000-0000-000000000903",
        .content_title_text = "Timeline_Test_903",
        .origin_package_id = "20000000-0000-0000-0000-000000000903",
        .composition_kind = CompositionKind::ov,
        .source_packages =
            {
                "20000000-0000-0000-0000-000000000903",
            },
        .resolved_reels =
            {
                Reel{
                    .reel_id = "31000000-0000-0000-0000-000000000903",
                    .picture_track =
                        make_track("32000000-0000-0000-0000-000000000903",
                                   TrackType::picture,
                                   Rational{24U, 1U},
                                   "MXF/reel3_picture.mxf",
                                   "20000000-0000-0000-0000-000000000903",
                                   DependencyKind::local),
                },
            },
    };
}

CompositionGraph make_single_lane_vf_graph() {
    auto graph = make_single_lane_graph();
    graph.composition_kind = CompositionKind::vf;
    return graph;
}

void expect_single_diagnostic(const BuildResult& result, const std::string& code, const std::string& path) {
    expect(!result.ok(), "Expected timeline build failure");
    expect(result.validation_status == ValidationStatus::error, "Expected error validation status");
    if (result.diagnostics.size() != 1U) {
        std::string detail = "Expected exactly one diagnostic, got " + std::to_string(result.diagnostics.size());
        for (const auto& diagnostic : result.diagnostics) {
            detail += " [" + diagnostic.code + " @ " + diagnostic.path + "]";
        }
        throw std::runtime_error(detail);
    }
    expect(result.diagnostics[0].code == code, "Unexpected diagnostic code");
    expect(result.diagnostics[0].path == path, "Unexpected diagnostic path");
}

}  // namespace

int main() {
    const std::filesystem::path fixture_root{TIMELINE_FIXTURE_DIR};

    {
        const auto result = dcplayer::playout::timeline::build(make_valid_graph());

        expect(result.ok(), "Valid graph must build a timeline");
        expect(result.validation_status == ValidationStatus::ok, "Expected ok validation status");
        expect(result.playback_timeline.has_value(), "Expected normalized playback timeline");
        expect(result.playback_timeline->reel_entries.size() == 2U, "Expected two reel entries");
        expect(result.playback_timeline->reel_entries[0].entry_index == 0U, "Unexpected first entry index");
        expect(result.playback_timeline->reel_entries[0].picture.has_value(), "Expected picture lane on first reel");
        expect(result.playback_timeline->reel_entries[0].sound.has_value(), "Expected sound lane on first reel");
        expect(!result.playback_timeline->reel_entries[0].subtitle.has_value(), "Did not expect subtitle on first reel");
        expect(result.playback_timeline->reel_entries[1].picture->dependency_kind == DependencyKind::external,
               "Expected external picture on second reel");
        expect(result.playback_timeline->lane_summary.picture == 2U, "Unexpected picture lane summary");
        expect(result.playback_timeline->lane_summary.sound == 1U, "Unexpected sound lane summary");
        expect(result.playback_timeline->lane_summary.subtitle == 1U, "Unexpected subtitle lane summary");
    }

    {
        auto graph = make_valid_graph();
        graph.missing_assets = {"42000000-0000-0000-0000-000000000901"};

        const auto result = dcplayer::playout::timeline::build(graph);
        expect_single_diagnostic(result, "timeline.graph_not_show_ready", "/missing_assets");
        expect(dcplayer::playout::timeline::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "graph_not_show_ready.diagnostics.json"),
               "Unexpected graph_not_show_ready diagnostics JSON");
    }

    {
        auto graph = make_valid_graph();
        graph.resolved_reels.clear();

        const auto result = dcplayer::playout::timeline::build(graph);
        expect_single_diagnostic(result, "timeline.empty_reel_list", "/resolved_reels");
        expect(dcplayer::playout::timeline::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "empty_reel_list.diagnostics.json"),
               "Unexpected empty_reel_list diagnostics JSON");
    }

    {
        auto graph = make_valid_graph();
        graph.resolved_reels[0].picture_track->track_type = TrackType::sound;

        const auto result = dcplayer::playout::timeline::build(graph);
        expect_single_diagnostic(result, "timeline.lane_type_mismatch", "/resolved_reels[1]/picture_track");
        expect(dcplayer::playout::timeline::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "lane_type_mismatch.diagnostics.json"),
               "Unexpected lane_type_mismatch diagnostics JSON");
    }

    {
        auto graph = make_valid_graph();
        graph.resolved_reels[0].sound_track->edit_rate = Rational{48U, 1U};

        const auto result = dcplayer::playout::timeline::build(graph);
        expect_single_diagnostic(result, "timeline.entry_edit_rate_mismatch", "/resolved_reels[1]");
        expect(dcplayer::playout::timeline::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "edit_rate_mismatch.diagnostics.json"),
               "Unexpected entry_edit_rate_mismatch diagnostics JSON");
    }

    {
        auto graph = make_valid_graph();
        graph.resolved_reels[1].subtitle_track->source_package_id = "20000000-0000-0000-0000-000000000999";
        graph.resolved_reels[1].subtitle_track->dependency_kind = DependencyKind::external;

        const auto result = dcplayer::playout::timeline::build(graph);
        expect_single_diagnostic(result,
                                 "timeline.unsupported_graph_shape",
                                 "/resolved_reels[2]/subtitle_track/source_package_id");
        expect(dcplayer::playout::timeline::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "unsupported_graph_shape.diagnostics.json"),
               "Unexpected unsupported_graph_shape diagnostics JSON");
    }

    {
        auto graph = make_valid_graph();
        graph.composition_kind = CompositionKind::ov;

        const auto result = dcplayer::playout::timeline::build(graph);
        expect_single_diagnostic(result,
                                 "timeline.composition_kind_dependency_mismatch",
                                 "/resolved_reels[2]/picture_track/dependency_kind");
        expect(dcplayer::playout::timeline::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "composition_kind_dependency_mismatch.diagnostics.json"),
               "Unexpected composition_kind_dependency_mismatch diagnostics JSON");
    }

    {
        auto graph = make_single_lane_vf_graph();
        const auto result = dcplayer::playout::timeline::build(graph);

        expect_single_diagnostic(result, "timeline.composition_kind_dependency_mismatch", "/composition_kind");
        expect(dcplayer::playout::timeline::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "vf_composition_kind_dependency_mismatch.diagnostics.json"),
               "Unexpected vf_composition_kind_dependency_mismatch diagnostics JSON");
    }

    {
        auto graph = make_single_lane_graph();
        graph.resolved_reels[0].picture_track->edit_rate = Rational{0U, 1U};

        const auto result = dcplayer::playout::timeline::build(graph);
        expect_single_diagnostic(result, "timeline.invalid_edit_rate", "/resolved_reels[1]/picture_track/edit_rate");
        expect(dcplayer::playout::timeline::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "single_lane_invalid_edit_rate.diagnostics.json"),
               "Unexpected single_lane_invalid_edit_rate diagnostics JSON");
    }

    {
        auto graph = make_single_lane_vf_graph();
        graph.resolved_reels[0].picture_track->edit_rate = Rational{0U, 1U};

        const auto result = dcplayer::playout::timeline::build(graph);
        expect_single_diagnostic(result, "timeline.invalid_edit_rate", "/resolved_reels[1]/picture_track/edit_rate");
        expect(dcplayer::playout::timeline::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "single_lane_invalid_edit_rate.diagnostics.json"),
               "Unexpected vf single_lane_invalid_edit_rate diagnostics JSON");
    }

    return 0;
}
