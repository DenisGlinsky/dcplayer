#pragma once

#include "composition_playlist.hpp"
#include "ov_vf_resolver.hpp"

#include <compare>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace dcplayer::playout::timeline {

using CompositionGraph = dcp::ov_vf_resolver::CompositionGraph;
using CompositionKind = dcp::ov_vf_resolver::CompositionKind;
using DependencyKind = dcp::ov_vf_resolver::DependencyKind;
using Diagnostic = dcp::ov_vf_resolver::Diagnostic;
using DiagnosticSeverity = dcp::ov_vf_resolver::DiagnosticSeverity;
using TrackFile = dcp::ov_vf_resolver::TrackFile;
using ValidationStatus = dcp::ov_vf_resolver::ValidationStatus;

struct LaneEntry {
    std::string asset_id;
    dcp::cpl::TrackType track_type{dcp::cpl::TrackType::picture};
    dcp::cpl::Rational edit_rate;
    std::string resolved_path;
    std::string source_package_id;
    DependencyKind dependency_kind{DependencyKind::local};

    [[nodiscard]] auto operator<=>(const LaneEntry&) const = default;
};

struct TimelineEntry {
    std::size_t entry_index{0U};
    std::string reel_id;
    dcp::cpl::Rational edit_rate;
    std::optional<LaneEntry> picture;
    std::optional<LaneEntry> sound;
    std::optional<LaneEntry> subtitle;

    [[nodiscard]] auto operator<=>(const TimelineEntry&) const = default;
};

struct LaneSummary {
    std::size_t picture{0U};
    std::size_t sound{0U};
    std::size_t subtitle{0U};

    [[nodiscard]] auto operator<=>(const LaneSummary&) const = default;
};

struct PlaybackTimeline {
    std::string composition_id;
    std::string origin_package_id;
    CompositionKind composition_kind{CompositionKind::ov};
    std::vector<std::string> source_packages;
    std::vector<TimelineEntry> reel_entries;
    LaneSummary lane_summary;

    [[nodiscard]] auto operator<=>(const PlaybackTimeline&) const = default;
};

struct BuildResult {
    std::optional<PlaybackTimeline> playback_timeline;
    std::vector<Diagnostic> diagnostics;
    ValidationStatus validation_status{ValidationStatus::ok};

    [[nodiscard]] bool ok() const noexcept {
        return validation_status != ValidationStatus::error;
    }
};

[[nodiscard]] BuildResult build(const CompositionGraph& composition_graph);
[[nodiscard]] std::string to_json(const PlaybackTimeline& playback_timeline);
[[nodiscard]] std::string diagnostics_to_json(const std::vector<Diagnostic>& diagnostics);

}  // namespace dcplayer::playout::timeline
