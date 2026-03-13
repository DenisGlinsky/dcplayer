#include "secure_channel.hpp"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace dcplayer::security_api::secure_channel {
namespace {

struct JsonValue {
    enum class Type {
        null_value,
        boolean,
        string,
        array,
        object,
    };

    Type type{Type::null_value};
    bool boolean_value{false};
    std::string string_value;
    std::vector<JsonValue> array_values;
    std::vector<std::pair<std::string, JsonValue>> object_values;
};

struct JsonParseError {
    std::string path;
    std::string message;
};

class JsonParser {
public:
    explicit JsonParser(std::string_view input)
        : input_(input) {}

    [[nodiscard]] std::optional<JsonValue> parse(JsonParseError& error) {
        skip_whitespace();
        auto value = parse_value("/", error);
        if (!value.has_value()) {
            return std::nullopt;
        }

        skip_whitespace();
        if (position_ != input_.size()) {
            error = JsonParseError{
                .path = "/",
                .message = "unexpected trailing content",
            };
            return std::nullopt;
        }

        return value;
    }

private:
    [[nodiscard]] std::optional<JsonValue> parse_value(std::string path, JsonParseError& error) {
        skip_whitespace();
        if (position_ >= input_.size()) {
            error = JsonParseError{
                .path = std::move(path),
                .message = "unexpected end of input",
            };
            return std::nullopt;
        }

        switch (input_[position_]) {
            case '{':
                return parse_object(std::move(path), error);
            case '[':
                return parse_array(std::move(path), error);
            case '"':
                return parse_string_value(std::move(path), error);
            case 't':
                return parse_literal("true", true, std::move(path), error);
            case 'f':
                return parse_literal("false", false, std::move(path), error);
            case 'n':
                return parse_null(std::move(path), error);
            default:
                error = JsonParseError{
                    .path = std::move(path),
                    .message = "expected JSON value",
                };
                return std::nullopt;
        }
    }

    [[nodiscard]] std::optional<JsonValue> parse_object(std::string path, JsonParseError& error) {
        JsonValue value;
        value.type = JsonValue::Type::object;

        ++position_;
        skip_whitespace();
        if (consume('}')) {
            return value;
        }

        std::size_t field_index = 0U;
        while (position_ < input_.size()) {
            skip_whitespace();
            const auto key = parse_string(path + "/@" + std::to_string(field_index + 1U), error);
            if (!key.has_value()) {
                return std::nullopt;
            }

            skip_whitespace();
            if (!consume(':')) {
                error = JsonParseError{
                    .path = path + "/" + *key,
                    .message = "expected ':' after object key",
                };
                return std::nullopt;
            }

            const std::string child_path = path == "/" ? "/" + *key : path + "/" + *key;
            auto child = parse_value(child_path, error);
            if (!child.has_value()) {
                return std::nullopt;
            }

            value.object_values.emplace_back(*key, std::move(*child));
            ++field_index;
            skip_whitespace();

            if (consume('}')) {
                return value;
            }

            if (!consume(',')) {
                error = JsonParseError{
                    .path = std::move(path),
                    .message = "expected ',' or '}' in object",
                };
                return std::nullopt;
            }
        }

        error = JsonParseError{
            .path = std::move(path),
            .message = "unterminated object",
        };
        return std::nullopt;
    }

    [[nodiscard]] std::optional<JsonValue> parse_array(std::string path, JsonParseError& error) {
        JsonValue value;
        value.type = JsonValue::Type::array;

        ++position_;
        skip_whitespace();
        if (consume(']')) {
            return value;
        }

        std::size_t index = 0U;
        while (position_ < input_.size()) {
            const std::string child_path = path + "[" + std::to_string(index + 1U) + "]";
            auto child = parse_value(child_path, error);
            if (!child.has_value()) {
                return std::nullopt;
            }

            value.array_values.push_back(std::move(*child));
            ++index;
            skip_whitespace();

            if (consume(']')) {
                return value;
            }

            if (!consume(',')) {
                error = JsonParseError{
                    .path = std::move(path),
                    .message = "expected ',' or ']' in array",
                };
                return std::nullopt;
            }
        }

        error = JsonParseError{
            .path = std::move(path),
            .message = "unterminated array",
        };
        return std::nullopt;
    }

    [[nodiscard]] std::optional<JsonValue> parse_string_value(std::string path, JsonParseError& error) {
        auto string_value = parse_string(path, error);
        if (!string_value.has_value()) {
            return std::nullopt;
        }

        JsonValue value;
        value.type = JsonValue::Type::string;
        value.string_value = std::move(*string_value);
        return value;
    }

    [[nodiscard]] std::optional<JsonValue> parse_literal(std::string_view literal,
                                                         bool value,
                                                         std::string path,
                                                         JsonParseError& error) {
        if (input_.substr(position_, literal.size()) != literal) {
            error = JsonParseError{
                .path = std::move(path),
                .message = "invalid JSON literal",
            };
            return std::nullopt;
        }

        position_ += literal.size();

        JsonValue parsed;
        parsed.type = JsonValue::Type::boolean;
        parsed.boolean_value = value;
        return parsed;
    }

    [[nodiscard]] std::optional<JsonValue> parse_null(std::string path, JsonParseError& error) {
        if (input_.substr(position_, 4U) != "null") {
            error = JsonParseError{
                .path = std::move(path),
                .message = "invalid JSON literal",
            };
            return std::nullopt;
        }

        position_ += 4U;
        return JsonValue{};
    }

    [[nodiscard]] std::optional<std::string> parse_string(std::string path, JsonParseError& error) {
        if (!consume('"')) {
            error = JsonParseError{
                .path = std::move(path),
                .message = "expected quoted string",
            };
            return std::nullopt;
        }

        std::string output;
        while (position_ < input_.size()) {
            const char ch = input_[position_++];
            if (ch == '"') {
                return output;
            }

            if (ch != '\\') {
                output.push_back(ch);
                continue;
            }

            if (position_ >= input_.size()) {
                error = JsonParseError{
                    .path = std::move(path),
                    .message = "unterminated escape sequence",
                };
                return std::nullopt;
            }

            const char escaped = input_[position_++];
            switch (escaped) {
                case '"':
                case '\\':
                case '/':
                    output.push_back(escaped);
                    break;
                case 'b':
                    output.push_back('\b');
                    break;
                case 'f':
                    output.push_back('\f');
                    break;
                case 'n':
                    output.push_back('\n');
                    break;
                case 'r':
                    output.push_back('\r');
                    break;
                case 't':
                    output.push_back('\t');
                    break;
                default:
                    error = JsonParseError{
                        .path = std::move(path),
                        .message = "unsupported escape sequence",
                    };
                    return std::nullopt;
            }
        }

        error = JsonParseError{
            .path = std::move(path),
            .message = "unterminated string",
        };
        return std::nullopt;
    }

    void skip_whitespace() {
        while (position_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[position_])) != 0) {
            ++position_;
        }
    }

    [[nodiscard]] bool consume(char expected) {
        if (position_ < input_.size() && input_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    std::string_view input_;
    std::size_t position_{0U};
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

[[nodiscard]] std::string lower_copy(std::string_view input) {
    std::string output;
    output.reserve(input.size());
    for (const char ch : input) {
        output.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return output;
}

[[nodiscard]] bool is_hex_string(std::string_view input) {
    return !input.empty() &&
           std::all_of(input.begin(), input.end(), [](char ch) { return std::isxdigit(static_cast<unsigned char>(ch)) != 0; });
}

[[nodiscard]] bool is_uuid_like(std::string_view input) {
    if (input.size() != 36U) {
        return false;
    }

    for (std::size_t index = 0U; index < input.size(); ++index) {
        const bool hyphen = index == 8U || index == 13U || index == 18U || index == 23U;
        if (hyphen) {
            if (input[index] != '-') {
                return false;
            }
            continue;
        }

        if (std::isxdigit(static_cast<unsigned char>(input[index])) == 0) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool is_utc_timestamp(std::string_view input) {
    if (input.size() != 20U) {
        return false;
    }

    const auto is_digit = [&](std::size_t index) {
        return std::isdigit(static_cast<unsigned char>(input[index])) != 0;
    };

    for (const std::size_t index : {0U, 1U, 2U, 3U, 5U, 6U, 8U, 9U, 11U, 12U, 14U, 15U, 17U, 18U}) {
        if (!is_digit(index)) {
            return false;
        }
    }

    return input[4] == '-' && input[7] == '-' && input[10] == 'T' && input[13] == ':' && input[16] == ':' &&
           input[19] == 'Z';
}

struct StringArrayEntry {
    std::string value;
    std::size_t item_index{0U};
};

template <typename T>
void sort_values(std::vector<T>& values) {
    std::sort(values.begin(), values.end());
}

constexpr std::string_view kBaselineSecureChannelId = "spb1.control.v1";
constexpr std::string_view kBaselineRequestEnvelopeRef = "protected_request.v1";
constexpr std::string_view kBaselineResponseEnvelopeRef = "protected_response.v1";

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

template <typename T>
[[nodiscard]] bool contains(const std::vector<T>& values, const T& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

[[nodiscard]] const JsonValue* find_field(const JsonValue& object, std::string_view name) {
    if (object.type != JsonValue::Type::object) {
        return nullptr;
    }

    for (const auto& [field_name, field_value] : object.object_values) {
        if (field_name == name) {
            return &field_value;
        }
    }

    return nullptr;
}

[[nodiscard]] std::optional<std::string> required_string_field(const JsonValue& object,
                                                               std::string_view field_name,
                                                               std::string_view path,
                                                               std::vector<Diagnostic>& diagnostics) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Required field is missing");
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::string) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must be a JSON string");
        return std::nullopt;
    }

    if (field->string_value.empty()) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_value",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must not be empty");
        return std::nullopt;
    }

    return field->string_value;
}

[[nodiscard]] std::optional<bool> required_bool_field(const JsonValue& object,
                                                      std::string_view field_name,
                                                      std::string_view path,
                                                      std::vector<Diagnostic>& diagnostics) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Required field is missing");
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::boolean) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must be a JSON boolean");
        return std::nullopt;
    }

    return field->boolean_value;
}

[[nodiscard]] std::optional<std::vector<StringArrayEntry>> required_string_array_entries(
    const JsonValue& object,
    std::string_view field_name,
    std::string_view path,
    std::vector<Diagnostic>& diagnostics,
    const char* duplicate_code = nullptr,
    std::string_view duplicate_message = "Array item duplicates an earlier entry") {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Required field is missing");
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::array) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must be a JSON array");
        return std::nullopt;
    }

    std::vector<StringArrayEntry> values;
    values.reserve(field->array_values.size());
    for (std::size_t index = 0U; index < field->array_values.size(); ++index) {
        const auto& item = field->array_values[index];
        const std::string item_path = std::string{path} + "[" + std::to_string(index + 1U) + "]";
        if (item.type != JsonValue::Type::string) {
            add_diagnostic(diagnostics,
                           "secure_channel.invalid_type",
                           DiagnosticSeverity::error,
                           item_path,
                           "Array item must be a JSON string");
            continue;
        }

        if (item.string_value.empty()) {
            add_diagnostic(diagnostics,
                           "secure_channel.invalid_value",
                           DiagnosticSeverity::error,
                           item_path,
                           "Array item must not be empty");
            continue;
        }

        values.push_back(StringArrayEntry{
            .value = item.string_value,
            .item_index = index,
        });
    }

    if (duplicate_code != nullptr) {
        std::map<std::string, std::size_t> first_seen;
        for (const auto& value : values) {
            if (!first_seen.emplace(value.value, value.item_index).second) {
                add_diagnostic(diagnostics,
                               duplicate_code,
                               DiagnosticSeverity::error,
                               std::string{path} + "[" + std::to_string(value.item_index + 1U) + "]",
                               std::string{duplicate_message});
            }
        }
    }

    if (values.empty()) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_value",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Array must not be empty");
        return std::nullopt;
    }

    return values;
}

