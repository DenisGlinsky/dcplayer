#include "playback_timeline.hpp"

#include <algorithm>
#include <array>
#include <sstream>
#include <string_view>
#include <tuple>
#include <utility>

namespace dcplayer::playout::timeline {
namespace {

struct LaneValidationState {
    bool has_lane{false};
    bool has_positive_edit_rate{false};
    bool has_invalid_edit_rate{false};
    bool has_external_lane{false};
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

[[nodiscard]] std::string reel_path(std::size_t reel_index) {
    return "/resolved_reels[" + std::to_string(reel_index + 1U) + "]";
}

[[nodiscard]] std::string lane_path(std::size_t reel_index, std::string_view lane_name) {
    return reel_path(reel_index) + "/" + std::string{lane_name};
}

[[nodiscard]] bool is_positive_edit_rate(const dcp::cpl::Rational& edit_rate) noexcept {
    return edit_rate.numerator > 0U && edit_rate.denominator > 0U;
}

[[nodiscard]] bool contains_package(const std::vector<std::string>& source_packages, std::string_view package_id) {
    return std::find(source_packages.begin(), source_packages.end(), package_id) != source_packages.end();
}

void validate_graph_header(const CompositionGraph& composition_graph, std::vector<Diagnostic>& diagnostics) {
    if (composition_graph.composition_id.empty()) {
        add_diagnostic(diagnostics,
                       "timeline.unsupported_graph_shape",
                       DiagnosticSeverity::error,
                       "/composition_id",
                       "CompositionGraph must contain a composition_id");
    }

    if (composition_graph.origin_package_id.empty()) {
        add_diagnostic(diagnostics,
                       "timeline.unsupported_graph_shape",
                       DiagnosticSeverity::error,
                       "/origin_package_id",
                       "CompositionGraph must contain an origin_package_id");
    }

    if (composition_graph.source_packages.empty()) {
        add_diagnostic(diagnostics,
                       "timeline.unsupported_graph_shape",
                       DiagnosticSeverity::error,
                       "/source_packages",
                       "CompositionGraph must contain at least one source package");
    } else if (!contains_package(composition_graph.source_packages, composition_graph.origin_package_id)) {
        add_diagnostic(diagnostics,
                       "timeline.unsupported_graph_shape",
                       DiagnosticSeverity::error,
                       "/source_packages",
                       "CompositionGraph origin_package_id must be present in source_packages");
    }

    if (!composition_graph.missing_assets.empty()) {
        add_diagnostic(diagnostics,
                       "timeline.graph_not_show_ready",
                       DiagnosticSeverity::error,
                       "/missing_assets",
                       "CompositionGraph contains unresolved assets and is not show-ready");
    }

    if (composition_graph.resolved_reels.empty()) {
        add_diagnostic(diagnostics,
                       "timeline.empty_reel_list",
                       DiagnosticSeverity::error,
                       "/resolved_reels",
                       "CompositionGraph must contain at least one resolved reel");
    }
}

[[nodiscard]] LaneValidationState validate_lane(const CompositionGraph& composition_graph,
                                                const TrackFile& track,
                                                dcp::cpl::TrackType expected_track_type,
                                                std::size_t reel_index,
                                                std::string_view lane_name,
                                                std::vector<Diagnostic>& diagnostics) {
    const auto path = lane_path(reel_index, lane_name);
    LaneValidationState state{
        .has_lane = true,
        .has_external_lane = track.dependency_kind == DependencyKind::external,
    };

    if (track.track_type != expected_track_type) {
        add_diagnostic(diagnostics,
                       "timeline.lane_type_mismatch",
                       DiagnosticSeverity::error,
                       path,
                       "TrackFile track_type does not match the reel lane");
    }

    if (!is_positive_edit_rate(track.edit_rate)) {
        add_diagnostic(diagnostics,
                       "timeline.invalid_edit_rate",
                       DiagnosticSeverity::error,
                       path + "/edit_rate",
                       "Timeline lane edit_rate must contain positive numerator and denominator");
        state.has_invalid_edit_rate = true;
    } else {
        state.has_positive_edit_rate = true;
    }

    if (track.asset_id.empty()) {
        add_diagnostic(diagnostics,
                       "timeline.unsupported_graph_shape",
                       DiagnosticSeverity::error,
                       path + "/asset_id",
                       "TrackFile asset_id must be present");
    }

    if (track.resolved_path.empty()) {
        add_diagnostic(diagnostics,
                       "timeline.unsupported_graph_shape",
                       DiagnosticSeverity::error,
                       path + "/resolved_path",
                       "TrackFile resolved_path must be present");
    }

    if (track.source_package_id.empty()) {
        add_diagnostic(diagnostics,
                       "timeline.unsupported_graph_shape",
                       DiagnosticSeverity::error,
                       path + "/source_package_id",
                       "TrackFile source_package_id must be present");
    } else if (!contains_package(composition_graph.source_packages, track.source_package_id)) {
        add_diagnostic(diagnostics,
                       "timeline.unsupported_graph_shape",
                       DiagnosticSeverity::error,
                       path + "/source_package_id",
                       "TrackFile source_package_id must be listed in CompositionGraph source_packages");
    }

    if (track.dependency_kind == DependencyKind::local && track.source_package_id != composition_graph.origin_package_id) {
        add_diagnostic(diagnostics,
                       "timeline.unsupported_graph_shape",
                       DiagnosticSeverity::error,
                       path + "/dependency_kind",
                       "Local lane must originate from the CompositionGraph origin package");
    }

    if (track.dependency_kind == DependencyKind::external && track.source_package_id == composition_graph.origin_package_id) {
        add_diagnostic(diagnostics,
                       "timeline.unsupported_graph_shape",
                       DiagnosticSeverity::error,
                       path + "/dependency_kind",
                       "External lane must originate from a non-origin package");
    }

    if (composition_graph.composition_kind == CompositionKind::ov && track.dependency_kind == DependencyKind::external) {
        add_diagnostic(diagnostics,
                       "timeline.composition_kind_dependency_mismatch",
                       DiagnosticSeverity::error,
                       path + "/dependency_kind",
                       "OV CompositionGraph cannot contain external lane dependencies");
    }

    return state;
}

[[nodiscard]] bool edit_rates_match(const std::vector<dcp::cpl::Rational>& edit_rates) {
    if (edit_rates.empty()) {
        return true;
    }

    return std::all_of(edit_rates.begin() + 1U, edit_rates.end(), [&](const dcp::cpl::Rational& candidate) {
        return candidate == edit_rates.front();
    });
}

[[nodiscard]] LaneEntry build_lane_entry(const TrackFile& track_file) {
    return LaneEntry{
        .asset_id = track_file.asset_id,
        .track_type = track_file.track_type,
        .edit_rate = track_file.edit_rate,
        .resolved_path = track_file.resolved_path,
        .source_package_id = track_file.source_package_id,
        .dependency_kind = track_file.dependency_kind,
    };
}

[[nodiscard]] const char* dependency_kind_to_string(DependencyKind dependency_kind) noexcept {
    return dcp::ov_vf_resolver::to_string(dependency_kind);
}

[[nodiscard]] const char* composition_kind_to_string(CompositionKind composition_kind) noexcept {
    return dcp::ov_vf_resolver::to_string(composition_kind);
}

[[nodiscard]] const char* diagnostic_severity_to_string(DiagnosticSeverity severity) noexcept {
    return dcp::ov_vf_resolver::to_string(severity);
}

[[nodiscard]] std::string json_escape(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (const char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }

    return escaped;
}

void append_indent(std::ostringstream& output, std::size_t level) {
    for (std::size_t index = 0U; index < level; ++index) {
        output << "  ";
    }
}

void append_rational(std::ostringstream& output, const dcp::cpl::Rational& edit_rate, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"numerator\": " << edit_rate.numerator << ",\n";
    append_indent(output, indent_level + 1U);
    output << "\"denominator\": " << edit_rate.denominator << '\n';
    append_indent(output, indent_level);
    output << "}";
}

void append_lane(std::ostringstream& output, const LaneEntry& lane, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"asset_id\": \"" << json_escape(lane.asset_id) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"track_type\": \"" << dcp::cpl::to_string(lane.track_type) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"edit_rate\": ";
    append_rational(output, lane.edit_rate, indent_level + 1U);
    output << ",\n";
    append_indent(output, indent_level + 1U);
    output << "\"resolved_path\": \"" << json_escape(lane.resolved_path) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"source_package_id\": \"" << json_escape(lane.source_package_id) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"dependency_kind\": \"" << dependency_kind_to_string(lane.dependency_kind) << "\"\n";
    append_indent(output, indent_level);
    output << "}";
}

void append_lane_or_null(std::ostringstream& output,
                         const std::optional<LaneEntry>& lane,
                         std::size_t indent_level) {
    if (!lane.has_value()) {
        output << "null";
        return;
    }

    append_lane(output, *lane, indent_level);
}

void append_string_array(std::ostringstream& output,
                         const std::vector<std::string>& values,
                         std::size_t indent_level) {
    if (values.empty()) {
        output << "[]";
        return;
    }

    output << "[\n";
    for (std::size_t index = 0U; index < values.size(); ++index) {
        append_indent(output, indent_level + 1U);
        output << "\"" << json_escape(values[index]) << "\"";
        if (index + 1U != values.size()) {
            output << ",";
        }
        output << '\n';
    }
    append_indent(output, indent_level);
    output << "]";
}

void append_timeline_entry(std::ostringstream& output, const TimelineEntry& entry, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"entry_index\": " << entry.entry_index << ",\n";
    append_indent(output, indent_level + 1U);
    output << "\"reel_id\": \"" << json_escape(entry.reel_id) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"edit_rate\": ";
    append_rational(output, entry.edit_rate, indent_level + 1U);
    output << ",\n";
    append_indent(output, indent_level + 1U);
    output << "\"picture\": ";
    append_lane_or_null(output, entry.picture, indent_level + 1U);
    output << ",\n";
    append_indent(output, indent_level + 1U);
    output << "\"sound\": ";
    append_lane_or_null(output, entry.sound, indent_level + 1U);
    output << ",\n";
    append_indent(output, indent_level + 1U);
    output << "\"subtitle\": ";
    append_lane_or_null(output, entry.subtitle, indent_level + 1U);
    output << '\n';
    append_indent(output, indent_level);
    output << "}";
}

void append_diagnostic(std::ostringstream& output, const Diagnostic& diagnostic, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"code\": \"" << json_escape(diagnostic.code) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"severity\": \"" << diagnostic_severity_to_string(diagnostic.severity) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"path\": \"" << json_escape(diagnostic.path) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"message\": \"" << json_escape(diagnostic.message) << "\"\n";
    append_indent(output, indent_level);
    output << "}";
}

}  // namespace

BuildResult build(const CompositionGraph& composition_graph) {
    std::vector<Diagnostic> diagnostics;
    validate_graph_header(composition_graph, diagnostics);

    std::array<std::size_t, 3U> lane_counts{0U, 0U, 0U};
    bool graph_has_external_lane = false;
    std::vector<TimelineEntry> entries;
    entries.reserve(composition_graph.resolved_reels.size());

    for (std::size_t reel_index = 0U; reel_index < composition_graph.resolved_reels.size(); ++reel_index) {
        const auto& reel = composition_graph.resolved_reels[reel_index];
        std::vector<dcp::cpl::Rational> edit_rates;
        LaneValidationState reel_state;

        if (reel.reel_id.empty()) {
            add_diagnostic(diagnostics,
                           "timeline.unsupported_graph_shape",
                           DiagnosticSeverity::error,
                           reel_path(reel_index) + "/reel_id",
                           "Resolved reel must contain a reel_id");
        }

        const auto collect_rate = [&](const std::optional<TrackFile>& track,
                                      dcp::cpl::TrackType expected_track_type,
                                      std::string_view lane_name) {
            if (!track.has_value()) {
                return;
            }

            const auto lane_state =
                validate_lane(composition_graph, *track, expected_track_type, reel_index, lane_name, diagnostics);
            reel_state.has_lane = reel_state.has_lane || lane_state.has_lane;
            reel_state.has_positive_edit_rate = reel_state.has_positive_edit_rate || lane_state.has_positive_edit_rate;
            reel_state.has_invalid_edit_rate = reel_state.has_invalid_edit_rate || lane_state.has_invalid_edit_rate;
            reel_state.has_external_lane = reel_state.has_external_lane || lane_state.has_external_lane;
            if (lane_state.has_positive_edit_rate) {
                edit_rates.push_back(track->edit_rate);
            }
        };

        collect_rate(reel.picture_track, dcp::cpl::TrackType::picture, "picture_track");
        collect_rate(reel.sound_track, dcp::cpl::TrackType::sound, "sound_track");
        collect_rate(reel.subtitle_track, dcp::cpl::TrackType::subtitle, "subtitle_track");

        graph_has_external_lane = graph_has_external_lane || reel_state.has_external_lane;

        if (!reel_state.has_lane) {
            add_diagnostic(diagnostics,
                           "timeline.unsupported_graph_shape",
                           DiagnosticSeverity::error,
                           reel_path(reel_index),
                           "Resolved reel must contain at least one valid supported lane");
            continue;
        }

        if (!reel_state.has_positive_edit_rate) {
            if (!reel_state.has_invalid_edit_rate) {
                add_diagnostic(diagnostics,
                               "timeline.unsupported_graph_shape",
                               DiagnosticSeverity::error,
                               reel_path(reel_index),
                               "Resolved reel must contain at least one valid supported lane");
            }
            continue;
        }

        if (!edit_rates_match(edit_rates)) {
            add_diagnostic(diagnostics,
                           "timeline.entry_edit_rate_mismatch",
                           DiagnosticSeverity::error,
                           reel_path(reel_index),
                           "All lanes within a timeline entry must share the same edit_rate");
            continue;
        }

        TimelineEntry entry{
            .entry_index = reel_index,
            .reel_id = reel.reel_id,
            .edit_rate = edit_rates.front(),
        };

        if (reel.picture_track.has_value()) {
            entry.picture = build_lane_entry(*reel.picture_track);
            ++lane_counts[0];
        }
        if (reel.sound_track.has_value()) {
            entry.sound = build_lane_entry(*reel.sound_track);
            ++lane_counts[1];
        }
        if (reel.subtitle_track.has_value()) {
            entry.subtitle = build_lane_entry(*reel.subtitle_track);
            ++lane_counts[2];
        }

        entries.push_back(std::move(entry));
    }

    if (composition_graph.composition_kind == CompositionKind::vf && diagnostics.empty() &&
        !composition_graph.resolved_reels.empty() && !graph_has_external_lane) {
        add_diagnostic(diagnostics,
                       "timeline.composition_kind_dependency_mismatch",
                       DiagnosticSeverity::error,
                       "/composition_kind",
                       "VF CompositionGraph must contain at least one external lane dependency");
    }

    sort_diagnostics(diagnostics);

    const auto validation_status = validation_status_for(diagnostics);
    if (validation_status == ValidationStatus::error) {
        return BuildResult{
            .playback_timeline = std::nullopt,
            .diagnostics = std::move(diagnostics),
            .validation_status = validation_status,
        };
    }

    return BuildResult{
        .playback_timeline =
            PlaybackTimeline{
                .composition_id = composition_graph.composition_id,
                .origin_package_id = composition_graph.origin_package_id,
                .composition_kind = composition_graph.composition_kind,
                .source_packages = composition_graph.source_packages,
                .reel_entries = std::move(entries),
                .lane_summary =
                    LaneSummary{
                        .picture = lane_counts[0],
                        .sound = lane_counts[1],
                        .subtitle = lane_counts[2],
                    },
            },
        .diagnostics = std::move(diagnostics),
        .validation_status = validation_status,
    };
}

std::string to_json(const PlaybackTimeline& playback_timeline) {
    std::ostringstream output;

    output << "{\n";
    append_indent(output, 1U);
    output << "\"composition_id\": \"" << json_escape(playback_timeline.composition_id) << "\",\n";
    append_indent(output, 1U);
    output << "\"origin_package_id\": \"" << json_escape(playback_timeline.origin_package_id) << "\",\n";
    append_indent(output, 1U);
    output << "\"composition_kind\": \"" << composition_kind_to_string(playback_timeline.composition_kind) << "\",\n";
    append_indent(output, 1U);
    output << "\"source_packages\": ";
    append_string_array(output, playback_timeline.source_packages, 1U);
    output << ",\n";
    append_indent(output, 1U);
    output << "\"reel_entries\": ";
    if (playback_timeline.reel_entries.empty()) {
        output << "[]";
    } else {
        output << "[\n";
        for (std::size_t index = 0U; index < playback_timeline.reel_entries.size(); ++index) {
            append_indent(output, 2U);
            append_timeline_entry(output, playback_timeline.reel_entries[index], 2U);
            if (index + 1U != playback_timeline.reel_entries.size()) {
                output << ",";
            }
            output << '\n';
        }
        append_indent(output, 1U);
        output << "]";
    }
    output << ",\n";
    append_indent(output, 1U);
    output << "\"lane_summary\": {\n";
    append_indent(output, 2U);
    output << "\"picture\": " << playback_timeline.lane_summary.picture << ",\n";
    append_indent(output, 2U);
    output << "\"sound\": " << playback_timeline.lane_summary.sound << ",\n";
    append_indent(output, 2U);
    output << "\"subtitle\": " << playback_timeline.lane_summary.subtitle << '\n';
    append_indent(output, 1U);
    output << "}\n";
    output << "}\n";

    return output.str();
}

std::string diagnostics_to_json(const std::vector<Diagnostic>& diagnostics) {
    std::ostringstream output;
    output << "[\n";
    for (std::size_t index = 0U; index < diagnostics.size(); ++index) {
        append_indent(output, 1U);
        append_diagnostic(output, diagnostics[index], 1U);
        if (index + 1U != diagnostics.size()) {
            output << ",";
        }
        output << '\n';
    }
    output << "]\n";
    return output.str();
}

}  // namespace dcplayer::playout::timeline
