#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dcplayer::dcp::assetmap {
struct AssetMap;
}

namespace dcplayer::dcp::pkl {

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

enum class AssetType {
    packing_list,
    composition_playlist,
    track_file,
    text_xml,
    unknown,
};

enum class DigestAlgorithm {
    sha1,
    sha256,
    unknown,
};

struct Diagnostic {
    std::string code;
    DiagnosticSeverity severity;
    std::string path;
    std::string message;

    [[nodiscard]] auto operator<=>(const Diagnostic&) const = default;
};

struct DigestEntry {
    DigestAlgorithm algorithm{DigestAlgorithm::unknown};
    std::string value;
    std::optional<std::string> normalized_hex;
};

struct AssetEntry {
    std::string asset_id;
    std::optional<std::string> annotation_text;
    AssetType asset_type{AssetType::unknown};
    std::string type_value;
    std::optional<std::string> original_filename;
    std::uint64_t size_bytes{0U};
    DigestEntry hash;
};

struct PackingList {
    std::string pkl_id;
    std::optional<std::string> annotation_text;
    std::string issuer;
    std::optional<std::string> creator;
    std::string issue_date_utc;
    std::string namespace_uri;
    SchemaFlavor schema_flavor{SchemaFlavor::unknown};
    std::vector<AssetEntry> assets;
};

struct ParseResult {
    std::optional<PackingList> packing_list;
    std::vector<Diagnostic> diagnostics;
    ValidationStatus validation_status{ValidationStatus::ok};

    [[nodiscard]] bool ok() const noexcept {
        return validation_status != ValidationStatus::error;
    }
};

[[nodiscard]] ParseResult parse(std::string_view xml_text);
[[nodiscard]] std::vector<Diagnostic> validate_against_asset_map(const PackingList& packing_list,
                                                                 const assetmap::AssetMap& asset_map);
[[nodiscard]] const char* to_string(SchemaFlavor schema_flavor) noexcept;
[[nodiscard]] const char* to_string(DiagnosticSeverity severity) noexcept;
[[nodiscard]] const char* to_string(ValidationStatus status) noexcept;
[[nodiscard]] const char* to_string(AssetType asset_type) noexcept;
[[nodiscard]] const char* to_string(DigestAlgorithm algorithm) noexcept;

}  // namespace dcplayer::dcp::pkl