[[nodiscard]] std::optional<std::vector<std::string>> required_string_array_field(
    const JsonValue& object,
    std::string_view field_name,
    std::string_view path,
    std::vector<Diagnostic>& diagnostics,
    const char* duplicate_code = nullptr,
    std::string_view duplicate_message = "Array item duplicates an earlier entry") {
    const auto entries =
        required_string_array_entries(object, field_name, path, diagnostics, duplicate_code, duplicate_message);
    if (!entries.has_value()) {
        return std::nullopt;
    }

    std::vector<std::string> values;
    values.reserve(entries->size());
    for (const auto& entry : *entries) {
        values.push_back(entry.value);
    }

    sort_values(values);
    return values;
}

[[nodiscard]] std::optional<std::map<std::string, std::string>> required_string_map_field(const JsonValue& object,
                                                                                           std::string_view field_name,
                                                                                           std::string_view path,
                                                                                           std::vector<Diagnostic>& diagnostics) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Required field is missing");
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must be a JSON object");
        return std::nullopt;
    }

    std::map<std::string, std::string> values;
    for (const auto& [key, item] : field->object_values) {
        const std::string item_path = std::string{path} + "/" + key;
        if (item.type != JsonValue::Type::string) {
            add_diagnostic(diagnostics,
                           "secure_channel.invalid_type",
                           DiagnosticSeverity::error,
                           item_path,
                           "Body value must be a JSON string");
            continue;
        }

        values.insert_or_assign(key, item.string_value);
    }

    return values;
}

