#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dcplayer::dcp::assetmap {

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

struct Diagnostic {
    std::string code;
    DiagnosticSeverity severity;
    std::string path;
    std::string message;

    [[nodiscard]] auto operator<=>(const Diagnostic&) const = default;
};

struct ChunkEntry {
    std::string path;
    std::uint64_t offset{0U};
    std::uint64_t length{0U};
    std::uint32_t volume_index{0U};
};

struct AssetEntry {
    std::string asset_id;
    std::vector<ChunkEntry> chunk_list;
    bool packing_list_hint{false};
    bool is_text_xml{false};
};

struct AssetMap {
    std::string asset_map_id;
    std::string issuer;
    std::string issue_date_utc;
    std::optional<std::string> creator;
    std::uint32_t volume_count{0U};
    std::string namespace_uri;
    SchemaFlavor schema_flavor{SchemaFlavor::unknown};
    std::vector<AssetEntry> assets;
};

struct ParseResult {
    std::optional<AssetMap> asset_map;
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

}  // namespace dcplayer::dcp::assetmap
