#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dcplayer::dcp::cpl {

enum class SchemaFlavor {
    interop,
    smpte,
    unknown,
};

enum class DiagnosticSeverity {
    error,
    warning,
};

enum class ValidationStatus {
    ok,
    warning,
    error,
};

enum class TrackType {
    picture,
    sound,
    subtitle,
};

struct Diagnostic {
    std::string code;
    DiagnosticSeverity severity;
    std::string path;
    std::string message;

    [[nodiscard]] auto operator<=>(const Diagnostic&) const = default;
};

struct Rational {
    std::uint32_t numerator{0U};
    std::uint32_t denominator{0U};

    [[nodiscard]] auto operator<=>(const Rational&) const = default;
};

struct AssetReference {
    std::string asset_id;
    TrackType track_type{TrackType::picture};
    std::optional<std::string> annotation_text;
    Rational edit_rate;
};

struct Reel {
    std::string reel_id;
    std::optional<AssetReference> picture;
    std::optional<AssetReference> sound;
    std::optional<AssetReference> subtitle;
};

struct CompositionPlaylist {
    std::string composition_id;
    std::string content_title_text;
    std::optional<std::string> annotation_text;
    std::string issuer;
    std::optional<std::string> creator;
    std::optional<std::string> content_kind;
    std::string issue_date_utc;
    Rational edit_rate;
    std::string namespace_uri;
    SchemaFlavor schema_flavor{SchemaFlavor::unknown};
    std::vector<Reel> reels;
};

struct ParseResult {
    std::optional<CompositionPlaylist> composition_playlist;
    std::vector<Diagnostic> diagnostics;
    ValidationStatus validation_status{ValidationStatus::ok};

    [[nodiscard]] bool ok() const noexcept {
        return validation_status != ValidationStatus::error;
    }
};

[[nodiscard]] ParseResult parse(std::string_view xml_text);
[[nodiscard]] const char* to_string(SchemaFlavor schema_flavor) noexcept;
[[nodiscard]] const char* to_string(DiagnosticSeverity severity) noexcept;
[[nodiscard]] const char* to_string(ValidationStatus status) noexcept;
[[nodiscard]] const char* to_string(TrackType track_type) noexcept;

}  // namespace dcplayer::dcp::cpl