[[nodiscard]] std::optional<PeerRole> parse_peer_role_string(std::string_view value) {
    if (value == "pi_zymkey") {
        return PeerRole::pi_zymkey;
    }
    if (value == "ubuntu_tpm") {
        return PeerRole::ubuntu_tpm;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<ApiName> parse_api_name_string(std::string_view value) {
    if (value == "sign") {
        return ApiName::sign;
    }
    if (value == "unwrap") {
        return ApiName::unwrap;
    }
    if (value == "decrypt") {
        return ApiName::decrypt;
    }
    if (value == "health") {
        return ApiName::health;
    }
    if (value == "identity") {
        return ApiName::identity;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<TrustDecision> parse_trust_decision_string(std::string_view value) {
    if (value == "trusted") {
        return TrustDecision::trusted;
    }
    if (value == "rejected") {
        return TrustDecision::rejected;
    }
    if (value == "warning_only") {
        return TrustDecision::warning_only;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<TrustDecisionReason> parse_trust_decision_reason_string(std::string_view value) {
    if (value == "ok") {
        return TrustDecisionReason::ok;
    }
    if (value == "anchor_missing") {
        return TrustDecisionReason::anchor_missing;
    }
    if (value == "expired") {
        return TrustDecisionReason::expired;
    }
    if (value == "not_yet_valid") {
        return TrustDecisionReason::not_yet_valid;
    }
    if (value == "revoked") {
        return TrustDecisionReason::revoked;
    }
    if (value == "chain_broken") {
        return TrustDecisionReason::chain_broken;
    }
    if (value == "algorithm_forbidden") {
        return TrustDecisionReason::algorithm_forbidden;
    }
    if (value == "revocation_unavailable") {
        return TrustDecisionReason::revocation_unavailable;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<RevocationStatus> parse_revocation_status_string(std::string_view value) {
    if (value == "good") {
        return RevocationStatus::good;
    }
    if (value == "revoked") {
        return RevocationStatus::revoked;
    }
    if (value == "unknown") {
        return RevocationStatus::unknown;
    }
    if (value == "not_checked") {
        return RevocationStatus::not_checked;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<EnvelopeStatus> parse_envelope_status_string(std::string_view value) {
    if (value == "ok") {
        return EnvelopeStatus::ok;
    }
    if (value == "denied") {
        return EnvelopeStatus::denied;
    }
    if (value == "error") {
        return EnvelopeStatus::error;
    }
    return std::nullopt;
}

template <typename Enum>
[[nodiscard]] std::optional<Enum> parse_required_enum(const JsonValue& object,
                                                      std::string_view field_name,
                                                      std::string_view path,
                                                      std::vector<Diagnostic>& diagnostics,
                                                      const char* invalid_code,
                                                      const auto& parser) {
    const auto value = required_string_field(object, field_name, path, diagnostics);
    if (!value.has_value()) {
        return std::nullopt;
    }

    const auto parsed = parser(*value);
    if (!parsed.has_value()) {
        add_diagnostic(diagnostics,
                       invalid_code,
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field has an unsupported enum value");
        return std::nullopt;
    }

    return parsed;
}

template <typename Enum>
[[nodiscard]] std::optional<std::vector<Enum>> parse_required_enum_array(const JsonValue& object,
                                                                         std::string_view field_name,
                                                                         std::string_view path,
                                                                         std::vector<Diagnostic>& diagnostics,
                                                                         const char* invalid_code,
                                                                         const auto& parser,
                                                                         const char* duplicate_code = nullptr,
                                                                         std::string_view duplicate_message =
                                                                             "Array item duplicates an earlier entry") {
    const auto strings = required_string_array_entries(object, field_name, path, diagnostics);
    if (!strings.has_value()) {
        return std::nullopt;
    }

    std::vector<Enum> values;
    values.reserve(strings->size());
    std::map<Enum, std::size_t> first_seen;
    for (const auto& string_entry : *strings) {
        const auto parsed = parser(string_entry.value);
        if (!parsed.has_value()) {
            add_diagnostic(diagnostics,
                           invalid_code,
                           DiagnosticSeverity::error,
                           std::string{path} + "[" + std::to_string(string_entry.item_index + 1U) + "]",
                           "Array item has an unsupported enum value");
            continue;
        }

        if (duplicate_code != nullptr && !first_seen.emplace(*parsed, string_entry.item_index).second) {
            add_diagnostic(diagnostics,
                           duplicate_code,
                           DiagnosticSeverity::error,
                           std::string{path} + "[" + std::to_string(string_entry.item_index + 1U) + "]",
                           std::string{duplicate_message});
        }

        values.push_back(*parsed);
    }

    if (values.empty()) {
        return std::nullopt;
    }

    sort_values(values);
    return values;
}

[[nodiscard]] std::optional<PeerIdentity> parse_peer_identity(const JsonValue& object,
                                                              std::string_view path,
                                                              std::vector<Diagnostic>& diagnostics) {
    if (object.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Identity must be a JSON object");
        return std::nullopt;
    }

    const auto role = parse_required_enum<PeerRole>(object,
                                                    "role",
                                                    std::string{path} + "/role",
                                                    diagnostics,
                                                    "secure_channel.invalid_role",
                                                    parse_peer_role_string);
    const auto device_id = required_string_field(object, "device_id", std::string{path} + "/device_id", diagnostics);
    auto fingerprint = required_string_field(object,
                                             "certificate_fingerprint",
                                             std::string{path} + "/certificate_fingerprint",
                                             diagnostics);
    const auto subject_dn = required_string_field(object, "subject_dn", std::string{path} + "/subject_dn", diagnostics);
    const auto san_dns_names =
        required_string_array_field(object,
                                    "san_dns_names",
                                    std::string{path} + "/san_dns_names",
                                    diagnostics,
                                    "secure_channel.duplicate_san_entry",
                                    "SAN entry duplicates an earlier entry");
    const auto san_uri_names =
        required_string_array_field(object,
                                    "san_uri_names",
                                    std::string{path} + "/san_uri_names",
                                    diagnostics,
                                    "secure_channel.duplicate_san_entry",
                                    "SAN entry duplicates an earlier entry");

    if (fingerprint.has_value()) {
        *fingerprint = lower_copy(*fingerprint);
        if (!is_hex_string(*fingerprint)) {
            add_diagnostic(diagnostics,
                           "secure_channel.invalid_fingerprint",
                           DiagnosticSeverity::error,
                           std::string{path} + "/certificate_fingerprint",
                           "Fingerprint must be a lowercase hex string");
            fingerprint.reset();
        }
    }

    if (!role.has_value() || !device_id.has_value() || !fingerprint.has_value() || !subject_dn.has_value() ||
        !san_dns_names.has_value() || !san_uri_names.has_value()) {
        return std::nullopt;
    }

    return PeerIdentity{
        .role = *role,
        .device_id = *device_id,
        .certificate_fingerprint = *fingerprint,
        .subject_dn = *subject_dn,
        .san_dns_names = *san_dns_names,
        .san_uri_names = *san_uri_names,
    };
}

[[nodiscard]] std::optional<TrustRequirements> parse_trust_requirements(const JsonValue& object,
                                                                        std::string_view path,
                                                                        std::vector<Diagnostic>& diagnostics) {
    if (object.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Trust requirements must be a JSON object");
        return std::nullopt;
    }

    const auto certificate_store_ref =
        required_string_field(object, "certificate_store_ref", std::string{path} + "/certificate_store_ref", diagnostics);
    const auto required_decision =
        parse_required_enum<TrustDecision>(object,
                                           "required_decision",
                                           std::string{path} + "/required_decision",
                                           diagnostics,
                                           "secure_channel.invalid_trust_decision",
                                           parse_trust_decision_string);
    const auto required_decision_reason =
        parse_required_enum<TrustDecisionReason>(object,
                                                 "required_decision_reason",
                                                 std::string{path} + "/required_decision_reason",
                                                 diagnostics,
                                                 "secure_channel.invalid_trust_reason",
                                                 parse_trust_decision_reason_string);
    const auto accepted_revocation_statuses =
        parse_required_enum_array<RevocationStatus>(object,
                                                    "accepted_revocation_statuses",
                                                    std::string{path} + "/accepted_revocation_statuses",
                                                    diagnostics,
                                                    "secure_channel.invalid_revocation_status",
                                                    parse_revocation_status_string,
                                                    "secure_channel.duplicate_revocation_status",
                                                    "Revocation status duplicates an earlier entry");
    const auto required_checked_sources = required_string_array_field(object,
                                                                      "required_checked_sources",
                                                                      std::string{path} + "/required_checked_sources",
                                                                      diagnostics,
                                                                      "secure_channel.duplicate_checked_source",
                                                                      "Checked source duplicates an earlier entry");

    if (!certificate_store_ref.has_value() || !required_decision.has_value() || !required_decision_reason.has_value() ||
        !accepted_revocation_statuses.has_value() || !required_checked_sources.has_value()) {
        return std::nullopt;
    }

    return TrustRequirements{
        .certificate_store_ref = *certificate_store_ref,
        .required_decision = *required_decision,
        .required_decision_reason = *required_decision_reason,
        .accepted_revocation_statuses = *accepted_revocation_statuses,
        .required_checked_sources = *required_checked_sources,
    };
}

[[nodiscard]] std::optional<TrustSummary> parse_trust_summary(const JsonValue& object,
                                                              std::string_view path,
                                                              std::vector<Diagnostic>& diagnostics) {
    if (object.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Trust summary must be a JSON object");
        return std::nullopt;
    }

    auto subject_fingerprint =
        required_string_field(object, "subject_fingerprint", std::string{path} + "/subject_fingerprint", diagnostics);
    const auto issuer_chain =
        required_string_array_field(object, "issuer_chain", std::string{path} + "/issuer_chain", diagnostics);
    const auto validation_time_utc =
        required_string_field(object, "validation_time_utc", std::string{path} + "/validation_time_utc", diagnostics);
    const auto revocation_status =
        parse_required_enum<RevocationStatus>(object,
                                              "revocation_status",
                                              std::string{path} + "/revocation_status",
                                              diagnostics,
                                              "secure_channel.invalid_revocation_status",
                                              parse_revocation_status_string);
    const auto decision = parse_required_enum<TrustDecision>(object,
                                                             "decision",
                                                             std::string{path} + "/decision",
                                                             diagnostics,
                                                             "secure_channel.invalid_trust_decision",
                                                             parse_trust_decision_string);
    const auto decision_reason =
        parse_required_enum<TrustDecisionReason>(object,
                                                 "decision_reason",
                                                 std::string{path} + "/decision_reason",
                                                 diagnostics,
                                                 "secure_channel.invalid_trust_reason",
                                                 parse_trust_decision_reason_string);
    const auto checked_sources =
        required_string_array_field(object,
                                    "checked_sources",
                                    std::string{path} + "/checked_sources",
                                    diagnostics,
                                    "secure_channel.duplicate_checked_source",
                                    "Checked source duplicates an earlier entry");

    if (subject_fingerprint.has_value()) {
        *subject_fingerprint = lower_copy(*subject_fingerprint);
        if (!is_hex_string(*subject_fingerprint)) {
            add_diagnostic(diagnostics,
                           "secure_channel.invalid_fingerprint",
                           DiagnosticSeverity::error,
                           std::string{path} + "/subject_fingerprint",
                           "Fingerprint must be a lowercase hex string");
            subject_fingerprint.reset();
        }
    }

    if (validation_time_utc.has_value() && !is_utc_timestamp(*validation_time_utc)) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_time",
                       DiagnosticSeverity::error,
                       std::string{path} + "/validation_time_utc",
                       "validation_time_utc must use UTC ISO-8601");
    }

    if (!subject_fingerprint.has_value() || !issuer_chain.has_value() || !validation_time_utc.has_value() ||
        !revocation_status.has_value() || !decision.has_value() || !decision_reason.has_value() ||
        !checked_sources.has_value() || !is_utc_timestamp(validation_time_utc.value_or(""))) {
        return std::nullopt;
    }

    return TrustSummary{
        .subject_fingerprint = *subject_fingerprint,
        .issuer_chain = *issuer_chain,
        .validation_time_utc = *validation_time_utc,
        .revocation_status = *revocation_status,
        .decision = *decision,
        .decision_reason = *decision_reason,
        .checked_sources = *checked_sources,
    };
}

[[nodiscard]] std::optional<AuthContextSummary> parse_auth_context_summary(const JsonValue& object,
                                                                           std::string_view path,
                                                                           std::vector<Diagnostic>& diagnostics) {
    if (object.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Auth context must be a JSON object");
        return std::nullopt;
    }

    const auto channel_id = required_string_field(object, "channel_id", std::string{path} + "/channel_id", diagnostics);
    const auto mutual_tls = required_bool_field(object, "mutual_tls", std::string{path} + "/mutual_tls", diagnostics);
    const auto local_role = parse_required_enum<PeerRole>(object,
                                                          "local_role",
                                                          std::string{path} + "/local_role",
                                                          diagnostics,
                                                          "secure_channel.invalid_role",
                                                          parse_peer_role_string);
    const auto* peer_trust_value = find_field(object, "peer_trust");
    if (peer_trust_value == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       std::string{path} + "/peer_trust",
                       "Required field is missing");
    }
    const auto checked_sources =
        required_string_array_field(object,
                                    "checked_sources",
                                    std::string{path} + "/checked_sources",
                                    diagnostics,
                                    "secure_channel.duplicate_checked_source",
                                    "Checked source duplicates an earlier entry");

    std::optional<TrustSummary> peer_trust;
    if (peer_trust_value != nullptr) {
        peer_trust = parse_trust_summary(*peer_trust_value, std::string{path} + "/peer_trust", diagnostics);
    }

    if (mutual_tls.has_value() && !*mutual_tls) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_channel_binding",
                       DiagnosticSeverity::error,
                       std::string{path} + "/mutual_tls",
                       "mutual_tls must be true");
    }

    if (!channel_id.has_value() || !mutual_tls.has_value() || !local_role.has_value() || !peer_trust.has_value() ||
        !checked_sources.has_value() || !*mutual_tls) {
        return std::nullopt;
    }

    return AuthContextSummary{
        .channel_id = *channel_id,
        .mutual_tls = *mutual_tls,
        .local_role = *local_role,
        .peer_trust = *peer_trust,
        .checked_sources = *checked_sources,
    };
}

[[nodiscard]] std::optional<PayloadContract> parse_payload_contract(const JsonValue& object,
                                                                    std::string_view path,
                                                                    std::vector<Diagnostic>& diagnostics) {
    if (object.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Payload must be a JSON object");
        return std::nullopt;
    }

    const auto payload_type =
        required_string_field(object, "payload_type", std::string{path} + "/payload_type", diagnostics);
    const auto schema_ref = required_string_field(object, "schema_ref", std::string{path} + "/schema_ref", diagnostics);
    const auto body = required_string_map_field(object, "body", std::string{path} + "/body", diagnostics);

    if (!payload_type.has_value() || !schema_ref.has_value() || !body.has_value()) {
        return std::nullopt;
    }

    return PayloadContract{
        .payload_type = *payload_type,
        .schema_ref = *schema_ref,
        .body = *body,
    };
}

[[nodiscard]] std::optional<AclRule> parse_acl_rule(const JsonValue& object,
                                                    std::string_view path,
                                                    std::vector<Diagnostic>& diagnostics) {
    if (object.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "ACL rule must be a JSON object");
        return std::nullopt;
    }

    const auto caller_role = parse_required_enum<PeerRole>(object,
                                                           "caller_role",
                                                           std::string{path} + "/caller_role",
                                                           diagnostics,
                                                           "secure_channel.invalid_role",
                                                           parse_peer_role_string);
    const auto allowed_api_names = parse_required_enum_array<ApiName>(object,
                                                                      "allowed_api_names",
                                                                      std::string{path} + "/allowed_api_names",
                                                                      diagnostics,
                                                                      "secure_channel.invalid_api_name",
                                                                      parse_api_name_string,
                                                                      "secure_channel.duplicate_allowed_api_name",
                                                                      "API name duplicates an earlier entry");

    if (!caller_role.has_value() || !allowed_api_names.has_value()) {
        return std::nullopt;
    }

    return AclRule{
        .caller_role = *caller_role,
        .allowed_api_names = *allowed_api_names,
    };
}

[[nodiscard]] std::optional<std::vector<AclRule>> parse_acl_rules(const JsonValue& object,
                                                                  std::string_view path,
                                                                  std::vector<Diagnostic>& diagnostics) {
    const auto* field = find_field(object, "acl");
    if (field == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Required field is missing");
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::array) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "ACL must be a JSON array");
        return std::nullopt;
    }

    std::vector<AclRule> rules;
    rules.reserve(field->array_values.size());
    std::map<PeerRole, std::size_t> first_seen_roles;
    for (std::size_t index = 0U; index < field->array_values.size(); ++index) {
        const auto parsed = parse_acl_rule(field->array_values[index],
                                           std::string{path} + "[" + std::to_string(index + 1U) + "]",
                                           diagnostics);
        if (parsed.has_value()) {
            if (!first_seen_roles.emplace(parsed->caller_role, index).second) {
                add_diagnostic(diagnostics,
                               "secure_channel.invalid_acl_binding",
                               DiagnosticSeverity::error,
                               std::string{path} + "[" + std::to_string(index + 1U) + "]/caller_role",
                               "ACL caller_role duplicates an earlier rule");
            }
            rules.push_back(*parsed);
        }
    }

    if (rules.empty()) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_acl_binding",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "ACL must contain at least one rule");
        return std::nullopt;
    }

    std::sort(rules.begin(), rules.end(), [](const AclRule& lhs, const AclRule& rhs) {
        return std::tuple{lhs.caller_role, lhs.allowed_api_names} < std::tuple{rhs.caller_role, rhs.allowed_api_names};
    });
    return rules;
}

enum class PayloadFamily {
    sign_request,
    unwrap_request,
    decrypt_request,
    health_request,
    identity_request,
    sign_response,
    unwrap_response,
    decrypt_response,
    health_response,
    identity_response,
    error_response,
};

[[nodiscard]] PayloadFamily expected_request_payload_family(ApiName api_name) noexcept {
    switch (api_name) {
        case ApiName::sign:
            return PayloadFamily::sign_request;
        case ApiName::unwrap:
            return PayloadFamily::unwrap_request;
        case ApiName::decrypt:
            return PayloadFamily::decrypt_request;
        case ApiName::health:
            return PayloadFamily::health_request;
        case ApiName::identity:
            return PayloadFamily::identity_request;
    }
    return PayloadFamily::health_request;
}

[[nodiscard]] const char* schema_ref_for(PayloadFamily family) noexcept {
    switch (family) {
        case PayloadFamily::sign_request:
            return "spb1.sign.request.v1";
        case PayloadFamily::unwrap_request:
            return "spb1.unwrap.request.v1";
        case PayloadFamily::decrypt_request:
            return "spb1.decrypt.request.v1";
        case PayloadFamily::health_request:
            return "spb1.health.request.v1";
        case PayloadFamily::identity_request:
            return "spb1.identity.request.v1";
        case PayloadFamily::sign_response:
            return "spb1.sign.response.v1";
        case PayloadFamily::unwrap_response:
            return "spb1.unwrap.response.v1";
        case PayloadFamily::decrypt_response:
            return "spb1.decrypt.response.v1";
        case PayloadFamily::health_response:
            return "spb1.health.response.v1";
        case PayloadFamily::identity_response:
            return "spb1.identity.response.v1";
        case PayloadFamily::error_response:
            return "spb1.error.response.v1";
    }
    return "spb1.error.response.v1";
}

[[nodiscard]] std::optional<PayloadFamily> parse_payload_family(std::string_view payload_type) noexcept {
    if (payload_type == "sign_request") {
        return PayloadFamily::sign_request;
    }
    if (payload_type == "unwrap_request") {
        return PayloadFamily::unwrap_request;
    }
    if (payload_type == "decrypt_request") {
        return PayloadFamily::decrypt_request;
    }
    if (payload_type == "health_request") {
        return PayloadFamily::health_request;
    }
    if (payload_type == "identity_request") {
        return PayloadFamily::identity_request;
    }
    if (payload_type == "sign_response") {
        return PayloadFamily::sign_response;
    }
    if (payload_type == "unwrap_response") {
        return PayloadFamily::unwrap_response;
    }
    if (payload_type == "decrypt_response") {
        return PayloadFamily::decrypt_response;
    }
    if (payload_type == "health_response") {
        return PayloadFamily::health_response;
    }
    if (payload_type == "identity_response") {
        return PayloadFamily::identity_response;
    }
    if (payload_type == "error_response") {
        return PayloadFamily::error_response;
    }
    return std::nullopt;
}

[[nodiscard]] bool is_request_payload_family(PayloadFamily family) noexcept {
    switch (family) {
        case PayloadFamily::sign_request:
        case PayloadFamily::unwrap_request:
        case PayloadFamily::decrypt_request:
        case PayloadFamily::health_request:
        case PayloadFamily::identity_request:
            return true;
        case PayloadFamily::sign_response:
        case PayloadFamily::unwrap_response:
        case PayloadFamily::decrypt_response:
        case PayloadFamily::health_response:
        case PayloadFamily::identity_response:
        case PayloadFamily::error_response:
            return false;
    }
    return false;
}

[[nodiscard]] std::optional<ApiName> api_name_for_payload_family(PayloadFamily family) noexcept {
    switch (family) {
        case PayloadFamily::sign_request:
        case PayloadFamily::sign_response:
            return ApiName::sign;
        case PayloadFamily::unwrap_request:
        case PayloadFamily::unwrap_response:
            return ApiName::unwrap;
        case PayloadFamily::decrypt_request:
        case PayloadFamily::decrypt_response:
            return ApiName::decrypt;
        case PayloadFamily::health_request:
        case PayloadFamily::health_response:
            return ApiName::health;
        case PayloadFamily::identity_request:
        case PayloadFamily::identity_response:
            return ApiName::identity;
        case PayloadFamily::error_response:
            return std::nullopt;
    }
    return std::nullopt;
}

[[nodiscard]] const char* expected_request_schema_ref(ApiName api_name) noexcept {
    return schema_ref_for(expected_request_payload_family(api_name));
}

[[nodiscard]] std::string* find_body_field(PayloadContract& payload, std::string_view field_name) {
    auto it = payload.body.find(std::string{field_name});
    if (it == payload.body.end()) {
        return nullptr;
    }
    return &it->second;
}

[[nodiscard]] bool body_field_present(const PayloadContract& payload, std::string_view field_name) {
    return payload.body.find(std::string{field_name}) != payload.body.end();
}

void add_missing_required_field(std::vector<Diagnostic>& diagnostics,
                                std::string_view body_path,
                                std::string_view field_name) {
    add_diagnostic(diagnostics,
                   "secure_channel.missing_required_field",
                   DiagnosticSeverity::error,
                   std::string{body_path} + "/" + std::string{field_name},
                   "Required body field is missing");
}

void add_unexpected_field(std::vector<Diagnostic>& diagnostics, std::string_view body_path, std::string_view field_name) {
    add_diagnostic(diagnostics,
                   "secure_channel.unexpected_field",
                   DiagnosticSeverity::error,
                   std::string{body_path} + "/" + std::string{field_name},
                   "Field is not allowed for payload family");
}

void add_invalid_body_field(std::vector<Diagnostic>& diagnostics,
                            const char* code,
                            std::string_view body_path,
                            std::string_view field_name,
                            std::string_view message) {
    add_diagnostic(diagnostics,
                   code,
                   DiagnosticSeverity::error,
                   std::string{body_path} + "/" + std::string{field_name},
                   std::string{message});
}

void validate_body_shape(PayloadContract& payload,
                         std::string_view body_path,
                         std::initializer_list<std::string_view> required_fields,
                         std::initializer_list<std::string_view> optional_fields,
                         std::vector<Diagnostic>& diagnostics) {
    for (const auto field_name : required_fields) {
        if (!body_field_present(payload, field_name)) {
            add_missing_required_field(diagnostics, body_path, field_name);
        }
    }

    const auto is_allowed_field = [&](std::string_view field_name) {
        return std::find(required_fields.begin(), required_fields.end(), field_name) != required_fields.end() ||
               std::find(optional_fields.begin(), optional_fields.end(), field_name) != optional_fields.end();
    };

    for (const auto& [field_name, field_value] : payload.body) {
        static_cast<void>(field_value);
        if (!is_allowed_field(field_name)) {
            add_unexpected_field(diagnostics, body_path, field_name);
        }
    }
}

void validate_non_empty_body_field(PayloadContract& payload,
                                   std::string_view body_path,
                                   std::string_view field_name,
                                   const char* invalid_code,
                                   std::vector<Diagnostic>& diagnostics,
                                   bool normalize_lower = false) {
    auto* value = find_body_field(payload, field_name);
    if (value == nullptr) {
        return;
    }

    if (normalize_lower) {
        *value = lower_copy(*value);
    }

    if (value->empty()) {
        add_invalid_body_field(diagnostics, invalid_code, body_path, field_name, "Body field must not be empty");
    }
}

void validate_hex_body_field(PayloadContract& payload,
                             std::string_view body_path,
                             std::string_view field_name,
                             const char* invalid_code,
                             std::vector<Diagnostic>& diagnostics) {
    auto* value = find_body_field(payload, field_name);
    if (value == nullptr) {
        return;
    }

    *value = lower_copy(*value);
    if (value->empty()) {
        add_invalid_body_field(diagnostics, invalid_code, body_path, field_name, "Body field must not be empty");
        return;
    }

    if (!is_hex_string(*value)) {
        add_invalid_body_field(diagnostics, invalid_code, body_path, field_name, "Body field must be a lowercase hex string");
    }
}

void validate_role_body_field(PayloadContract& payload,
                              std::string_view body_path,
                              std::string_view field_name,
                              PeerRole expected_role,
                              const char* invalid_code,
                              std::vector<Diagnostic>& diagnostics) {
    auto* value = find_body_field(payload, field_name);
    if (value == nullptr) {
        return;
    }

    *value = lower_copy(*value);
    if (value->empty()) {
        add_invalid_body_field(diagnostics, invalid_code, body_path, field_name, "Body field must not be empty");
        return;
    }

    const auto parsed = parse_peer_role_string(*value);
    if (!parsed.has_value()) {
        add_invalid_body_field(diagnostics, invalid_code, body_path, field_name, "Body field must be a supported peer role");
        return;
    }

    if (*parsed != expected_role) {
        add_invalid_body_field(diagnostics, invalid_code, body_path, field_name, "Body field must match the baseline module role");
    }
}

void validate_health_status_field(PayloadContract& payload,
                                  std::string_view body_path,
                                  std::string_view field_name,
                                  const char* invalid_code,
                                  std::vector<Diagnostic>& diagnostics) {
    auto* value = find_body_field(payload, field_name);
    if (value == nullptr) {
        return;
    }

    *value = lower_copy(*value);
    if (value->empty()) {
        add_invalid_body_field(diagnostics, invalid_code, body_path, field_name, "Body field must not be empty");
        return;
    }

    if (*value != "ok" && *value != "degraded" && *value != "maintenance") {
        add_invalid_body_field(diagnostics, invalid_code, body_path, field_name, "Body field must use a supported health status");
    }
}

void validate_payload_body(PayloadFamily family,
                           PayloadContract& payload,
                           std::string_view path,
                           std::vector<Diagnostic>& diagnostics) {
    const std::string body_path = std::string{path} + "/body";

    switch (family) {
        case PayloadFamily::sign_request:
            validate_body_shape(payload, body_path, {"manifest_hash", "metadata_summary"}, {"key_slot"}, diagnostics);
            validate_non_empty_body_field(payload, body_path, "manifest_hash", "secure_channel.invalid_request_body", diagnostics, true);
            validate_non_empty_body_field(payload, body_path, "metadata_summary", "secure_channel.invalid_request_body", diagnostics);
            validate_non_empty_body_field(payload, body_path, "key_slot", "secure_channel.invalid_request_body", diagnostics);
            return;
        case PayloadFamily::unwrap_request:
            validate_body_shape(payload, body_path, {"wrapped_key_ref", "wrapping_key_slot"}, {"unwrap_context"}, diagnostics);
            validate_non_empty_body_field(payload, body_path, "wrapped_key_ref", "secure_channel.invalid_request_body", diagnostics);
            validate_non_empty_body_field(payload, body_path, "wrapping_key_slot", "secure_channel.invalid_request_body", diagnostics);
            validate_non_empty_body_field(payload, body_path, "unwrap_context", "secure_channel.invalid_request_body", diagnostics);
            return;
        case PayloadFamily::decrypt_request:
            validate_body_shape(payload, body_path, {"ciphertext_ref", "context_summary"}, {"output_handle"}, diagnostics);
            validate_non_empty_body_field(payload, body_path, "ciphertext_ref", "secure_channel.invalid_request_body", diagnostics);
            validate_non_empty_body_field(payload, body_path, "context_summary", "secure_channel.invalid_request_body", diagnostics);
            validate_non_empty_body_field(payload, body_path, "output_handle", "secure_channel.invalid_request_body", diagnostics);
            return;
        case PayloadFamily::health_request:
        case PayloadFamily::identity_request:
            validate_body_shape(payload, body_path, {}, {}, diagnostics);
            return;
        case PayloadFamily::sign_response:
            validate_body_shape(payload, body_path, {"signature", "pubkey"}, {"key_fingerprint"}, diagnostics);
            validate_non_empty_body_field(payload, body_path, "signature", "secure_channel.invalid_response_body", diagnostics);
            validate_non_empty_body_field(payload, body_path, "pubkey", "secure_channel.invalid_response_body", diagnostics);
            validate_hex_body_field(payload, body_path, "key_fingerprint", "secure_channel.invalid_response_body", diagnostics);
            return;
        case PayloadFamily::unwrap_response:
            validate_body_shape(payload, body_path, {"key_handle"}, {"unwrap_receipt"}, diagnostics);
            validate_non_empty_body_field(payload, body_path, "key_handle", "secure_channel.invalid_response_body", diagnostics);
            validate_non_empty_body_field(payload, body_path, "unwrap_receipt", "secure_channel.invalid_response_body", diagnostics);
            return;
        case PayloadFamily::decrypt_response:
            validate_body_shape(payload, body_path, {"result_handle"}, {"operation_note"}, diagnostics);
            validate_non_empty_body_field(payload, body_path, "result_handle", "secure_channel.invalid_response_body", diagnostics);
            validate_non_empty_body_field(payload, body_path, "operation_note", "secure_channel.invalid_response_body", diagnostics);
            return;
        case PayloadFamily::health_response:
            validate_body_shape(payload, body_path, {"module_status"}, {"status_summary"}, diagnostics);
            validate_health_status_field(payload, body_path, "module_status", "secure_channel.invalid_response_body", diagnostics);
            validate_non_empty_body_field(payload, body_path, "status_summary", "secure_channel.invalid_response_body", diagnostics);
            return;
        case PayloadFamily::identity_response:
            validate_body_shape(payload,
                                body_path,
                                {"module_id", "module_role", "certificate_fingerprint"},
                                {"subject_dn"},
                                diagnostics);
            validate_non_empty_body_field(payload, body_path, "module_id", "secure_channel.invalid_response_body", diagnostics);
            validate_role_body_field(payload,
                                     body_path,
                                     "module_role",
                                     PeerRole::pi_zymkey,
                                     "secure_channel.invalid_response_body",
                                     diagnostics);
            validate_hex_body_field(payload,
                                    body_path,
                                    "certificate_fingerprint",
                                    "secure_channel.invalid_response_body",
                                    diagnostics);
            validate_non_empty_body_field(payload, body_path, "subject_dn", "secure_channel.invalid_response_body", diagnostics);
            return;
        case PayloadFamily::error_response:
            validate_body_shape(payload, body_path, {"error_code", "error_summary"}, {}, diagnostics);
            validate_non_empty_body_field(payload, body_path, "error_code", "secure_channel.invalid_response_body", diagnostics, true);
            validate_non_empty_body_field(payload, body_path, "error_summary", "secure_channel.invalid_response_body", diagnostics);
            return;
    }
}

[[nodiscard]] std::optional<PayloadFamily> validate_request_payload_contract(ApiName api_name,
                                                                             PayloadContract& payload,
                                                                             std::string_view path,
                                                                             std::vector<Diagnostic>& diagnostics) {
    const auto expected_family = expected_request_payload_family(api_name);
    const auto actual_family = parse_payload_family(payload.payload_type);
    if (!actual_family.has_value() || !is_request_payload_family(*actual_family)) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_payload_contract",
                       DiagnosticSeverity::error,
                       std::string{path} + "/payload_type",
                       "payload_type must reference a request payload family");
        return std::nullopt;
    }

    if (*actual_family != expected_family) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_payload_contract",
                       DiagnosticSeverity::error,
                       std::string{path} + "/payload_type",
                       "payload_type must match api_name");
        return std::nullopt;
    }

    if (payload.schema_ref != expected_request_schema_ref(api_name)) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_payload_contract",
                       DiagnosticSeverity::error,
                       std::string{path} + "/schema_ref",
                       "schema_ref must match payload_type");
        return std::nullopt;
    }

    validate_payload_body(*actual_family, payload, path, diagnostics);
    return actual_family;
}

