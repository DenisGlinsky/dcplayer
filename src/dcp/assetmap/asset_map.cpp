#include "asset_map.hpp"

#include "internal/xml_dom.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace dcplayer::dcp::assetmap {
namespace {

using dcplayer::dcp::xml::Node;

constexpr std::string_view kError = "error";

[[nodiscard]] std::string trim_copy(std::string_view input) {
    constexpr std::string_view whitespace = " \t\r\n";
    const auto begin = input.find_first_not_of(whitespace);
    if (begin == std::string_view::npos) {
        return {};
    }
    const auto end = input.find_last_not_of(whitespace);
    return std::string{input.substr(begin, end - begin + 1U)};
}

[[nodiscard]] std::string lower_copy(std::string_view input) {
    std::string output;
    output.reserve(input.size());
    for (const char current : input) {
        output.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(current))));
    }
    return output;
}

[[nodiscard]] SchemaFlavor schema_flavor_for(std::string_view namespace_uri) {
    if (namespace_uri == "http://www.digicine.com/PROTO-ASDCP-AM-20040311#") {
        return SchemaFlavor::interop;
    }
    if (namespace_uri == "http://www.smpte-ra.org/schemas/429-9/2007/AM") {
        return SchemaFlavor::smpte;
    }
    return SchemaFlavor::unknown;
}

