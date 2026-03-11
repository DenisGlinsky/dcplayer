#include "packing_list.hpp"

#include "asset_map.hpp"
#include "internal/xml_dom.hpp"

#include <algorithm>
#include <array>
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

namespace dcplayer::dcp::pkl {
namespace {

using dcplayer::dcp::xml::Node;

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

[[nodiscard]] SchemaFlavor schema_flavor_for(std::string_view namespace_uri) {
    if (namespace_uri == "http://www.digicine.com/PROTO-ASDCP-PKL-20040311#") {
        return SchemaFlavor::interop;
    }
    if (namespace_uri == "http://www.smpte-ra.org/schemas/429-8/2007/PKL") {
        return SchemaFlavor::smpte;
    }
    return SchemaFlavor::unknown;
}

[[nodiscard]] std::optional<std::string> required_text(const Node& parent,
                                                       std::string_view child_name,
                                                       std::string path,
                                                       std::vector<Diagnostic>& diagnostics) {
    const auto* child = xml::find_child(parent, child_name);
    if (child == nullptr) {
        add_diagnostic(diagnostics,
                       "pkl.missing_required_field",
                       DiagnosticSeverity::error,
                       std::move(path),
                       "Missing required field");
        return std::nullopt;
    }

    const std::string value = trim_copy(xml::trimmed_text(*child));
    if (value.empty()) {
        add_diagnostic(diagnostics,
                       "pkl.missing_required_field",
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

    for (std::size_t index = 0U; index < value.size(); ++index) {
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

[[nodiscard]] AssetType asset_type_for(std::string_view raw_type) {
    const std::string normalized = lower_copy(raw_type);
    if (normalized.find("asdcpkind=pkl") != std::string::npos) {
        return AssetType::packing_list;
    }
    if (normalized.find("asdcpkind=cpl") != std::string::npos) {
        return AssetType::composition_playlist;
    }
    if (normalized.find("application/mxf") != std::string::npos) {
        return AssetType::track_file;
    }
    if (normalized.find("xml") != std::string::npos) {
        return AssetType::text_xml;
    }
    return AssetType::unknown;
}

[[nodiscard]] DigestAlgorithm digest_algorithm_for(std::string_view raw_algorithm) {
    const std::string normalized = lower_copy(raw_algorithm);
    if (normalized.find("sha1") != std::string::npos) {
        return DigestAlgorithm::sha1;
    }
    if (normalized.find("sha256") != std::string::npos) {
        return DigestAlgorithm::sha256;
    }
    return DigestAlgorithm::unknown;
}

[[nodiscard]] bool is_hex_character(char current) noexcept {
    return std::isxdigit(static_cast<unsigned char>(current)) != 0;
}

[[nodiscard]] std::optional<std::uint8_t> hex_nibble(char current) {
    if (current >= '0' && current <= '9') {
        return static_cast<std::uint8_t>(current - '0');
    }
    const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(current)));
    if (lower >= 'a' && lower <= 'f') {
        return static_cast<std::uint8_t>(10 + lower - 'a');
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::vector<std::uint8_t>> decode_hex(std::string_view input) {
    if (input.size() % 2U != 0U) {
        return std::nullopt;
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(input.size() / 2U);
    for (std::size_t index = 0U; index < input.size(); index += 2U) {
        const auto high = hex_nibble(input[index]);
        const auto low = hex_nibble(input[index + 1U]);
        if (!high.has_value() || !low.has_value()) {
            return std::nullopt;
        }
        bytes.push_back(static_cast<std::uint8_t>((*high << 4U) | *low));
    }
    return bytes;
}

[[nodiscard]] std::optional<std::vector<std::uint8_t>> decode_base64(std::string_view input) {
    static constexpr std::string_view kAlphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string compact;
    compact.reserve(input.size());
    for (const char current : input) {
        if (std::isspace(static_cast<unsigned char>(current)) == 0) {
            compact.push_back(current);
        }
    }

    if (compact.empty() || compact.size() % 4U != 0U) {
        return std::nullopt;
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve((compact.size() / 4U) * 3U);

    for (std::size_t index = 0U; index < compact.size(); index += 4U) {
        std::array<int, 4U> values{};
        int padding = 0;
        for (std::size_t block_index = 0U; block_index < 4U; ++block_index) {
            const char current = compact[index + block_index];
            if (current == '=') {
                values[block_index] = 0;
                ++padding;
                continue;
            }

            const auto position = kAlphabet.find(current);
            if (position == std::string_view::npos) {
                return std::nullopt;
            }
            values[block_index] = static_cast<int>(position);
        }

        bytes.push_back(static_cast<std::uint8_t>((values[0] << 2) | (values[1] >> 4)));
        if (padding < 2) {
            bytes.push_back(static_cast<std::uint8_t>(((values[1] & 0x0f) << 4) | (values[2] >> 2)));
        }
        if (padding == 0) {
            bytes.push_back(static_cast<std::uint8_t>(((values[2] & 0x03) << 6) | values[3]));
        }
    }

    return bytes;
}

[[nodiscard]] std::string hex_encode(const std::vector<std::uint8_t>& bytes) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string encoded;
    encoded.reserve(bytes.size() * 2U);
    for (const std::uint8_t byte : bytes) {
        encoded.push_back(kHex[(byte >> 4U) & 0x0fU]);
        encoded.push_back(kHex[byte & 0x0fU]);
    }
    return encoded;
}

[[nodiscard]] std::optional<std::string> normalize_digest_value(DigestAlgorithm algorithm, std::string_view input) {
    if (algorithm == DigestAlgorithm::unknown) {
        return std::nullopt;
    }

    const std::string trimmed = trim_copy(input);
    if (trimmed.empty()) {
        return std::nullopt;
    }

    const std::size_t expected_bytes = algorithm == DigestAlgorithm::sha1 ? 20U : 32U;
    const bool looks_hex = std::all_of(trimmed.begin(), trimmed.end(), is_hex_character);

    if (looks_hex) {
        const auto decoded = decode_hex(trimmed);
        if (!decoded.has_value() || decoded->size() != expected_bytes) {
            return std::nullopt;
        }
        return lower_copy(trimmed);
    }

    const auto decoded = decode_base64(trimmed);
    if (!decoded.has_value() || decoded->size() != expected_bytes) {
        return std::nullopt;
    }
    return hex_encode(*decoded);
}

}  // namespace

ParseResult parse(std::string_view xml_text) {
    ParseResult result;

    const auto xml_result = xml::parse_document(xml_text);
    if (!xml_result.ok()) {
        add_diagnostic(result.diagnostics,
                       "pkl.xml_malformed",
                       DiagnosticSeverity::error,
                       xml_result.error->path.empty() ? "/" : xml_result.error->path,
                       "Malformed XML: " + xml_result.error->message);
        result.validation_status = ValidationStatus::error;
        return result;
    }

    const Node& root = xml_result.document->root;
    if (root.local_name != "PackingList") {
        add_diagnostic(result.diagnostics,
                       "pkl.missing_required_field",
                       DiagnosticSeverity::error,
                       "/PackingList",
                       "Root element must be PackingList");
        result.validation_status = ValidationStatus::error;
        return result;
    }

    PackingList packing_list;
    packing_list.namespace_uri = root.namespace_uri().value_or("");
    packing_list.schema_flavor = schema_flavor_for(packing_list.namespace_uri);
    if (packing_list.schema_flavor == SchemaFlavor::unknown) {
        add_diagnostic(result.diagnostics,
                       "pkl.unsupported_namespace",
                       DiagnosticSeverity::warning,
                       "/PackingList",
                       "Unsupported namespace URI: " + packing_list.namespace_uri);
    }

    if (const auto value = required_text(root, "Id", "/PackingList/Id", result.diagnostics); value.has_value()) {
        const auto normalized = normalized_uuid(*value);
        if (normalized.has_value()) {
            packing_list.pkl_id = *normalized;
        } else {
            add_diagnostic(result.diagnostics,
                           "pkl.invalid_uuid",
                           DiagnosticSeverity::error,
                           "/PackingList/Id",
                           "Invalid UUID value");
        }
    }

    packing_list.annotation_text = optional_text(root, "AnnotationText");

    if (const auto value = required_text(root, "Issuer", "/PackingList/Issuer", result.diagnostics); value.has_value()) {
        packing_list.issuer = *value;
    }

    if (const auto value = required_text(root, "IssueDate", "/PackingList/IssueDate", result.diagnostics); value.has_value()) {
        if (is_valid_utc_timestamp(*value)) {
            packing_list.issue_date_utc = *value;
        } else {
            add_diagnostic(result.diagnostics,
                           "pkl.invalid_time",
                           DiagnosticSeverity::error,
                           "/PackingList/IssueDate",
                           "Invalid UTC timestamp");
        }
    }

    packing_list.creator = optional_text(root, "Creator");

    const Node* asset_list = xml::find_child(root, "AssetList");
    if (asset_list == nullptr) {
        add_diagnostic(result.diagnostics,
                       "pkl.missing_required_field",
                       DiagnosticSeverity::error,
                       "/PackingList/AssetList",
                       "Missing required field");
    } else {
        std::unordered_set<std::string> asset_ids;
        const auto asset_nodes = xml::find_children(*asset_list, "Asset");
        for (std::size_t asset_index = 0U; asset_index < asset_nodes.size(); ++asset_index) {
            const Node& asset_node = *asset_nodes[asset_index];
            const std::string asset_path = "/PackingList/AssetList/Asset[" + std::to_string(asset_index + 1U) + "]";

            AssetEntry asset_entry;
            asset_entry.annotation_text = optional_text(asset_node, "AnnotationText");
            asset_entry.original_filename = optional_text(asset_node, "OriginalFileName");

            if (const auto value = required_text(asset_node, "Id", asset_path + "/Id", result.diagnostics); value.has_value()) {
                const auto normalized = normalized_uuid(*value);
                if (normalized.has_value()) {
                    asset_entry.asset_id = *normalized;
                    if (!asset_ids.insert(*normalized).second) {
                        add_diagnostic(result.diagnostics,
                                       "pkl.duplicate_asset_id",
                                       DiagnosticSeverity::error,
                                       asset_path + "/Id",
                                       "Duplicate asset_id");
                    }
                } else {
                    add_diagnostic(result.diagnostics,
                                   "pkl.invalid_uuid",
                                   DiagnosticSeverity::error,
                                   asset_path + "/Id",
                                   "Invalid UUID value");
                }
            }

            if (const auto value = required_text(asset_node, "Type", asset_path + "/Type", result.diagnostics); value.has_value()) {
                asset_entry.type_value = *value;
                asset_entry.asset_type = asset_type_for(*value);
            }

            if (const auto value = required_text(asset_node, "Size", asset_path + "/Size", result.diagnostics); value.has_value()) {
                const auto parsed_value = parse_unsigned_integer<std::uint64_t>(*value);
                if (parsed_value.has_value() && *parsed_value > 0U) {
                    asset_entry.size_bytes = *parsed_value;
                } else {
                    add_diagnostic(result.diagnostics,
                                   "pkl.invalid_integer",
                                   DiagnosticSeverity::error,
                                   asset_path + "/Size",
                                   "Size must be an integer > 0");
                }
            }

            const Node* hash_node = xml::find_child(asset_node, "Hash");
            if (hash_node == nullptr) {
                add_diagnostic(result.diagnostics,
                               "pkl.missing_required_field",
                               DiagnosticSeverity::error,
                               asset_path + "/Hash",
                               "Missing required field");
            } else {
                asset_entry.hash.value = trim_copy(xml::trimmed_text(*hash_node));

                const auto* algorithm_attribute = hash_node->find_attribute("Algorithm");
                if (algorithm_attribute == nullptr || trim_copy(algorithm_attribute->value).empty()) {
                    add_diagnostic(result.diagnostics,
                                   "pkl.invalid_digest_algorithm",
                                   DiagnosticSeverity::error,
                                   asset_path + "/Hash/@Algorithm",
                                   "Hash algorithm is required");
                } else {
                    asset_entry.hash.algorithm = digest_algorithm_for(algorithm_attribute->value);
                    if (asset_entry.hash.algorithm == DigestAlgorithm::unknown) {
                        add_diagnostic(result.diagnostics,
                                       "pkl.invalid_digest_algorithm",
                                       DiagnosticSeverity::error,
                                       asset_path + "/Hash/@Algorithm",
                                       "Unsupported digest algorithm");
                    }
                }

                const auto normalized_digest = normalize_digest_value(asset_entry.hash.algorithm, asset_entry.hash.value);
                if (normalized_digest.has_value()) {
                    asset_entry.hash.normalized_hex = *normalized_digest;
                } else {
                    add_diagnostic(result.diagnostics,
                                   "pkl.invalid_digest_value",
                                   DiagnosticSeverity::error,
                                   asset_path + "/Hash",
                                   "Digest value does not match the declared algorithm");
                }
            }

            packing_list.assets.push_back(std::move(asset_entry));
        }
    }

    sort_diagnostics(result.diagnostics);
    result.validation_status = validation_status_for(result.diagnostics);
    if (result.ok()) {
        result.packing_list = std::move(packing_list);
    }
    return result;
}

std::vector<Diagnostic> validate_against_asset_map(const PackingList& packing_list, const assetmap::AssetMap& asset_map) {
    std::vector<Diagnostic> diagnostics;

    std::unordered_set<std::string> asset_ids;
    for (const auto& asset : asset_map.assets) {
        asset_ids.insert(asset.asset_id);
    }

    for (std::size_t index = 0U; index < packing_list.assets.size(); ++index) {
        const auto& asset = packing_list.assets[index];
        if (asset_ids.contains(asset.asset_id)) {
            continue;
        }

        add_diagnostic(diagnostics,
                       "pkl.asset_missing_in_assetmap",
                       DiagnosticSeverity::error,
                       "/PackingList/AssetList/Asset[" + std::to_string(index + 1U) + "]/Id",
                       "PKL asset_id is absent from AssetMap");
    }

    sort_diagnostics(diagnostics);
    return diagnostics;
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
    return "error";
}

const char* to_string(ValidationStatus status) noexcept {
    switch (status) {
        case ValidationStatus::ok:
            return "ok";
        case ValidationStatus::warning:
            return "warning";
        case ValidationStatus::error:
            return "error";
    }
    return "error";
}

const char* to_string(AssetType asset_type) noexcept {
    switch (asset_type) {
        case AssetType::packing_list:
            return "packing_list";
        case AssetType::composition_playlist:
            return "composition_playlist";
        case AssetType::track_file:
            return "track_file";
        case AssetType::text_xml:
            return "text_xml";
        case AssetType::unknown:
            return "unknown";
    }
    return "unknown";
}

const char* to_string(DigestAlgorithm algorithm) noexcept {
    switch (algorithm) {
        case DigestAlgorithm::sha1:
            return "sha1";
        case DigestAlgorithm::sha256:
            return "sha256";
        case DigestAlgorithm::unknown:
            return "unknown";
    }
    return "unknown";
}

}  // namespace dcplayer::dcp::pkl