[[nodiscard]] std::optional<PayloadFamily> validate_response_payload_contract(PayloadContract& payload,
                                                                              std::optional<EnvelopeStatus> status,
                                                                              bool enforce_status_body_pair,
                                                                              std::string_view path,
                                                                              std::vector<Diagnostic>& diagnostics) {
    const auto actual_family = parse_payload_family(payload.payload_type);
    if (!actual_family.has_value() || is_request_payload_family(*actual_family)) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_payload_contract",
                       DiagnosticSeverity::error,
                       std::string{path} + "/payload_type",
                       "payload_type must reference a response payload family");
        return std::nullopt;
    }

    if (payload.schema_ref != schema_ref_for(*actual_family)) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_payload_contract",
                       DiagnosticSeverity::error,
                       std::string{path} + "/schema_ref",
                       "schema_ref must match payload_type");
        return std::nullopt;
    }

    if (enforce_status_body_pair) {
        if (status.has_value() && *status == EnvelopeStatus::ok && *actual_family == PayloadFamily::error_response) {
            add_diagnostic(diagnostics,
                           "secure_channel.invalid_status_body_combination",
                           DiagnosticSeverity::error,
                           std::string{path} + "/payload_type",
                           "status=ok must use an api-specific response family");
        }

        if (status.has_value() && *status != EnvelopeStatus::ok && *actual_family != PayloadFamily::error_response) {
            add_diagnostic(diagnostics,
                           "secure_channel.invalid_status_body_combination",
                           DiagnosticSeverity::error,
                           std::string{path} + "/payload_type",
                           "denied or error status must use error_response");
        }
    }

    validate_payload_body(*actual_family, payload, path, diagnostics);
    return actual_family;
}

