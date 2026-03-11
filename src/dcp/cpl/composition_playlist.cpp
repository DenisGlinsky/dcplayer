#include "composition_playlist.hpp"

#include "internal/xml_dom.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace dcplayer::dcp::cpl {
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

[[nodiscard]] std::optional<std::string> required_text(const Node& parent,
                                                       std::string_view child_name,
                                                       std::string path,
                                                       std::vector<Diagnostic>& diagnostics) {
    const auto* child = xml::find_child(parent, child_name);
    if (child == nullptr) {
        add_diagnostic(diagnostics,
                       "cpl.missing_required_field",
                       DiagnosticSeverity::error,
                       std::move(path),
                       "Missing required field");
        return std::nullopt;
    }

    const std::string value = trim_copy(xml::trimmed_text(*child));
    if (value.empty()) {
        add_diagnostic(diagnostics,
                       "cpl.missing_required_field",
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
        if (std::isxdigit(static_cast<unsigned char>(current)) == 0) {
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

[[nodiscard]] std::optional<Rational> parse_edit_rate(std::string_view input) {
    const std::string value = trim_copy(input);
    if (value.empty()) {
        return std::nullopt;
    }

    std::vector<std::string_view> parts;
    std::size_t start = 0U;
    while (start < value.size()) {
        while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
            ++start;
        }
        if (start >= value.size()) {
            break;
        }

        std::size_t end = start;
        while (end < value.size() && std::isspace(static_cast<unsigned char>(value[end])) == 0) {
            ++end;
        }
        parts.push_back(std::string_view{value}.substr(start, end - start));
        start = end;
    }

    if (parts.size() != 2U) {
        return std::nullopt;
    }

    const auto numerator = parse_unsigned_integer<std::uint32_t>(parts[0]);
    const auto denominator = parse_unsigned_integer<std::uint32_t>(parts[1]);
    if (!numerator.has_value() || !denominator.has_value() || *numerator == 0U || *denominator == 0U) {
        return std::nullopt;
    }

    return Rational{
        .numerator = *numerator,
        .denominator = *denominator,
    };
}

[[nodiscard]] SchemaFlavor schema_flavor_for(std::string_view namespace_uri) {
    if (namespace_uri == "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#") {
        return SchemaFlavor::interop;
    }
    if (namespace_uri == "http://www.smpte-ra.org/schemas/429-7/2006/CPL") {
        return SchemaFlavor::smpte;
    }
    return SchemaFlavor::unknown;
}

[[nodiscard]] std::optional<TrackType> track_type_for(std::string_view local_name) {
    if (local_name == "MainPicture") {
        return TrackType::picture;
    }
    if (local_name == "MainSound") {
        return TrackType::sound;
    }
    if (local_name == "MainSubtitle") {
        return TrackType::subtitle;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<AssetReference>* slot_for(Reel& reel, TrackType track_type) {
    switch (track_type) {
        case TrackType::picture:
            return &reel.picture;
        case TrackType::sound:
            return &reel.sound;
        case TrackType::subtitle:
            return &reel.subtitle;
    }
    return &reel.picture;
}

void parse_track_reference(const Node& track_node,
                           std::string_view track_path,
                           TrackType track_type,
                           std::vector<Diagnostic>& diagnostics,
                           std::optional<AssetReference>& output) {
    AssetReference reference;
    reference.track_type = track_type;
    reference.annotation_text = optional_text(track_node, "AnnotationText");

    if (const auto value = required_text(track_node, "Id", std::string{track_path} + "/Id", diagnostics); value.has_value()) {
        const auto normalized = normalized_uuid(*value);
        if (normalized.has_value()) {
            reference.asset_id = *normalized;
        } else {
            add_diagnostic(diagnostics,
                           "cpl.invalid_uuid",
                           DiagnosticSeverity::error,
                           std::string{track_path} + "/Id",
                           "Invalid UUID value");
        }
    }

    if (const auto value = required_text(track_node, "EditRate", std::string{track_path} + "/EditRate", diagnostics);
        value.has_value()) {
        const auto edit_rate = parse_edit_rate(*value);
        if (edit_rate.has_value()) {
            reference.edit_rate = *edit_rate;
        } else {
            add_diagnostic(diagnostics,
                           "cpl.invalid_edit_rate",
                           DiagnosticSeverity::error,
                           std::string{track_path} + "/EditRate",
                           "EditRate must contain two positive integers");
        }
    }

    output = std::move(reference);
}

void validate_duplicate_asset_references(const Reel& reel, std::string_view asset_list_path, std::vector<Diagnostic>& diagnostics) {
    struct TrackEntry {
        const std::optional<AssetReference>* reference;
        std::string_view element_name;
    };

    const std::array<TrackEntry, 3U> tracks{{
        TrackEntry{&reel.picture, "MainPicture"},
        TrackEntry{&reel.sound, "MainSound"},
        TrackEntry{&reel.subtitle, "MainSubtitle"},
    }};

    std::unordered_set<std::string> asset_ids;
    for (const auto& track : tracks) {
        if (!track.reference->has_value() || track.reference->value().asset_id.empty()) {
            continue;
        }

        if (!asset_ids.insert(track.reference->value().asset_id).second) {
            add_diagnostic(diagnostics,
                           "cpl.duplicate_asset_reference",
                           DiagnosticSeverity::error,
                           std::string{asset_list_path} + "/" + std::string{track.element_name} + "/Id",
                           "Duplicate asset_id within reel");
        }
    }
}

}  // namespace

ParseResult parse(std::string_view xml_text) {
    ParseResult result;

    const auto xml_result = xml::parse_document(xml_text);
    if (!xml_result.ok()) {
        add_diagnostic(result.diagnostics,
                       "cpl.xml_malformed",
                       DiagnosticSeverity::error,
                       xml_result.error->path.empty() ? "/" : xml_result.error->path,
                       "Malformed XML: " + xml_result.error->message);
        result.validation_status = ValidationStatus::error;
        return result;
    }

    const Node& root = xml_result.document->root;
    if (root.local_name != "CompositionPlaylist") {
        add_diagnostic(result.diagnostics,
                       "cpl.missing_required_field",
                       DiagnosticSeverity::error,
                       "/CompositionPlaylist",
                       "Root element must be CompositionPlaylist");
        result.validation_status = ValidationStatus::error;
        return result;
    }

    CompositionPlaylist composition_playlist;
    composition_playlist.namespace_uri = root.namespace_uri().value_or("");
    composition_playlist.schema_flavor = schema_flavor_for(composition_playlist.namespace_uri);
    if (composition_playlist.schema_flavor == SchemaFlavor::unknown) {
        add_diagnostic(result.diagnostics,
                       "cpl.unsupported_namespace",
                       DiagnosticSeverity::warning,
                       "/CompositionPlaylist",
                       "Unsupported namespace URI: " + composition_playlist.namespace_uri);
    }

    if (const auto value = required_text(root, "Id", "/CompositionPlaylist/Id", result.diagnostics); value.has_value()) {
        const auto normalized = normalized_uuid(*value);
        if (normalized.has_value()) {
            composition_playlist.composition_id = *normalized;
        } else {
            add_diagnostic(result.diagnostics,
                           "cpl.invalid_uuid",
                           DiagnosticSeverity::error,
                           "/CompositionPlaylist/Id",
                           "Invalid UUID value");
        }
    }

    if (const auto value =
            required_text(root, "ContentTitleText", "/CompositionPlaylist/ContentTitleText", result.diagnostics);
        value.has_value()) {
        composition_playlist.content_title_text = *value;
    }

    composition_playlist.annotation_text = optional_text(root, "AnnotationText");

    if (const auto value = required_text(root, "Issuer", "/CompositionPlaylist/Issuer", result.diagnostics); value.has_value()) {
        composition_playlist.issuer = *value;
    }

    composition_playlist.creator = optional_text(root, "Creator");
    composition_playlist.content_kind = optional_text(root, "ContentKind");

    if (const auto value = required_text(root, "IssueDate", "/CompositionPlaylist/IssueDate", result.diagnostics); value.has_value()) {
        if (is_valid_utc_timestamp(*value)) {
            composition_playlist.issue_date_utc = *value;
        } else {
            add_diagnostic(result.diagnostics,
                           "cpl.invalid_time",
                           DiagnosticSeverity::error,
                           "/CompositionPlaylist/IssueDate",
                           "Invalid UTC timestamp");
        }
    }

    if (const auto value = required_text(root, "EditRate", "/CompositionPlaylist/EditRate", result.diagnostics); value.has_value()) {
        const auto edit_rate = parse_edit_rate(*value);
        if (edit_rate.has_value()) {
            composition_playlist.edit_rate = *edit_rate;
        } else {
            add_diagnostic(result.diagnostics,
                           "cpl.invalid_edit_rate",
                           DiagnosticSeverity::error,
                           "/CompositionPlaylist/EditRate",
                           "EditRate must contain two positive integers");
        }
    }

    const Node* reel_list = xml::find_child(root, "ReelList");
    if (reel_list == nullptr) {
        add_diagnostic(result.diagnostics,
                       "cpl.missing_required_field",
                       DiagnosticSeverity::error,
                       "/CompositionPlaylist/ReelList",
                       "Missing required field");
    } else {
        std::unordered_set<std::string> reel_ids;
        const auto reel_nodes = xml::find_children(*reel_list, "Reel");
        if (reel_nodes.empty()) {
            add_diagnostic(result.diagnostics,
                           "cpl.empty_reel_list",
                           DiagnosticSeverity::error,
                           "/CompositionPlaylist/ReelList",
                           "CompositionPlaylist must contain at least one reel");
        } else {
            for (std::size_t reel_index = 0U; reel_index < reel_nodes.size(); ++reel_index) {
                const Node& reel_node = *reel_nodes[reel_index];
                const std::string reel_path = "/CompositionPlaylist/ReelList/Reel[" + std::to_string(reel_index + 1U) + "]";

                Reel reel;
                if (const auto value = required_text(reel_node, "Id", reel_path + "/Id", result.diagnostics); value.has_value()) {
                    const auto normalized = normalized_uuid(*value);
                    if (normalized.has_value()) {
                        reel.reel_id = *normalized;
                        if (!reel_ids.insert(*normalized).second) {
                            add_diagnostic(result.diagnostics,
                                           "cpl.duplicate_reel_id",
                                           DiagnosticSeverity::error,
                                           reel_path + "/Id",
                                           "Duplicate reel_id");
                        }
                    } else {
                        add_diagnostic(result.diagnostics,
                                       "cpl.invalid_uuid",
                                       DiagnosticSeverity::error,
                                       reel_path + "/Id",
                                       "Invalid UUID value");
                    }
                }

                const Node* asset_list = xml::find_child(reel_node, "AssetList");
                if (asset_list == nullptr) {
                    add_diagnostic(result.diagnostics,
                                   "cpl.missing_required_field",
                                   DiagnosticSeverity::error,
                                   reel_path + "/AssetList",
                                   "Missing required field");
                } else {
                    std::array<std::size_t, 3U> track_counts{0U, 0U, 0U};
                    for (const auto& child : asset_list->children) {
                        const auto track_type = track_type_for(child.local_name);
                        if (!track_type.has_value()) {
                            continue;
                        }

                        const std::size_t track_index = static_cast<std::size_t>(*track_type);
                        ++track_counts[track_index];

                        std::string track_path = reel_path + "/AssetList/" + child.local_name;
                        if (track_counts[track_index] > 1U) {
                            track_path += "[" + std::to_string(track_counts[track_index]) + "]";
                            add_diagnostic(result.diagnostics,
                                           "cpl.duplicate_track_type",
                                           DiagnosticSeverity::error,
                                           track_path,
                                           "Duplicate track type within reel");
                            continue;
                        }

                        parse_track_reference(child, track_path, *track_type, result.diagnostics, *slot_for(reel, *track_type));
                    }

                    if (!reel.picture.has_value() && !reel.sound.has_value() && !reel.subtitle.has_value()) {
                        add_diagnostic(result.diagnostics,
                                       "cpl.reel_missing_track",
                                       DiagnosticSeverity::error,
                                       reel_path + "/AssetList",
                                       "Reel must contain at least one supported track reference");
                    } else {
                        validate_duplicate_asset_references(reel, reel_path + "/AssetList", result.diagnostics);
                    }
                }

                composition_playlist.reels.push_back(std::move(reel));
            }
        }
    }

    sort_diagnostics(result.diagnostics);
    result.validation_status = validation_status_for(result.diagnostics);
    if (result.ok()) {
        result.composition_playlist = std::move(composition_playlist);
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

const char* to_string(TrackType track_type) noexcept {
    switch (track_type) {
        case TrackType::picture:
            return "picture";
        case TrackType::sound:
            return "sound";
        case TrackType::subtitle:
            return "subtitle";
    }
    return "picture";
}

}  // namespace dcplayer::dcp::cpl
