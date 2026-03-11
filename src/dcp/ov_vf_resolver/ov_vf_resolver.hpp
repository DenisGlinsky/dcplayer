#pragma once

#include "asset_map.hpp"
#include "composition_playlist.hpp"
#include "packing_list.hpp"

#include <compare>
#include <optional>
#include <string>
#include <vector>

namespace dcplayer::dcp::ov_vf_resolver {

enum class DiagnosticSeverity {
    error,
    warning,
};

enum class ValidationStatus {
    ok,
    warning,
    error,
};

enum class CompositionKind {
    ov,
    vf,
};

enum class DependencyKind {
    local,
    external,
};

struct Diagnostic {
    std::string code;
    DiagnosticSeverity severity;
    std::string path;
    std::string message;

    [[nodiscard]] auto operator<=>(const Diagnostic&) const = default;
};

struct TrackFile {
    std::string asset_id;
    cpl::TrackType track_type{cpl::TrackType::picture};
    cpl::Rational edit_rate;
    std::string resolved_path;
    std::string source_package_id;
    DependencyKind dependency_kind{DependencyKind::local};

    [[nodiscard]] auto operator<=>(const TrackFile&) const = default;
};

struct Reel {
    std::string reel_id;
    std::optional<TrackFile> picture_track;
    std::optional<TrackFile> sound_track;
    std::optional<TrackFile> subtitle_track;

    [[nodiscard]] auto operator<=>(const Reel&) const = default;
};

struct CompositionGraph {
    std::string composition_id;
    std::string content_title_text;
    std::string origin_package_id;
    CompositionKind composition_kind{CompositionKind::ov};
    std::vector<std::string> source_packages;
    std::vector<Reel> resolved_reels;
    std::vector<std::string> missing_assets;

    [[nodiscard]] auto operator<=>(const CompositionGraph&) const = default;
};

struct Package {
    assetmap::AssetMap asset_map;
    pkl::PackingList packing_list;
    std::vector<cpl::CompositionPlaylist> composition_playlists;
};

struct ResolveRequest {
    std::string composition_id;
    std::vector<Package> packages;
};

struct ResolveResult {
    std::optional<CompositionGraph> composition_graph;
    std::vector<Diagnostic> diagnostics;
    ValidationStatus validation_status{ValidationStatus::ok};

    [[nodiscard]] bool ok() const noexcept {
        return validation_status != ValidationStatus::error;
    }
};

[[nodiscard]] ResolveResult resolve(const ResolveRequest& request);
[[nodiscard]] const char* to_string(DiagnosticSeverity severity) noexcept;
[[nodiscard]] const char* to_string(ValidationStatus status) noexcept;
[[nodiscard]] const char* to_string(CompositionKind composition_kind) noexcept;
[[nodiscard]] const char* to_string(DependencyKind dependency_kind) noexcept;

}  // namespace dcplayer::dcp::ov_vf_resolver