void normalize_peer_identity(PeerIdentity& identity) {
    sort_values(identity.san_dns_names);
    sort_values(identity.san_uri_names);
    identity.certificate_fingerprint = lower_copy(identity.certificate_fingerprint);
}

template <typename T>
[[nodiscard]] ParseResult<T> make_parse_result(std::optional<T> value, std::vector<Diagnostic> diagnostics) {
    sort_diagnostics(diagnostics);
    const auto status = validation_status_for(diagnostics);
    if (status == ValidationStatus::error) {
        value.reset();
    }
    return ParseResult<T>{
        .value = std::move(value),
        .diagnostics = std::move(diagnostics),
        .validation_status = status,
    };
}

[[nodiscard]] std::optional<SecureChannelContract> build_secure_channel_contract(const JsonValue& root,
                                                                                 std::vector<Diagnostic>& diagnostics) {
    if (root.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       "/",
                       "SecureChannelContract must be a JSON object");
        return std::nullopt;
    }

    const auto channel_id = required_string_field(root, "channel_id", "/channel_id", diagnostics);
    const auto server_role = parse_required_enum<PeerRole>(root,
                                                           "server_role",
                                                           "/server_role",
                                                           diagnostics,
                                                           "secure_channel.invalid_role",
                                                           parse_peer_role_string);
    const auto client_role = parse_required_enum<PeerRole>(root,
                                                           "client_role",
                                                           "/client_role",
                                                           diagnostics,
                                                           "secure_channel.invalid_role",
                                                           parse_peer_role_string);
    const auto tls_profile = required_string_field(root, "tls_profile", "/tls_profile", diagnostics);
    const auto* trust_requirements_value = find_field(root, "trust_requirements");
    if (trust_requirements_value == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       "/trust_requirements",
                       "Required field is missing");
    }
    const auto* server_identity_value = find_field(root, "server_identity");
    if (server_identity_value == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       "/server_identity",
                       "Required field is missing");
    }
    const auto* client_identity_value = find_field(root, "client_identity");
    if (client_identity_value == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       "/client_identity",
                       "Required field is missing");
    }

    std::optional<TrustRequirements> trust_requirements;
    std::optional<PeerIdentity> server_identity;
    std::optional<PeerIdentity> client_identity;

    if (trust_requirements_value != nullptr) {
        trust_requirements = parse_trust_requirements(*trust_requirements_value, "/trust_requirements", diagnostics);
    }
    if (server_identity_value != nullptr) {
        server_identity = parse_peer_identity(*server_identity_value, "/server_identity", diagnostics);
    }
    if (client_identity_value != nullptr) {
        client_identity = parse_peer_identity(*client_identity_value, "/client_identity", diagnostics);
    }

    const auto acl = parse_acl_rules(root, "/acl", diagnostics);

    if (server_role.has_value() && *server_role != PeerRole::pi_zymkey) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_server_role",
                       DiagnosticSeverity::error,
                       "/server_role",
                       "server_role must be pi_zymkey");
    }

    if (client_role.has_value() && *client_role != PeerRole::ubuntu_tpm) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_client_role",
                       DiagnosticSeverity::error,
                       "/client_role",
                       "client_role must be ubuntu_tpm");
    }

    if (server_identity.has_value() && server_role.has_value() && server_identity->role != *server_role) {
        add_diagnostic(diagnostics,
                       "secure_channel.role_mismatch",
                       DiagnosticSeverity::error,
                       "/server_identity/role",
                       "server_identity.role must match server_role");
    }

    if (client_identity.has_value() && client_role.has_value() && client_identity->role != *client_role) {
        add_diagnostic(diagnostics,
                       "secure_channel.role_mismatch",
                       DiagnosticSeverity::error,
                       "/client_identity/role",
                       "client_identity.role must match client_role");
    }

    if (acl.has_value() && client_role.has_value()) {
        const bool has_client_acl =
            std::any_of(acl->begin(), acl->end(), [&](const AclRule& rule) { return rule.caller_role == *client_role; });
        if (!has_client_acl) {
            add_diagnostic(diagnostics,
                           "secure_channel.invalid_acl_binding",
                           DiagnosticSeverity::error,
                           "/acl",
                           "ACL must contain a rule for the client role");
        }

        for (std::size_t index = 0U; index < acl->size(); ++index) {
            if ((*acl)[index].caller_role != *client_role) {
                add_diagnostic(diagnostics,
                               "secure_channel.invalid_acl_binding",
                               DiagnosticSeverity::error,
                               "/acl[" + std::to_string(index + 1U) + "]/caller_role",
                               "ACL caller_role must match client_role");
            }
        }
    }

    if (!channel_id.has_value() || !server_role.has_value() || !client_role.has_value() || !tls_profile.has_value() ||
        !trust_requirements.has_value() || !server_identity.has_value() || !client_identity.has_value() || !acl.has_value()) {
        return std::nullopt;
    }

    normalize_peer_identity(*server_identity);
    normalize_peer_identity(*client_identity);

    return SecureChannelContract{
        .channel_id = *channel_id,
        .server_role = *server_role,
        .client_role = *client_role,
        .tls_profile = *tls_profile,
        .trust_requirements = *trust_requirements,
        .server_identity = *server_identity,
        .client_identity = *client_identity,
        .acl = *acl,
    };
}