[[nodiscard]] bool is_error(const Diagnostic& diagnostic) noexcept {
    return diagnostic.severity == DiagnosticSeverity::error;
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

[[nodiscard]] std::optional<std::string> required_text(const Node& parent,
                                                       std::string_view child_name,
                                                       std::string path,
                                                       std::vector<Diagnostic>& diagnostics) {
    const auto* child = xml::find_child(parent, child_name);
    if (child == nullptr) {
        add_diagnostic(diagnostics,
                       "assetmap.missing_required_field",
                       DiagnosticSeverity::error,
                       std::move(path),
                       "Missing required field");
        return std::nullopt;
    }

    const std::string value = trim_copy(xml::trimmed_text(*child));
    if (value.empty()) {
        add_diagnostic(diagnostics,
                       "assetmap.missing_required_field",
                       DiagnosticSeverity::error,
                       std::move(path),
                       "Required field is empty");
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] std::optional<std::string> optional_text(const Node& parent, std::string_view child_name) {
    const auto* child = xml::find_child(parent, child_name);
    if (child == nullptr) {
        return std::nullopt;
    }

    const std::string value = trim_copy(xml::trimmed_text(*child));
    if (value.empty()) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] std::optional<std::string> normalized_uuid(std::string_view input) {
    std::string value = lower_copy(trim_copy(input));
    constexpr std::string_view prefix = "urn:uuid:";
    if (value.rfind(prefix, 0U) == 0U) {
        value.erase(0U, prefix.size());
    }

    if (value.size() != 36U) {
        return std::nullopt;
    }

    for (std::size_t index = 0; index < value.size(); ++index) {
        const char current = value[index];
        const bool hyphen_position = index == 8U || index == 13U || index == 18U || index == 23U;
        if (hyphen_position) {
            if (current != '-') {
                return std::nullopt;
            }
            continue;
        }
        if (!std::isxdigit(static_cast<unsigned char>(current))) {
            return std::nullopt;
        }
    }
    return value;
}

[[nodiscard]] bool is_leap_year(int year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

[[nodiscard]] int days_in_month(int year, int month) noexcept {
    static constexpr int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2) {
        return is_leap_year(year) ? 29 : 28;
    }
    return kDays[month - 1];
}

[[nodiscard]] bool is_valid_utc_timestamp(std::string_view input) {
    if (input.size() != 20U) {
        return false;
    }

    const auto digit = [&](std::size_t index) noexcept {
        return std::isdigit(static_cast<unsigned char>(input[index])) != 0;
    };
    const bool structure_ok = digit(0U) && digit(1U) && digit(2U) && digit(3U) && input[4U] == '-' && digit(5U) &&
                              digit(6U) && input[7U] == '-' && digit(8U) && digit(9U) && input[10U] == 'T' &&
                              digit(11U) && digit(12U) && input[13U] == ':' && digit(14U) && digit(15U) &&
                              input[16U] == ':' && digit(17U) && digit(18U) && input[19U] == 'Z';
    if (!structure_ok) {
        return false;
    }

    const int year = std::stoi(std::string{input.substr(0U, 4U)});
    const int month = std::stoi(std::string{input.substr(5U, 2U)});
    const int day = std::stoi(std::string{input.substr(8U, 2U)});
    const int hour = std::stoi(std::string{input.substr(11U, 2U)});
    const int minute = std::stoi(std::string{input.substr(14U, 2U)});
    const int second = std::stoi(std::string{input.substr(17U, 2U)});

    if (month < 1 || month > 12) {
        return false;
    }
    if (day < 1 || day > days_in_month(year, month)) {
        return false;
    }
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return false;
    }
    return true;
}

template <typename Integer>
[[nodiscard]] std::optional<Integer> parse_unsigned_integer(std::string_view input) {
    const std::string value = trim_copy(input);
    if (value.empty()) {
        return std::nullopt;
    }

    Integer parsed_value{};
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const auto result = std::from_chars(begin, end, parsed_value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return parsed_value;
}

[[nodiscard]] std::optional<std::string> normalize_relative_path(std::string_view input) {
    const std::string trimmed = trim_copy(input);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    if (trimmed.front() == '/' || trimmed.find('\\') != std::string::npos) {
        return std::nullopt;
    }

    std::vector<std::string> segments;
    std::size_t start = 0U;
    while (start <= trimmed.size()) {
        const auto end = trimmed.find('/', start);
        const std::string_view segment = end == std::string::npos
                                             ? std::string_view{trimmed}.substr(start)
                                             : std::string_view{trimmed}.substr(start, end - start);

        if (segment.empty() || segment == ".") {
            if (end == std::string::npos) {
                break;
            }
            start = end + 1U;
            continue;
        }
        if (segment == "..") {
            return std::nullopt;
        }
        segments.emplace_back(segment);

        if (end == std::string::npos) {
            break;
        }
        start = end + 1U;
    }

    if (segments.empty()) {
        return std::nullopt;
    }

    std::string normalized = segments.front();
    for (std::size_t index = 1U; index < segments.size(); ++index) {
        normalized += "/";
        normalized += segments[index];
    }
    return normalized;
}

[[nodiscard]] bool text_xml_hint_for(const std::vector<ChunkEntry>& chunks) {
    if (chunks.empty()) {
        return false;
    }
    const std::string lower_path = lower_copy(chunks.front().path);
    return lower_path.size() >= 4U && lower_path.substr(lower_path.size() - 4U) == ".xml";
}

[[nodiscard]] bool packing_list_hint_for(const Node& asset_node) {
    const auto value = optional_text(asset_node, "PackingList");
    if (!value.has_value()) {
        return false;
    }
    const std::string normalized = lower_copy(*value);
    return normalized == "true" || normalized == "1";
}

}  // namespace

ParseResult parse(std::string_view xml_text) {
    ParseResult result;

    const auto xml_result = xml::parse_document(xml_text);
    if (!xml_result.ok()) {
        add_diagnostic(result.diagnostics,
                       "assetmap.xml_malformed",
                       DiagnosticSeverity::error,
                       xml_result.error->path.empty() ? "/" : xml_result.error->path,
                       "Malformed XML: " + xml_result.error->message);
        result.validation_status = ValidationStatus::error;
        return result;
    }

    const Node& root = xml_result.document->root;
    if (root.local_name != "AssetMap") {
        add_diagnostic(result.diagnostics,
                       "assetmap.missing_required_field",
                       DiagnosticSeverity::error,
                       "/AssetMap",
                       "Root element must be AssetMap");
        result.validation_status = ValidationStatus::error;
        return result;
    }

    AssetMap asset_map;
    asset_map.namespace_uri = root.namespace_uri().value_or("");
    asset_map.schema_flavor = schema_flavor_for(asset_map.namespace_uri);
    if (asset_map.schema_flavor == SchemaFlavor::unknown) {
        add_diagnostic(result.diagnostics,
                       "assetmap.unsupported_namespace",
                       DiagnosticSeverity::warning,
                       "/AssetMap",
                       "Unsupported namespace URI: " + asset_map.namespace_uri);
    }

    if (const auto value = required_text(root, "Id", "/AssetMap/Id", result.diagnostics); value.has_value()) {
        const auto normalized = normalized_uuid(*value);
        if (normalized.has_value()) {
            asset_map.asset_map_id = *normalized;
        } else {
            add_diagnostic(result.diagnostics,
                           "assetmap.invalid_uuid",
                           DiagnosticSeverity::error,
                           "/AssetMap/Id",
                           "Invalid UUID value");
        }
    }

    if (const auto value = required_text(root, "Issuer", "/AssetMap/Issuer", result.diagnostics); value.has_value()) {
        asset_map.issuer = *value;
    }

    if (const auto value = required_text(root, "IssueDate", "/AssetMap/IssueDate", result.diagnostics); value.has_value()) {
        if (is_valid_utc_timestamp(*value)) {
            asset_map.issue_date_utc = *value;
        } else {
            add_diagnostic(result.diagnostics,
                           "assetmap.invalid_time",
                           DiagnosticSeverity::error,
                           "/AssetMap/IssueDate",
                           "Invalid UTC timestamp");
        }
    }

    asset_map.creator = optional_text(root, "Creator");

    if (const auto value = required_text(root, "VolumeCount", "/AssetMap/VolumeCount", result.diagnostics); value.has_value()) {
        const auto parsed_value = parse_unsigned_integer<std::uint32_t>(*value);
        if (parsed_value.has_value() && *parsed_value >= 1U) {
            asset_map.volume_count = *parsed_value;
        } else {
            add_diagnostic(result.diagnostics,
                           "assetmap.invalid_integer",
                           DiagnosticSeverity::error,
                           "/AssetMap/VolumeCount",
                           "VolumeCount must be an integer >= 1");
        }
    }

    const Node* asset_list = xml::find_child(root, "AssetList");
    if (asset_list == nullptr) {
        add_diagnostic(result.diagnostics,
                       "assetmap.missing_required_field",
                       DiagnosticSeverity::error,
                       "/AssetMap/AssetList",
                       "Missing required field");
    } else {
        std::unordered_set<std::string> asset_ids;

        const auto asset_nodes = xml::find_children(*asset_list, "Asset");
        for (std::size_t asset_index = 0U; asset_index < asset_nodes.size(); ++asset_index) {
            const Node& asset_node = *asset_nodes[asset_index];
            const std::string asset_path = "/AssetMap/AssetList/Asset[" + std::to_string(asset_index + 1U) + "]";

            AssetEntry asset_entry;
            asset_entry.packing_list_hint = packing_list_hint_for(asset_node);

            if (const auto value = required_text(asset_node, "Id", asset_path + "/Id", result.diagnostics); value.has_value()) {
                const auto normalized = normalized_uuid(*value);
                if (normalized.has_value()) {
                    asset_entry.asset_id = *normalized;
                    if (!asset_ids.insert(*normalized).second) {
                        add_diagnostic(result.diagnostics,
                                       "assetmap.duplicate_asset_id",
                                       DiagnosticSeverity::error,
                                       asset_path + "/Id",
                                       "Duplicate asset_id");
                    }
                } else {
                    add_diagnostic(result.diagnostics,
                                   "assetmap.invalid_uuid",
                                   DiagnosticSeverity::error,
                                   asset_path + "/Id",
                                   "Invalid UUID value");
                }
            }

            const Node* chunk_list = xml::find_child(asset_node, "ChunkList");
            if (chunk_list == nullptr) {
                add_diagnostic(result.diagnostics,
                               "assetmap.empty_chunk_list",
                               DiagnosticSeverity::error,
                               asset_path + "/ChunkList",
                               "Asset must contain at least one chunk");
            } else {
                const auto chunk_nodes = xml::find_children(*chunk_list, "Chunk");
                if (chunk_nodes.empty()) {
                    add_diagnostic(result.diagnostics,
                                   "assetmap.empty_chunk_list",
                                   DiagnosticSeverity::error,
                                   asset_path + "/ChunkList",
                                   "Asset must contain at least one chunk");
                } else {
                    std::set<std::tuple<std::uint32_t, std::string, std::uint64_t>> chunk_keys;
                    for (std::size_t chunk_index = 0U; chunk_index < chunk_nodes.size(); ++chunk_index) {
                        const Node& chunk_node = *chunk_nodes[chunk_index];
                        const std::string chunk_path = asset_path + "/ChunkList/Chunk[" + std::to_string(chunk_index + 1U) + "]";

                        ChunkEntry chunk_entry;

                        if (const auto value = required_text(chunk_node, "Path", chunk_path + "/Path", result.diagnostics); value.has_value()) {
                            const auto normalized = normalize_relative_path(*value);
                            if (normalized.has_value()) {
                                chunk_entry.path = *normalized;
                            } else {
                                add_diagnostic(result.diagnostics,
                                               "assetmap.invalid_relative_path",
                                               DiagnosticSeverity::error,
                                               chunk_path + "/Path",
                                               "Path must be a normalized relative POSIX path");
                            }
                        }

                        if (const auto value = required_text(chunk_node, "Offset", chunk_path + "/Offset", result.diagnostics); value.has_value()) {
                            const auto parsed_value = parse_unsigned_integer<std::uint64_t>(*value);
                            if (parsed_value.has_value()) {
                                chunk_entry.offset = *parsed_value;
                            } else {
                                add_diagnostic(result.diagnostics,
                                               "assetmap.invalid_integer",
                                               DiagnosticSeverity::error,
                                               chunk_path + "/Offset",
                                               "Offset must be an integer >= 0");
                            }
                        }

                        if (const auto value = required_text(chunk_node, "Length", chunk_path + "/Length", result.diagnostics); value.has_value()) {
                            const auto parsed_value = parse_unsigned_integer<std::uint64_t>(*value);
                            if (parsed_value.has_value() && *parsed_value > 0U) {
                                chunk_entry.length = *parsed_value;
                            } else {
                                add_diagnostic(result.diagnostics,
                                               "assetmap.invalid_integer",
                                               DiagnosticSeverity::error,
                                               chunk_path + "/Length",
                                               "Length must be an integer > 0");
                            }
                        }

                        if (const auto value = required_text(chunk_node, "VolumeIndex", chunk_path + "/VolumeIndex", result.diagnostics);
                            value.has_value()) {
                            const auto parsed_value = parse_unsigned_integer<std::uint32_t>(*value);
                            if (parsed_value.has_value() && *parsed_value >= 1U) {
                                chunk_entry.volume_index = *parsed_value;
                                if (asset_map.volume_count != 0U && *parsed_value > asset_map.volume_count) {
                                    add_diagnostic(result.diagnostics,
                                                   "assetmap.invalid_integer",
                                                   DiagnosticSeverity::error,
                                                   chunk_path + "/VolumeIndex",
                                                   "VolumeIndex must be <= VolumeCount");
                                }
                            } else {
                                add_diagnostic(result.diagnostics,
                                               "assetmap.invalid_integer",
                                               DiagnosticSeverity::error,
                                               chunk_path + "/VolumeIndex",
                                               "VolumeIndex must be an integer >= 1");
                            }
                        }

                        if (!chunk_entry.path.empty() && chunk_entry.volume_index != 0U && chunk_entry.length != 0U) {
                            const auto key = std::tuple{chunk_entry.volume_index, chunk_entry.path, chunk_entry.offset};
                            if (!chunk_keys.insert(key).second) {
                                add_diagnostic(result.diagnostics,
                                               "assetmap.duplicate_chunk_entry",
                                               DiagnosticSeverity::error,
                                               chunk_path,
                                               "Duplicate chunk entry");
                            }
                        }

                        asset_entry.chunk_list.push_back(std::move(chunk_entry));
                    }
                }
            }

            asset_entry.is_text_xml = text_xml_hint_for(asset_entry.chunk_list);
            asset_map.assets.push_back(std::move(asset_entry));
        }
    }

    sort_diagnostics(result.diagnostics);
    result.validation_status = validation_status_for(result.diagnostics);
    if (result.ok()) {
        result.asset_map = std::move(asset_map);
    }
    return result;
}

const char* to_string(SchemaFlavor schema_flavor) noexcept {
    switch (schema_flavor) {
        case SchemaFlavor::interop:
            return "interop";
        case SchemaFlavor::smpte:
            return "smpte";
        case SchemaFlavor::unknown:
            return "unknown";
    }
    return "unknown";
}

const char* to_string(DiagnosticSeverity severity) noexcept {
    switch (severity) {
        case DiagnosticSeverity::error:
            return "error";
        case DiagnosticSeverity::warning:
            return "warning";
    }
    return kError.data();
}

const char* to_string(ValidationStatus status) noexcept {
    switch (status) {
        case ValidationStatus::ok:
            return "ok";
        case ValidationStatus::warning:
            return "warning";
        case ValidationStatus::error:
            return kError.data();
    }
    return kError.data();
}

}  // namespace dcplayer::dcp::assetmap