[[nodiscard]] std::optional<ProtectedRequestEnvelope> build_request_envelope(const JsonValue& root,
                                                                             std::vector<Diagnostic>& diagnostics) {
    if (root.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       "/",
                       "Protected request envelope must be a JSON object");
        return std::nullopt;
    }

    auto request_id = required_string_field(root, "request_id", "/request_id", diagnostics);
    const auto api_name =
        parse_required_enum<ApiName>(root, "api_name", "/api_name", diagnostics, "secure_channel.invalid_api_name", parse_api_name_string);
    const auto caller_role =
        parse_required_enum<PeerRole>(root, "caller_role", "/caller_role", diagnostics, "secure_channel.invalid_role", parse_peer_role_string);
    const auto* caller_identity_value = find_field(root, "caller_identity");
    if (caller_identity_value == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       "/caller_identity",
                       "Required field is missing");
    }
    const auto* auth_context_value = find_field(root, "auth_context");
    if (auth_context_value == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       "/auth_context",
                       "Required field is missing");
    }
    const auto* payload_value = find_field(root, "payload");
    if (payload_value == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       "/payload",
                       "Required field is missing");
    }

    std::optional<PeerIdentity> caller_identity;
    std::optional<AuthContextSummary> auth_context;
    std::optional<PayloadContract> payload;

    if (caller_identity_value != nullptr) {
        caller_identity = parse_peer_identity(*caller_identity_value, "/caller_identity", diagnostics);
    }
    if (auth_context_value != nullptr) {
        auth_context = parse_auth_context_summary(*auth_context_value, "/auth_context", diagnostics);
    }
    if (payload_value != nullptr) {
        payload = parse_payload_contract(*payload_value, "/payload", diagnostics);
    }

    if (request_id.has_value()) {
        *request_id = lower_copy(*request_id);
        if (!is_uuid_like(*request_id)) {
            add_diagnostic(diagnostics,
                           "secure_channel.invalid_request_id",
                           DiagnosticSeverity::error,
                           "/request_id",
                           "request_id must be a canonical UUID");
        }
    }

    if (caller_identity.has_value() && caller_role.has_value() && caller_identity->role != *caller_role) {
        add_diagnostic(diagnostics,
                       "secure_channel.role_mismatch",
                       DiagnosticSeverity::error,
                       "/caller_identity/role",
                       "caller_identity.role must match caller_role");
    }

    if (auth_context.has_value() && auth_context->local_role != PeerRole::pi_zymkey) {
        add_diagnostic(diagnostics,
                       "secure_channel.role_mismatch",
                       DiagnosticSeverity::error,
                       "/auth_context/local_role",
                       "auth_context.local_role must be pi_zymkey");
    }

    if (caller_identity.has_value() && auth_context.has_value() &&
        caller_identity->certificate_fingerprint != auth_context->peer_trust.subject_fingerprint) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_trust_binding",
                       DiagnosticSeverity::error,
                       "/auth_context/peer_trust/subject_fingerprint",
                       "peer_trust.subject_fingerprint must bind to caller_identity.certificate_fingerprint");
    }

    if (payload.has_value() && api_name.has_value()) {
        static_cast<void>(validate_request_payload_contract(*api_name, *payload, "/payload", diagnostics));
    }

    if (!request_id.has_value() || !api_name.has_value() || !caller_role.has_value() || !caller_identity.has_value() ||
        !auth_context.has_value() || !payload.has_value() || !is_uuid_like(request_id.value_or(""))) {
        return std::nullopt;
    }

    normalize_peer_identity(*caller_identity);

    return ProtectedRequestEnvelope{
        .request_id = *request_id,
        .api_name = *api_name,
        .caller_role = *caller_role,
        .caller_identity = *caller_identity,
        .auth_context = *auth_context,
        .payload = *payload,
    };
}

[[nodiscard]] std::optional<ProtectedResponseEnvelope> build_response_envelope(const JsonValue& root,
                                                                               std::vector<Diagnostic>& diagnostics) {
    if (root.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       "/",
                       "Protected response envelope must be a JSON object");
        return std::nullopt;
    }

    auto request_id = required_string_field(root, "request_id", "/request_id", diagnostics);
    const auto status = parse_required_enum<EnvelopeStatus>(root,
                                                            "status",
                                                            "/status",
                                                            diagnostics,
                                                            "secure_channel.invalid_status",
                                                            parse_envelope_status_string);
    const auto* diagnostics_value = find_field(root, "diagnostics");
    if (diagnostics_value == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       "/diagnostics",
                       "Required field is missing");
    }
    const auto* payload_value = find_field(root, "payload");
    if (payload_value == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       "/payload",
                       "Required field is missing");
    }

    std::vector<Diagnostic> response_diagnostics;
    if (diagnostics_value != nullptr) {
        if (diagnostics_value->type != JsonValue::Type::array) {
            add_diagnostic(diagnostics,
                           "secure_channel.invalid_type",
                           DiagnosticSeverity::error,
                           "/diagnostics",
                           "diagnostics must be a JSON array");
        } else {
            for (std::size_t index = 0U; index < diagnostics_value->array_values.size(); ++index) {
                const auto& item = diagnostics_value->array_values[index];
                const std::string base_path = "/diagnostics[" + std::to_string(index + 1U) + "]";
                if (item.type != JsonValue::Type::object) {
                    add_diagnostic(diagnostics,
                                   "secure_channel.invalid_type",
                                   DiagnosticSeverity::error,
                                   base_path,
                                   "Diagnostic entry must be a JSON object");
                    continue;
                }

                const auto code = required_string_field(item, "code", base_path + "/code", diagnostics);
                const auto severity = parse_required_enum<DiagnosticSeverity>(item,
                                                                              "severity",
                                                                              base_path + "/severity",
                                                                              diagnostics,
                                                                              "secure_channel.invalid_severity",
                                                                              [](std::string_view value)
                                                                                  -> std::optional<DiagnosticSeverity> {
                                                                                  if (value == "error") {
                                                                                      return DiagnosticSeverity::error;
                                                                                  }
                                                                                  if (value == "warning") {
                                                                                      return DiagnosticSeverity::warning;
                                                                                  }
                                                                                  return std::nullopt;
                                                                              });
                const auto path = required_string_field(item, "path", base_path + "/path", diagnostics);
                const auto message = required_string_field(item, "message", base_path + "/message", diagnostics);
                if (code.has_value() && severity.has_value() && path.has_value() && message.has_value()) {
                    response_diagnostics.push_back(Diagnostic{
                        .code = *code,
                        .severity = *severity,
                        .path = *path,
                        .message = *message,
                    });
                }
            }
        }
    }

    std::optional<PayloadContract> payload;
    if (payload_value != nullptr) {
        payload = parse_payload_contract(*payload_value, "/payload", diagnostics);
    }

    if (request_id.has_value()) {
        *request_id = lower_copy(*request_id);
        if (!is_uuid_like(*request_id)) {
            add_diagnostic(diagnostics,
                           "secure_channel.invalid_request_id",
                           DiagnosticSeverity::error,
                           "/request_id",
                           "request_id must be a canonical UUID");
        }
    }

    if (status.has_value() && *status == EnvelopeStatus::ok && !response_diagnostics.empty()) {
        add_diagnostic(diagnostics,
                       "secure_channel.unexpected_response_diagnostics",
                       DiagnosticSeverity::error,
                       "/diagnostics",
                       "status=ok response must not carry diagnostics");
    }

    if (status.has_value() && *status != EnvelopeStatus::ok && response_diagnostics.empty()) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_response_diagnostics",
                       DiagnosticSeverity::error,
                       "/diagnostics",
                       "denied or error response must carry diagnostics");
    }

    if (payload.has_value()) {
        static_cast<void>(validate_response_payload_contract(*payload, status, false, "/payload", diagnostics));
    }

    if (!request_id.has_value() || !status.has_value() || !payload.has_value() || !is_uuid_like(request_id.value_or(""))) {
        return std::nullopt;
    }

    sort_diagnostics(response_diagnostics);

    return ProtectedResponseEnvelope{
        .request_id = *request_id,
        .status = *status,
        .diagnostics = std::move(response_diagnostics),
        .payload = *payload,
    };
}

[[nodiscard]] std::optional<SecurityModuleContract> build_security_module_contract(const JsonValue& root,
                                                                                   std::vector<Diagnostic>& diagnostics) {
    if (root.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_type",
                       DiagnosticSeverity::error,
                       "/",
                       "SecurityModuleContract must be a JSON object");
        return std::nullopt;
    }

    const auto module_id = required_string_field(root, "module_id", "/module_id", diagnostics);
    const auto module_role =
        parse_required_enum<PeerRole>(root, "module_role", "/module_role", diagnostics, "secure_channel.invalid_role", parse_peer_role_string);
    const auto secure_channel_id =
        required_string_field(root, "secure_channel_id", "/secure_channel_id", diagnostics);
    const auto request_envelope_ref =
        required_string_field(root, "request_envelope_ref", "/request_envelope_ref", diagnostics);
    const auto response_envelope_ref =
        required_string_field(root, "response_envelope_ref", "/response_envelope_ref", diagnostics);
    const auto supported_api_names =
        parse_required_enum_array<ApiName>(root,
                                           "supported_api_names",
                                           "/supported_api_names",
                                           diagnostics,
                                           "secure_channel.invalid_api_name",
                                           parse_api_name_string,
                                           "secure_channel.duplicate_supported_api_name",
                                           "API name duplicates an earlier entry");
    const auto* server_identity_value = find_field(root, "server_identity");
    if (server_identity_value == nullptr) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_field",
                       DiagnosticSeverity::error,
                       "/server_identity",
                       "Required field is missing");
    }

    std::optional<PeerIdentity> server_identity;
    if (server_identity_value != nullptr) {
        server_identity = parse_peer_identity(*server_identity_value, "/server_identity", diagnostics);
    }

    if (module_role.has_value() && *module_role != PeerRole::pi_zymkey) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_server_role",
                       DiagnosticSeverity::error,
                       "/module_role",
                       "module_role must be pi_zymkey");
    }

    if (server_identity.has_value() && module_role.has_value() && server_identity->role != *module_role) {
        add_diagnostic(diagnostics,
                       "secure_channel.role_mismatch",
                       DiagnosticSeverity::error,
                       "/server_identity/role",
                       "server_identity.role must match module_role");
    }

    if (secure_channel_id.has_value() && *secure_channel_id != kBaselineSecureChannelId) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_secure_channel_ref",
                       DiagnosticSeverity::error,
                       "/secure_channel_id",
                       "secure_channel_id must reference the T03b secure channel baseline");
    }

    if (request_envelope_ref.has_value() && *request_envelope_ref != kBaselineRequestEnvelopeRef) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_request_envelope_ref",
                       DiagnosticSeverity::error,
                       "/request_envelope_ref",
                       "request_envelope_ref must reference the protected request baseline");
    }

    if (response_envelope_ref.has_value() && *response_envelope_ref != kBaselineResponseEnvelopeRef) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_response_envelope_ref",
                       DiagnosticSeverity::error,
                       "/response_envelope_ref",
                       "response_envelope_ref must reference the protected response baseline");
    }

    if (!module_id.has_value() || !module_role.has_value() || !secure_channel_id.has_value() ||
        !request_envelope_ref.has_value() || !response_envelope_ref.has_value() || !supported_api_names.has_value() ||
        !server_identity.has_value()) {
        return std::nullopt;
    }

    normalize_peer_identity(*server_identity);

    return SecurityModuleContract{
        .module_id = *module_id,
        .module_role = *module_role,
        .secure_channel_id = *secure_channel_id,
        .request_envelope_ref = *request_envelope_ref,
        .response_envelope_ref = *response_envelope_ref,
        .supported_api_names = *supported_api_names,
        .server_identity = *server_identity,
    };
}

void check_identity_binding(const PeerIdentity& expected,
                            const PeerIdentity& actual,
                            std::string_view path,
                            std::vector<Diagnostic>& diagnostics) {
    if (expected.role != actual.role) {
        add_diagnostic(diagnostics,
                       "secure_channel.role_mismatch",
                       DiagnosticSeverity::error,
                       std::string{path} + "/role",
                       "Peer role does not match contract");
    }

    if (expected.device_id != actual.device_id) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_peer_identity",
                       DiagnosticSeverity::error,
                       std::string{path} + "/device_id",
                       "device_id does not match contract");
    }

    if (expected.certificate_fingerprint != actual.certificate_fingerprint) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_peer_identity",
                       DiagnosticSeverity::error,
                       std::string{path} + "/certificate_fingerprint",
                       "certificate_fingerprint does not match contract");
    }

    if (expected.subject_dn != actual.subject_dn) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_peer_identity",
                       DiagnosticSeverity::error,
                       std::string{path} + "/subject_dn",
                       "subject_dn does not match contract");
    }

    if (expected.san_dns_names != actual.san_dns_names) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_peer_identity",
                       DiagnosticSeverity::error,
                       std::string{path} + "/san_dns_names",
                       "san_dns_names do not match contract");
    }

    if (expected.san_uri_names != actual.san_uri_names) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_peer_identity",
                       DiagnosticSeverity::error,
                       std::string{path} + "/san_uri_names",
                       "san_uri_names do not match contract");
    }
}

void append_string_array(std::ostringstream& output, const std::vector<std::string>& values, std::size_t indent_level) {
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
        output << "\n";
    }
    append_indent(output, indent_level);
    output << "]";
}

template <typename T>
void append_enum_array(std::ostringstream& output,
                       const std::vector<T>& values,
                       std::size_t indent_level,
                       const auto& formatter) {
    if (values.empty()) {
        output << "[]";
        return;
    }

    output << "[\n";
    for (std::size_t index = 0U; index < values.size(); ++index) {
        append_indent(output, indent_level + 1U);
        output << "\"" << formatter(values[index]) << "\"";
        if (index + 1U != values.size()) {
            output << ",";
        }
        output << "\n";
    }
    append_indent(output, indent_level);
    output << "]";
}

void append_string_map(std::ostringstream& output, const std::map<std::string, std::string>& values, std::size_t indent_level) {
    if (values.empty()) {
        output << "{}";
        return;
    }

    output << "{\n";
    std::size_t index = 0U;
    for (const auto& [key, value] : values) {
        append_indent(output, indent_level + 1U);
        output << "\"" << json_escape(key) << "\": \"" << json_escape(value) << "\"";
        if (index + 1U != values.size()) {
            output << ",";
        }
        output << "\n";
        ++index;
    }
    append_indent(output, indent_level);
    output << "}";
}

void append_peer_identity(std::ostringstream& output, const PeerIdentity& identity, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"role\": \"" << to_string(identity.role) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"device_id\": \"" << json_escape(identity.device_id) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"certificate_fingerprint\": \"" << json_escape(identity.certificate_fingerprint) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"subject_dn\": \"" << json_escape(identity.subject_dn) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"san_dns_names\": ";
    append_string_array(output, identity.san_dns_names, indent_level + 1U);
    output << ",\n";
    append_indent(output, indent_level + 1U);
    output << "\"san_uri_names\": ";
    append_string_array(output, identity.san_uri_names, indent_level + 1U);
    output << "\n";
    append_indent(output, indent_level);
    output << "}";
}

void append_trust_requirements(std::ostringstream& output, const TrustRequirements& requirements, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"certificate_store_ref\": \"" << json_escape(requirements.certificate_store_ref) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"required_decision\": \"" << to_string(requirements.required_decision) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"required_decision_reason\": \"" << to_string(requirements.required_decision_reason) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"accepted_revocation_statuses\": ";
    append_enum_array(output,
                      requirements.accepted_revocation_statuses,
                      indent_level + 1U,
                      [](RevocationStatus status) { return to_string(status); });
    output << ",\n";
    append_indent(output, indent_level + 1U);
    output << "\"required_checked_sources\": ";
    append_string_array(output, requirements.required_checked_sources, indent_level + 1U);
    output << "\n";
    append_indent(output, indent_level);
    output << "}";
}

void append_trust_summary(std::ostringstream& output, const TrustSummary& summary, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"subject_fingerprint\": \"" << json_escape(summary.subject_fingerprint) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"issuer_chain\": ";
    append_string_array(output, summary.issuer_chain, indent_level + 1U);
    output << ",\n";
    append_indent(output, indent_level + 1U);
    output << "\"validation_time_utc\": \"" << json_escape(summary.validation_time_utc) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"revocation_status\": \"" << to_string(summary.revocation_status) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"decision\": \"" << to_string(summary.decision) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"decision_reason\": \"" << to_string(summary.decision_reason) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"checked_sources\": ";
    append_string_array(output, summary.checked_sources, indent_level + 1U);
    output << "\n";
    append_indent(output, indent_level);
    output << "}";
}

void append_auth_context(std::ostringstream& output, const AuthContextSummary& auth_context, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"channel_id\": \"" << json_escape(auth_context.channel_id) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"mutual_tls\": " << (auth_context.mutual_tls ? "true" : "false") << ",\n";
    append_indent(output, indent_level + 1U);
    output << "\"local_role\": \"" << to_string(auth_context.local_role) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"peer_trust\": ";
    append_trust_summary(output, auth_context.peer_trust, indent_level + 1U);
    output << ",\n";
    append_indent(output, indent_level + 1U);
    output << "\"checked_sources\": ";
    append_string_array(output, auth_context.checked_sources, indent_level + 1U);
    output << "\n";
    append_indent(output, indent_level);
    output << "}";
}

void append_payload(std::ostringstream& output, const PayloadContract& payload, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"payload_type\": \"" << json_escape(payload.payload_type) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"schema_ref\": \"" << json_escape(payload.schema_ref) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"body\": ";
    append_string_map(output, payload.body, indent_level + 1U);
    output << "\n";
    append_indent(output, indent_level);
    output << "}";
}

void append_acl_rule(std::ostringstream& output, const AclRule& rule, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"caller_role\": \"" << to_string(rule.caller_role) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"allowed_api_names\": ";
    append_enum_array(output,
                      rule.allowed_api_names,
                      indent_level + 1U,
                      [](ApiName api_name) { return to_string(api_name); });
    output << "\n";
    append_indent(output, indent_level);
    output << "}";
}

void append_diagnostic(std::ostringstream& output, const Diagnostic& diagnostic, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"code\": \"" << json_escape(diagnostic.code) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"severity\": \"" << to_string(diagnostic.severity) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"path\": \"" << json_escape(diagnostic.path) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"message\": \"" << json_escape(diagnostic.message) << "\"\n";
    append_indent(output, indent_level);
    output << "}";
}

}  // namespace

ParseResult<SecureChannelContract> parse_secure_channel_contract(std::string_view json) {
    JsonParser parser(json);
    JsonParseError error;
    auto root = parser.parse(error);
    if (!root.has_value()) {
        return make_parse_result<SecureChannelContract>(
            std::nullopt,
            {Diagnostic{
                .code = "secure_channel.json_malformed",
                .severity = DiagnosticSeverity::error,
                .path = std::move(error.path),
                .message = std::move(error.message),
            }});
    }

    std::vector<Diagnostic> diagnostics;
    auto contract = build_secure_channel_contract(*root, diagnostics);
    return make_parse_result(std::move(contract), std::move(diagnostics));
}

ParseResult<ProtectedRequestEnvelope> parse_protected_request_envelope(std::string_view json) {
    JsonParser parser(json);
    JsonParseError error;
    auto root = parser.parse(error);
    if (!root.has_value()) {
        return make_parse_result<ProtectedRequestEnvelope>(
            std::nullopt,
            {Diagnostic{
                .code = "secure_channel.json_malformed",
                .severity = DiagnosticSeverity::error,
                .path = std::move(error.path),
                .message = std::move(error.message),
            }});
    }

    std::vector<Diagnostic> diagnostics;
    auto envelope = build_request_envelope(*root, diagnostics);
    return make_parse_result(std::move(envelope), std::move(diagnostics));
}

ParseResult<ProtectedResponseEnvelope> parse_protected_response_envelope(std::string_view json) {
    JsonParser parser(json);
    JsonParseError error;
    auto root = parser.parse(error);
    if (!root.has_value()) {
        return make_parse_result<ProtectedResponseEnvelope>(
            std::nullopt,
            {Diagnostic{
                .code = "secure_channel.json_malformed",
                .severity = DiagnosticSeverity::error,
                .path = std::move(error.path),
                .message = std::move(error.message),
            }});
    }

    std::vector<Diagnostic> diagnostics;
    auto envelope = build_response_envelope(*root, diagnostics);
    return make_parse_result(std::move(envelope), std::move(diagnostics));
}

ParseResult<SecurityModuleContract> parse_security_module_contract(std::string_view json) {
    JsonParser parser(json);
    JsonParseError error;
    auto root = parser.parse(error);
    if (!root.has_value()) {
        return make_parse_result<SecurityModuleContract>(
            std::nullopt,
            {Diagnostic{
                .code = "secure_channel.json_malformed",
                .severity = DiagnosticSeverity::error,
                .path = std::move(error.path),
                .message = std::move(error.message),
            }});
    }

    std::vector<Diagnostic> diagnostics;
    auto contract = build_security_module_contract(*root, diagnostics);
    return make_parse_result(std::move(contract), std::move(diagnostics));
}

CheckResult authorize_request(const SecureChannelContract& contract, const ProtectedRequestEnvelope& request) {
    std::vector<Diagnostic> diagnostics;

    const bool caller_role_matches_contract = request.caller_role == contract.client_role;
    if (!caller_role_matches_contract) {
        add_diagnostic(diagnostics,
                       "secure_channel.role_mismatch",
                       DiagnosticSeverity::error,
                       "/caller_role",
                       "caller_role must match contract.client_role");
    }

    if (request.auth_context.local_role != contract.server_role) {
        add_diagnostic(diagnostics,
                       "secure_channel.role_mismatch",
                       DiagnosticSeverity::error,
                       "/auth_context/local_role",
                       "auth_context.local_role must match contract.server_role");
    }

    if (request.auth_context.channel_id != contract.channel_id) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_channel_binding",
                       DiagnosticSeverity::error,
                       "/auth_context/channel_id",
                       "auth_context.channel_id must match contract.channel_id");
    }

    if (!request.auth_context.mutual_tls) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_channel_binding",
                       DiagnosticSeverity::error,
                       "/auth_context/mutual_tls",
                       "mutual_tls must be true");
    }

    if (caller_role_matches_contract) {
        check_identity_binding(contract.client_identity, request.caller_identity, "/caller_identity", diagnostics);
    }

    if (caller_role_matches_contract &&
        request.caller_identity.certificate_fingerprint != request.auth_context.peer_trust.subject_fingerprint) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_trust_binding",
                       DiagnosticSeverity::error,
                       "/auth_context/peer_trust/subject_fingerprint",
                       "peer_trust.subject_fingerprint must bind to caller_identity.certificate_fingerprint");
    }

    if (caller_role_matches_contract) {
        if (request.auth_context.peer_trust.decision != contract.trust_requirements.required_decision ||
            request.auth_context.peer_trust.decision_reason != contract.trust_requirements.required_decision_reason ||
            !contains(contract.trust_requirements.accepted_revocation_statuses, request.auth_context.peer_trust.revocation_status)) {
            add_diagnostic(diagnostics,
                           "secure_channel.peer_not_trusted",
                           DiagnosticSeverity::error,
                           "/auth_context/peer_trust",
                           "peer_trust does not satisfy trust_requirements");
        }

        for (const auto& required_source : contract.trust_requirements.required_checked_sources) {
            if (!contains(request.auth_context.peer_trust.checked_sources, required_source)) {
                add_diagnostic(diagnostics,
                               "secure_channel.missing_trust_binding",
                               DiagnosticSeverity::error,
                               "/auth_context/peer_trust/checked_sources",
                               "peer_trust.checked_sources must include every required trust source");
                break;
            }
        }
    }

    bool acl_match = false;
    bool api_allowed = false;
    if (caller_role_matches_contract) {
        for (const auto& rule : contract.acl) {
            if (rule.caller_role != request.caller_role) {
                continue;
            }
            acl_match = true;
            api_allowed = contains(rule.allowed_api_names, request.api_name);
            break;
        }
    }

    if (caller_role_matches_contract && (!acl_match || !api_allowed)) {
        add_diagnostic(diagnostics,
                       "secure_channel.unauthorized_api",
                       DiagnosticSeverity::error,
                       "/api_name",
                       "caller_role is not authorized for api_name");
    }

    auto payload = request.payload;
    static_cast<void>(validate_request_payload_contract(request.api_name, payload, "/payload", diagnostics));

    sort_diagnostics(diagnostics);
    const auto status = validation_status_for(diagnostics);
    return CheckResult{
        .diagnostics = std::move(diagnostics),
        .validation_status = status,
    };
}

CheckResult validate_response(const SecureChannelContract& contract,
                              const ProtectedRequestEnvelope& request,
                              const ProtectedResponseEnvelope& response) {
    std::vector<Diagnostic> diagnostics;

    if (request.auth_context.channel_id != contract.channel_id) {
        add_diagnostic(diagnostics,
                       "secure_channel.invalid_channel_binding",
                       DiagnosticSeverity::error,
                       "/request/auth_context/channel_id",
                       "request auth_context.channel_id must match contract.channel_id");
    }

    if (response.request_id != request.request_id) {
        add_diagnostic(diagnostics,
                       "secure_channel.request_id_mismatch",
                       DiagnosticSeverity::error,
                       "/request_id",
                       "response.request_id must match request.request_id");
    }

    auto payload = response.payload;
    const auto response_family =
        validate_response_payload_contract(payload, response.status, true, "/payload", diagnostics);

    if (response_family.has_value() && response.status == EnvelopeStatus::ok &&
        *response_family != PayloadFamily::error_response) {
        const auto response_api_name = api_name_for_payload_family(*response_family);
        if (!response_api_name.has_value() || *response_api_name != request.api_name) {
            add_diagnostic(diagnostics,
                           "secure_channel.request_response_api_mismatch",
                           DiagnosticSeverity::error,
                           "/payload/payload_type",
                           "response payload family must match request.api_name");
        }
    }

    if (response.status == EnvelopeStatus::ok && !response.diagnostics.empty()) {
        add_diagnostic(diagnostics,
                       "secure_channel.unexpected_response_diagnostics",
                       DiagnosticSeverity::error,
                       "/diagnostics",
                       "status=ok response must not carry diagnostics");
    }

    if (response.status != EnvelopeStatus::ok && response.diagnostics.empty()) {
        add_diagnostic(diagnostics,
                       "secure_channel.missing_response_diagnostics",
                       DiagnosticSeverity::error,
                       "/diagnostics",
                       "denied or error response must carry diagnostics");
    }

    sort_diagnostics(diagnostics);
    const auto status = validation_status_for(diagnostics);
    return CheckResult{
        .diagnostics = std::move(diagnostics),
        .validation_status = status,
    };
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

const char* to_string(PeerRole role) noexcept {
    switch (role) {
        case PeerRole::pi_zymkey:
            return "pi_zymkey";
        case PeerRole::ubuntu_tpm:
            return "ubuntu_tpm";
    }
    return "ubuntu_tpm";
}

const char* to_string(ApiName api_name) noexcept {
    switch (api_name) {
        case ApiName::sign:
            return "sign";
        case ApiName::unwrap:
            return "unwrap";
        case ApiName::decrypt:
            return "decrypt";
        case ApiName::health:
            return "health";
        case ApiName::identity:
            return "identity";
    }
    return "health";
}

const char* to_string(TrustDecision decision) noexcept {
    switch (decision) {
        case TrustDecision::trusted:
            return "trusted";
        case TrustDecision::rejected:
            return "rejected";
        case TrustDecision::warning_only:
            return "warning_only";
    }
    return "rejected";
}

const char* to_string(TrustDecisionReason reason) noexcept {
    switch (reason) {
        case TrustDecisionReason::ok:
            return "ok";
        case TrustDecisionReason::anchor_missing:
            return "anchor_missing";
        case TrustDecisionReason::expired:
            return "expired";
        case TrustDecisionReason::not_yet_valid:
            return "not_yet_valid";
        case TrustDecisionReason::revoked:
            return "revoked";
        case TrustDecisionReason::chain_broken:
            return "chain_broken";
        case TrustDecisionReason::algorithm_forbidden:
            return "algorithm_forbidden";
        case TrustDecisionReason::revocation_unavailable:
            return "revocation_unavailable";
    }
    return "ok";
}

const char* to_string(RevocationStatus status) noexcept {
    switch (status) {
        case RevocationStatus::good:
            return "good";
        case RevocationStatus::revoked:
            return "revoked";
        case RevocationStatus::unknown:
            return "unknown";
        case RevocationStatus::not_checked:
            return "not_checked";
    }
    return "unknown";
}

const char* to_string(EnvelopeStatus status) noexcept {
    switch (status) {
        case EnvelopeStatus::ok:
            return "ok";
        case EnvelopeStatus::denied:
            return "denied";
        case EnvelopeStatus::error:
            return "error";
    }
    return "error";
}

std::string to_json(const SecureChannelContract& contract) {
    std::ostringstream output;
    output << "{\n";
    append_indent(output, 1U);
    output << "\"channel_id\": \"" << json_escape(contract.channel_id) << "\",\n";
    append_indent(output, 1U);
    output << "\"server_role\": \"" << to_string(contract.server_role) << "\",\n";
    append_indent(output, 1U);
    output << "\"client_role\": \"" << to_string(contract.client_role) << "\",\n";
    append_indent(output, 1U);
    output << "\"tls_profile\": \"" << json_escape(contract.tls_profile) << "\",\n";
    append_indent(output, 1U);
    output << "\"trust_requirements\": ";
    append_trust_requirements(output, contract.trust_requirements, 1U);
    output << ",\n";
    append_indent(output, 1U);
    output << "\"server_identity\": ";
    append_peer_identity(output, contract.server_identity, 1U);
    output << ",\n";
    append_indent(output, 1U);
    output << "\"client_identity\": ";
    append_peer_identity(output, contract.client_identity, 1U);
    output << ",\n";
    append_indent(output, 1U);
    output << "\"acl\": [\n";
    for (std::size_t index = 0U; index < contract.acl.size(); ++index) {
        append_indent(output, 2U);
        append_acl_rule(output, contract.acl[index], 2U);
        if (index + 1U != contract.acl.size()) {
            output << ",";
        }
        output << "\n";
    }
    append_indent(output, 1U);
    output << "]\n";
    output << "}\n";
    return output.str();
}

std::string to_json(const ProtectedRequestEnvelope& envelope) {
    std::ostringstream output;
    output << "{\n";
    append_indent(output, 1U);
    output << "\"request_id\": \"" << json_escape(envelope.request_id) << "\",\n";
    append_indent(output, 1U);
    output << "\"api_name\": \"" << to_string(envelope.api_name) << "\",\n";
    append_indent(output, 1U);
    output << "\"caller_role\": \"" << to_string(envelope.caller_role) << "\",\n";
    append_indent(output, 1U);
    output << "\"caller_identity\": ";
    append_peer_identity(output, envelope.caller_identity, 1U);
    output << ",\n";
    append_indent(output, 1U);
    output << "\"auth_context\": ";
    append_auth_context(output, envelope.auth_context, 1U);
    output << ",\n";
    append_indent(output, 1U);
    output << "\"payload\": ";
    append_payload(output, envelope.payload, 1U);
    output << "\n";
    output << "}\n";
    return output.str();
}

std::string to_json(const ProtectedResponseEnvelope& envelope) {
    std::ostringstream output;
    output << "{\n";
    append_indent(output, 1U);
    output << "\"request_id\": \"" << json_escape(envelope.request_id) << "\",\n";
    append_indent(output, 1U);
    output << "\"status\": \"" << to_string(envelope.status) << "\",\n";
    append_indent(output, 1U);
    output << "\"diagnostics\": ";
    if (envelope.diagnostics.empty()) {
        output << "[]";
    } else {
        output << "[\n";
        for (std::size_t index = 0U; index < envelope.diagnostics.size(); ++index) {
            append_indent(output, 2U);
            append_diagnostic(output, envelope.diagnostics[index], 2U);
            if (index + 1U != envelope.diagnostics.size()) {
                output << ",";
            }
            output << "\n";
        }
        append_indent(output, 1U);
        output << "]";
    }
    output << ",\n";
    append_indent(output, 1U);
    output << "\"payload\": ";
    append_payload(output, envelope.payload, 1U);
    output << "\n";
    output << "}\n";
    return output.str();
}

std::string to_json(const SecurityModuleContract& contract) {
    std::ostringstream output;
    output << "{\n";
    append_indent(output, 1U);
    output << "\"module_id\": \"" << json_escape(contract.module_id) << "\",\n";
    append_indent(output, 1U);
    output << "\"module_role\": \"" << to_string(contract.module_role) << "\",\n";
    append_indent(output, 1U);
    output << "\"secure_channel_id\": \"" << json_escape(contract.secure_channel_id) << "\",\n";
    append_indent(output, 1U);
    output << "\"request_envelope_ref\": \"" << json_escape(contract.request_envelope_ref) << "\",\n";
    append_indent(output, 1U);
    output << "\"response_envelope_ref\": \"" << json_escape(contract.response_envelope_ref) << "\",\n";
    append_indent(output, 1U);
    output << "\"supported_api_names\": ";
    append_enum_array(output,
                      contract.supported_api_names,
                      1U,
                      [](ApiName api_name) { return to_string(api_name); });
    output << ",\n";
    append_indent(output, 1U);
    output << "\"server_identity\": ";
    append_peer_identity(output, contract.server_identity, 1U);
    output << "\n";
    output << "}\n";
    return output.str();
}

std::string diagnostics_to_json(const std::vector<Diagnostic>& diagnostics) {
    std::ostringstream output;
    if (diagnostics.empty()) {
        output << "[]\n";
        return output.str();
    }

    output << "[\n";
    for (std::size_t index = 0U; index < diagnostics.size(); ++index) {
        append_indent(output, 1U);
        append_diagnostic(output, diagnostics[index], 1U);
        if (index + 1U != diagnostics.size()) {
            output << ",";
        }
        output << "\n";
    }
    output << "]\n";
    return output.str();
}

}  // namespace dcplayer::security_api::secure_channel
