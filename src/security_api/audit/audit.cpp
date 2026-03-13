#include "audit.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace dcplayer::security_api::audit {
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
        const auto parsed = parse_string(std::move(path), error);
        if (!parsed.has_value()) {
            return std::nullopt;
        }

        JsonValue value;
        value.type = JsonValue::Type::string;
        value.string_value = std::move(*parsed);
        return value;
    }

    [[nodiscard]] std::optional<JsonValue> parse_literal(std::string_view literal,
                                                         bool boolean_value,
                                                         std::string path,
                                                         JsonParseError& error) {
        if (input_.substr(position_, literal.size()) != literal) {
            error = JsonParseError{
                .path = std::move(path),
                .message = "invalid literal",
            };
            return std::nullopt;
        }

        position_ += literal.size();
        JsonValue value;
        value.type = JsonValue::Type::boolean;
        value.boolean_value = boolean_value;
        return value;
    }

    [[nodiscard]] std::optional<JsonValue> parse_null(std::string path, JsonParseError& error) {
        if (input_.substr(position_, 4U) != "null") {
            error = JsonParseError{
                .path = std::move(path),
                .message = "invalid literal",
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
                .message = "expected string",
            };
            return std::nullopt;
        }

        std::string value;
        while (position_ < input_.size()) {
            const char ch = input_[position_++];
            if (ch == '"') {
                return value;
            }

            if (ch == '\\') {
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
                        value.push_back(escaped);
                        break;
                    case 'b':
                        value.push_back('\b');
                        break;
                    case 'f':
                        value.push_back('\f');
                        break;
                    case 'n':
                        value.push_back('\n');
                        break;
                    case 'r':
                        value.push_back('\r');
                        break;
                    case 't':
                        value.push_back('\t');
                        break;
                    default:
                        error = JsonParseError{
                            .path = std::move(path),
                            .message = "unsupported escape sequence",
                        };
                        return std::nullopt;
                }
                continue;
            }

            value.push_back(ch);
        }

        error = JsonParseError{
            .path = std::move(path),
            .message = "unterminated string",
        };
        return std::nullopt;
    }

    void skip_whitespace() {
        while (position_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[position_])) != 0) {
            ++position_;
        }
    }

    [[nodiscard]] bool consume(char expected) {
        if (position_ >= input_.size() || input_[position_] != expected) {
            return false;
        }
        ++position_;
        return true;
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
                    std::string_view code,
                    DiagnosticSeverity severity,
                    std::string path,
                    std::string message) {
    diagnostics.push_back(Diagnostic{
        .code = std::string{code},
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
           std::all_of(input.begin(), input.end(), [](char ch) {
               return std::isxdigit(static_cast<unsigned char>(ch)) != 0;
           });
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

[[nodiscard]] std::string json_escape(std::string_view value) {
    std::ostringstream output;
    for (const char ch : value) {
        switch (ch) {
            case '\\':
                output << "\\\\";
                break;
            case '"':
                output << "\\\"";
                break;
            case '\b':
                output << "\\b";
                break;
            case '\f':
                output << "\\f";
                break;
            case '\n':
                output << "\\n";
                break;
            case '\r':
                output << "\\r";
                break;
            case '\t':
                output << "\\t";
                break;
            default:
                output << ch;
                break;
        }
    }
    return output.str();
}

void append_indent(std::ostringstream& output, std::size_t indent_level) {
    for (std::size_t index = 0U; index < indent_level; ++index) {
        output << "  ";
    }
}

[[nodiscard]] const JsonValue* find_field(const JsonValue& object, std::string_view name) {
    if (object.type != JsonValue::Type::object) {
        return nullptr;
    }

    for (const auto& [field_name, value] : object.object_values) {
        if (field_name == name) {
            return &value;
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
                       "audit.missing_required_field",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Missing required audit field");
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::string) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must be a string");
        return std::nullopt;
    }

    return field->string_value;
}

[[nodiscard]] std::optional<std::string> optional_string_field(const JsonValue& object,
                                                               std::string_view field_name,
                                                               std::string_view path,
                                                               std::vector<Diagnostic>& diagnostics) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::string) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must be a string");
        return std::nullopt;
    }

    return field->string_value;
}

[[nodiscard]] const JsonValue* required_object_field(const JsonValue& object,
                                                     std::string_view field_name,
                                                     std::string_view path,
                                                     std::vector<Diagnostic>& diagnostics) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        add_diagnostic(diagnostics,
                       "audit.missing_required_field",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Missing required audit field");
        return nullptr;
    }

    if (field->type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must be an object");
        return nullptr;
    }

    return field;
}

[[nodiscard]] const JsonValue* required_array_field(const JsonValue& object,
                                                    std::string_view field_name,
                                                    std::string_view path,
                                                    std::vector<Diagnostic>& diagnostics) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        add_diagnostic(diagnostics,
                       "audit.missing_required_field",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Missing required audit field");
        return nullptr;
    }

    if (field->type != JsonValue::Type::array) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must be an array");
        return nullptr;
    }

    return field;
}

[[nodiscard]] std::optional<std::map<std::string, std::string>> required_string_map_field(const JsonValue& object,
                                                                                           std::string_view field_name,
                                                                                           std::string_view path,
                                                                                           std::vector<Diagnostic>& diagnostics) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        add_diagnostic(diagnostics,
                       "audit.missing_required_field",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Missing required audit field");
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must be an object");
        return std::nullopt;
    }

    std::map<std::string, std::string> values;
    for (const auto& [field_name_value, item] : field->object_values) {
        if (item.type != JsonValue::Type::string) {
            add_diagnostic(diagnostics,
                           "audit.invalid_type",
                           DiagnosticSeverity::error,
                           std::string{path} + "/" + field_name_value,
                           "Metadata values must be strings");
            return std::nullopt;
        }

        values.insert_or_assign(field_name_value, item.string_value);
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

[[nodiscard]] std::optional<SecurityEventType> parse_event_type_string(std::string_view value) {
    if (value == "auth.session_authenticated") {
        return SecurityEventType::auth_session_authenticated;
    }
    if (value == "trust.peer_validated") {
        return SecurityEventType::trust_peer_validated;
    }
    if (value == "acl.decision_recorded") {
        return SecurityEventType::acl_decision_recorded;
    }
    if (value == "api.request_processed") {
        return SecurityEventType::api_request_processed;
    }
    if (value == "tamper.tamper_detected") {
        return SecurityEventType::tamper_detected;
    }
    if (value == "tamper.tamper_lockout") {
        return SecurityEventType::tamper_lockout;
    }
    if (value == "recovery.recovery_started") {
        return SecurityEventType::recovery_started;
    }
    if (value == "recovery.recovery_completed") {
        return SecurityEventType::recovery_completed;
    }
    if (value == "recovery.recovery_failed") {
        return SecurityEventType::recovery_failed;
    }
    if (value == "export.audit_exported") {
        return SecurityEventType::export_audit_exported;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<SecurityEventSeverity> parse_event_severity_string(std::string_view value) {
    if (value == "info") {
        return SecurityEventSeverity::info;
    }
    if (value == "warning") {
        return SecurityEventSeverity::warning;
    }
    if (value == "error") {
        return SecurityEventSeverity::error;
    }
    if (value == "critical") {
        return SecurityEventSeverity::critical;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<AuditCompleteness> parse_audit_completeness_string(std::string_view value) {
    if (value == "complete") {
        return AuditCompleteness::complete;
    }
    if (value == "partial") {
        return AuditCompleteness::partial;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<IntegrityPlaceholderType> parse_integrity_placeholder_type_string(std::string_view value) {
    if (value == "none") {
        return IntegrityPlaceholderType::none;
    }
    if (value == "checksum_pending") {
        return IntegrityPlaceholderType::checksum_pending;
    }
    if (value == "signature_pending") {
        return IntegrityPlaceholderType::signature_pending;
    }
    return std::nullopt;
}

[[nodiscard]] bool is_known_peer_role(PeerRole role) noexcept {
    switch (role) {
        case PeerRole::pi_zymkey:
        case PeerRole::ubuntu_tpm:
            return true;
    }

    return false;
}

[[nodiscard]] bool is_known_event_type(SecurityEventType event_type) noexcept {
    switch (event_type) {
        case SecurityEventType::auth_session_authenticated:
        case SecurityEventType::trust_peer_validated:
        case SecurityEventType::acl_decision_recorded:
        case SecurityEventType::api_request_processed:
        case SecurityEventType::tamper_detected:
        case SecurityEventType::tamper_lockout:
        case SecurityEventType::recovery_started:
        case SecurityEventType::recovery_completed:
        case SecurityEventType::recovery_failed:
        case SecurityEventType::export_audit_exported:
            return true;
    }

    return false;
}

[[nodiscard]] bool is_known_event_severity(SecurityEventSeverity severity) noexcept {
    switch (severity) {
        case SecurityEventSeverity::info:
        case SecurityEventSeverity::warning:
        case SecurityEventSeverity::error:
        case SecurityEventSeverity::critical:
            return true;
    }

    return false;
}

[[nodiscard]] bool is_known_audit_completeness(AuditCompleteness completeness) noexcept {
    switch (completeness) {
        case AuditCompleteness::complete:
        case AuditCompleteness::partial:
            return true;
    }

    return false;
}

[[nodiscard]] bool is_known_integrity_placeholder_type(IntegrityPlaceholderType placeholder_type) noexcept {
    switch (placeholder_type) {
        case IntegrityPlaceholderType::none:
        case IntegrityPlaceholderType::checksum_pending:
        case IntegrityPlaceholderType::signature_pending:
            return true;
    }

    return false;
}

template <typename Enum, typename Parser>
[[nodiscard]] std::optional<Enum> parse_required_enum(const JsonValue& object,
                                                      std::string_view field_name,
                                                      std::string_view path,
                                                      std::vector<Diagnostic>& diagnostics,
                                                      const char* invalid_code,
                                                      Parser&& parser) {
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

[[nodiscard]] std::optional<std::string> parse_required_uuid(const JsonValue& object,
                                                             std::string_view field_name,
                                                             std::string_view path,
                                                             std::vector<Diagnostic>& diagnostics) {
    const auto value = required_string_field(object, field_name, path, diagnostics);
    if (!value.has_value()) {
        return std::nullopt;
    }

    if (!is_uuid_like(*value)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_identifier",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must be a canonical UUID string");
        return std::nullopt;
    }

    return lower_copy(*value);
}

[[nodiscard]] std::optional<std::string> parse_optional_uuid(const JsonValue& object,
                                                             std::string_view field_name,
                                                             std::string_view path,
                                                             std::vector<Diagnostic>& diagnostics) {
    const auto value = optional_string_field(object, field_name, path, diagnostics);
    if (!value.has_value()) {
        return std::nullopt;
    }

    if (!is_uuid_like(*value)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_identifier",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must be a canonical UUID string");
        return std::nullopt;
    }

    return lower_copy(*value);
}

[[nodiscard]] std::optional<std::string> parse_required_utc_timestamp(const JsonValue& object,
                                                                      std::string_view field_name,
                                                                      std::string_view path,
                                                                      std::vector<Diagnostic>& diagnostics,
                                                                      const char* invalid_code) {
    const auto value = required_string_field(object, field_name, path, diagnostics);
    if (!value.has_value()) {
        return std::nullopt;
    }

    if (!is_utc_timestamp(*value)) {
        add_diagnostic(diagnostics,
                       invalid_code,
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must use UTC ISO-8601");
        return std::nullopt;
    }

    return *value;
}

[[nodiscard]] std::optional<IdentitySummary> parse_identity_summary(const JsonValue& object,
                                                                    std::string_view path,
                                                                    std::vector<Diagnostic>& diagnostics) {
    if (object.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Identity summary must be an object");
        return std::nullopt;
    }

    const auto role = parse_required_enum<PeerRole>(
        object, "role", std::string{path} + "/role", diagnostics, "audit.invalid_identity_role", parse_peer_role_string);
    const auto device_id = required_string_field(object, "device_id", std::string{path} + "/device_id", diagnostics);
    const auto certificate_fingerprint =
        required_string_field(object, "certificate_fingerprint", std::string{path} + "/certificate_fingerprint", diagnostics);

    if (certificate_fingerprint.has_value()) {
        if (!is_hex_string(*certificate_fingerprint) ||
            *certificate_fingerprint != lower_copy(*certificate_fingerprint)) {
            add_diagnostic(diagnostics,
                           "audit.invalid_fingerprint",
                           DiagnosticSeverity::error,
                           std::string{path} + "/certificate_fingerprint",
                           "Fingerprint must be a lowercase hex string");
            return std::nullopt;
        }
    }

    if (!role.has_value() || !device_id.has_value() || !certificate_fingerprint.has_value()) {
        return std::nullopt;
    }

    return IdentitySummary{
        .role = *role,
        .device_id = *device_id,
        .certificate_fingerprint = *certificate_fingerprint,
    };
}

[[nodiscard]] std::optional<DecisionSummary> parse_decision_summary(const JsonValue& object,
                                                                    std::string_view path,
                                                                    std::vector<Diagnostic>& diagnostics) {
    if (object.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Decision summary must be an object");
        return std::nullopt;
    }

    const auto decision = required_string_field(object, "decision", std::string{path} + "/decision", diagnostics);
    if (!decision.has_value()) {
        return std::nullopt;
    }

    if (decision->empty()) {
        add_diagnostic(diagnostics,
                       "audit.invalid_value",
                       DiagnosticSeverity::error,
                       std::string{path} + "/decision",
                       "Decision summary must not be empty");
        return std::nullopt;
    }

    return DecisionSummary{
        .decision = *decision,
    };
}

[[nodiscard]] std::optional<ResultSummary> parse_result_summary(const JsonValue& object,
                                                                std::string_view path,
                                                                std::vector<Diagnostic>& diagnostics) {
    if (object.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Result summary must be an object");
        return std::nullopt;
    }

    const auto result = required_string_field(object, "result", std::string{path} + "/result", diagnostics);
    if (!result.has_value()) {
        return std::nullopt;
    }

    if (result->empty()) {
        add_diagnostic(diagnostics,
                       "audit.invalid_value",
                       DiagnosticSeverity::error,
                       std::string{path} + "/result",
                       "Result summary must not be empty");
        return std::nullopt;
    }

    return ResultSummary{
        .result = *result,
    };
}

enum class OptionalFieldState {
    absent,
    present_valid,
    present_invalid,
};

struct SecurityEventFieldState {
    OptionalFieldState actor_identity{OptionalFieldState::absent};
    OptionalFieldState peer_identity{OptionalFieldState::absent};
    OptionalFieldState decision_summary{OptionalFieldState::absent};
    OptionalFieldState result_summary{OptionalFieldState::absent};
};

struct OptionalObjectLookup {
    const JsonValue* value{nullptr};
    OptionalFieldState state{OptionalFieldState::absent};
};

[[nodiscard]] bool event_requires_actor_identity(SecurityEventType event_type) {
    return event_type == SecurityEventType::auth_session_authenticated ||
           event_type == SecurityEventType::acl_decision_recorded ||
           event_type == SecurityEventType::api_request_processed;
}

[[nodiscard]] bool event_requires_peer_identity(SecurityEventType event_type) {
    return event_type == SecurityEventType::auth_session_authenticated ||
           event_type == SecurityEventType::trust_peer_validated;
}

[[nodiscard]] bool event_requires_decision_summary(SecurityEventType event_type) {
    return event_type == SecurityEventType::auth_session_authenticated ||
           event_type == SecurityEventType::trust_peer_validated ||
           event_type == SecurityEventType::acl_decision_recorded;
}

[[nodiscard]] bool event_requires_result_summary(SecurityEventType event_type) {
    return event_type == SecurityEventType::api_request_processed ||
           event_type == SecurityEventType::tamper_detected ||
           event_type == SecurityEventType::tamper_lockout ||
           event_type == SecurityEventType::recovery_started ||
           event_type == SecurityEventType::recovery_completed ||
           event_type == SecurityEventType::recovery_failed ||
           event_type == SecurityEventType::export_audit_exported;
}

[[nodiscard]] bool event_requires_request_id(SecurityEventType event_type) {
    return event_type == SecurityEventType::api_request_processed;
}

[[nodiscard]] bool event_requires_export_id_ref(SecurityEventType event_type) {
    return event_type == SecurityEventType::export_audit_exported;
}

[[nodiscard]] std::string child_path(std::string_view base_path, std::string_view field_name);
void validate_event_semantics(const SecurityEvent& event,
                              std::string_view path,
                              const SecurityEventFieldState& field_state,
                              std::vector<Diagnostic>& diagnostics);

template <typename T>
[[nodiscard]] OptionalFieldState field_state_for(const std::optional<T>& field) {
    return field.has_value() ? OptionalFieldState::present_valid : OptionalFieldState::absent;
}

[[nodiscard]] SecurityEventFieldState field_state_for(const SecurityEvent& event) {
    return SecurityEventFieldState{
        .actor_identity = field_state_for(event.actor_identity),
        .peer_identity = field_state_for(event.peer_identity),
        .decision_summary = field_state_for(event.decision_summary),
        .result_summary = field_state_for(event.result_summary),
    };
}

[[nodiscard]] OptionalObjectLookup find_optional_object_field(const JsonValue& object,
                                                              std::string_view field_name,
                                                              std::string_view path,
                                                              std::vector<Diagnostic>& diagnostics) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        return {};
    }

    if (field->type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Field must be an object");
        return OptionalObjectLookup{
            .value = nullptr,
            .state = OptionalFieldState::present_invalid,
        };
    }

    return OptionalObjectLookup{
        .value = field,
        .state = OptionalFieldState::present_valid,
    };
}

void validate_identity_summary_object(const IdentitySummary& identity,
                                      std::string_view path,
                                      std::vector<Diagnostic>& diagnostics) {
    if (!is_known_peer_role(identity.role)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_identity_role",
                       DiagnosticSeverity::error,
                       child_path(path, "role"),
                       "Field has an unsupported enum value");
    }

    if (!is_hex_string(identity.certificate_fingerprint) ||
        identity.certificate_fingerprint != lower_copy(identity.certificate_fingerprint)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_fingerprint",
                       DiagnosticSeverity::error,
                       child_path(path, "certificate_fingerprint"),
                       "Fingerprint must be a lowercase hex string");
    }
}

void validate_decision_summary_object(const DecisionSummary& summary,
                                      std::string_view path,
                                      std::vector<Diagnostic>& diagnostics) {
    if (summary.decision.empty()) {
        add_diagnostic(diagnostics,
                       "audit.invalid_value",
                       DiagnosticSeverity::error,
                       child_path(path, "decision"),
                       "Decision summary must not be empty");
    }
}

void validate_result_summary_object(const ResultSummary& summary,
                                    std::string_view path,
                                    std::vector<Diagnostic>& diagnostics) {
    if (summary.result.empty()) {
        add_diagnostic(diagnostics,
                       "audit.invalid_value",
                       DiagnosticSeverity::error,
                       child_path(path, "result"),
                       "Result summary must not be empty");
    }
}

void validate_security_event_object(const SecurityEvent& event,
                                    std::string_view path,
                                    const SecurityEventFieldState& field_state,
                                    std::vector<Diagnostic>& diagnostics) {
    if (!is_uuid_like(event.event_id)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_identifier",
                       DiagnosticSeverity::error,
                       child_path(path, "event_id"),
                       "Field must be a canonical UUID string");
    }

    if (!is_known_event_type(event.event_type)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_event_type",
                       DiagnosticSeverity::error,
                       child_path(path, "event_type"),
                       "Field has an unsupported enum value");
    }

    if (!is_known_event_severity(event.severity)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_severity",
                       DiagnosticSeverity::error,
                       child_path(path, "severity"),
                       "Field has an unsupported enum value");
    }

    if (!is_utc_timestamp(event.event_time_utc)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_event_time",
                       DiagnosticSeverity::error,
                       child_path(path, "event_time_utc"),
                       "Field must use UTC ISO-8601");
    }

    if (!is_known_peer_role(event.producer_role)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_producer_role",
                       DiagnosticSeverity::error,
                       child_path(path, "producer_role"),
                       "Field has an unsupported enum value");
    }

    if (event.request_id.has_value() && !is_uuid_like(*event.request_id)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_identifier",
                       DiagnosticSeverity::error,
                       child_path(path, "request_id"),
                       "Field must be a canonical UUID string");
    }

    if (event.export_id_ref.has_value() && !is_uuid_like(*event.export_id_ref)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_identifier",
                       DiagnosticSeverity::error,
                       child_path(path, "export_id_ref"),
                       "Field must be a canonical UUID string");
    }

    if (event.actor_identity.has_value()) {
        validate_identity_summary_object(*event.actor_identity, child_path(path, "actor_identity"), diagnostics);
    }

    if (event.peer_identity.has_value()) {
        validate_identity_summary_object(*event.peer_identity, child_path(path, "peer_identity"), diagnostics);
    }

    if (event.decision_summary.has_value()) {
        validate_decision_summary_object(*event.decision_summary, child_path(path, "decision_summary"), diagnostics);
    }

    if (event.result_summary.has_value()) {
        validate_result_summary_object(*event.result_summary, child_path(path, "result_summary"), diagnostics);
    }

    validate_event_semantics(event, path, field_state, diagnostics);
}

void validate_security_event_object(const SecurityEvent& event,
                                    std::string_view path,
                                    std::vector<Diagnostic>& diagnostics) {
    validate_security_event_object(event, path, field_state_for(event), diagnostics);
}

void validate_event_semantics(const SecurityEvent& event,
                              std::string_view path,
                              const SecurityEventFieldState& field_state,
                              std::vector<Diagnostic>& diagnostics) {
    if (event_requires_actor_identity(event.event_type) && field_state.actor_identity == OptionalFieldState::absent) {
        add_diagnostic(diagnostics,
                       "audit.missing_required_field",
                       DiagnosticSeverity::error,
                       child_path(path, "actor_identity"),
                       "Missing required audit field");
    }

    if (event_requires_peer_identity(event.event_type) && field_state.peer_identity == OptionalFieldState::absent) {
        add_diagnostic(diagnostics,
                       "audit.missing_required_field",
                       DiagnosticSeverity::error,
                       child_path(path, "peer_identity"),
                       "Missing required audit field");
    }

    if (event_requires_decision_summary(event.event_type) && field_state.decision_summary == OptionalFieldState::absent) {
        add_diagnostic(diagnostics,
                       "audit.missing_required_field",
                       DiagnosticSeverity::error,
                       child_path(path, "decision_summary"),
                       "Missing required audit field");
    }

    if (event_requires_result_summary(event.event_type) && field_state.result_summary == OptionalFieldState::absent) {
        add_diagnostic(diagnostics,
                       "audit.missing_required_field",
                       DiagnosticSeverity::error,
                       child_path(path, "result_summary"),
                       "Missing required audit field");
    }

    if (event_requires_request_id(event.event_type) && !event.request_id.has_value()) {
        add_diagnostic(diagnostics,
                       "audit.missing_required_field",
                       DiagnosticSeverity::error,
                       child_path(path, "request_id"),
                       "Missing required audit field");
    }

    if (event_requires_export_id_ref(event.event_type) && !event.export_id_ref.has_value()) {
        add_diagnostic(diagnostics,
                       "audit.missing_required_field",
                       DiagnosticSeverity::error,
                       child_path(path, "export_id_ref"),
                       "Missing required audit field");
    }
}

[[nodiscard]] std::optional<SecurityEvent> build_security_event(const JsonValue& root,
                                                                std::string_view path,
                                                                std::vector<Diagnostic>& diagnostics) {
    if (root.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "SecurityEvent must be an object");
        return std::nullopt;
    }

    const auto event_id = parse_required_uuid(root, "event_id", child_path(path, "event_id"), diagnostics);
    const auto event_type = parse_required_enum<SecurityEventType>(
        root, "event_type", child_path(path, "event_type"), diagnostics, "audit.invalid_event_type", parse_event_type_string);
    const auto severity = parse_required_enum<SecurityEventSeverity>(root,
                                                                     "severity",
                                                                     child_path(path, "severity"),
                                                                     diagnostics,
                                                                     "audit.invalid_severity",
                                                                     parse_event_severity_string);
    const auto event_time_utc = parse_required_utc_timestamp(
        root, "event_time_utc", child_path(path, "event_time_utc"), diagnostics, "audit.invalid_event_time");
    const auto producer_role = parse_required_enum<PeerRole>(root,
                                                             "producer_role",
                                                             child_path(path, "producer_role"),
                                                             diagnostics,
                                                             "audit.invalid_producer_role",
                                                             parse_peer_role_string);
    const auto correlation_id = optional_string_field(root, "correlation_id", child_path(path, "correlation_id"), diagnostics);
    const auto request_id = parse_optional_uuid(root, "request_id", child_path(path, "request_id"), diagnostics);
    const auto export_id_ref = parse_optional_uuid(root, "export_id_ref", child_path(path, "export_id_ref"), diagnostics);
    const auto payload_metadata = required_string_map_field(root, "payload_metadata", child_path(path, "payload_metadata"), diagnostics);

    std::optional<IdentitySummary> actor_identity;
    SecurityEventFieldState field_state;
    const auto actor_identity_path = child_path(path, "actor_identity");
    if (const auto lookup = find_optional_object_field(root, "actor_identity", actor_identity_path, diagnostics);
        lookup.value != nullptr) {
        field_state.actor_identity = lookup.state;
        actor_identity = parse_identity_summary(*lookup.value, actor_identity_path, diagnostics);
        if (!actor_identity.has_value()) {
            field_state.actor_identity = OptionalFieldState::present_invalid;
        }
    } else {
        field_state.actor_identity = lookup.state;
    }

    std::optional<IdentitySummary> peer_identity;
    const auto peer_identity_path = child_path(path, "peer_identity");
    if (const auto lookup = find_optional_object_field(root, "peer_identity", peer_identity_path, diagnostics);
        lookup.value != nullptr) {
        field_state.peer_identity = lookup.state;
        peer_identity = parse_identity_summary(*lookup.value, peer_identity_path, diagnostics);
        if (!peer_identity.has_value()) {
            field_state.peer_identity = OptionalFieldState::present_invalid;
        }
    } else {
        field_state.peer_identity = lookup.state;
    }

    std::optional<DecisionSummary> decision_summary;
    const auto decision_summary_path = child_path(path, "decision_summary");
    if (const auto lookup = find_optional_object_field(root, "decision_summary", decision_summary_path, diagnostics);
        lookup.value != nullptr) {
        field_state.decision_summary = lookup.state;
        decision_summary = parse_decision_summary(*lookup.value, decision_summary_path, diagnostics);
        if (!decision_summary.has_value()) {
            field_state.decision_summary = OptionalFieldState::present_invalid;
        }
    } else {
        field_state.decision_summary = lookup.state;
    }

    std::optional<ResultSummary> result_summary;
    const auto result_summary_path = child_path(path, "result_summary");
    if (const auto lookup = find_optional_object_field(root, "result_summary", result_summary_path, diagnostics);
        lookup.value != nullptr) {
        field_state.result_summary = lookup.state;
        result_summary = parse_result_summary(*lookup.value, result_summary_path, diagnostics);
        if (!result_summary.has_value()) {
            field_state.result_summary = OptionalFieldState::present_invalid;
        }
    } else {
        field_state.result_summary = lookup.state;
    }

    if (!event_id.has_value() || !event_type.has_value() || !severity.has_value() || !event_time_utc.has_value() ||
        !producer_role.has_value() || !payload_metadata.has_value()) {
        return std::nullopt;
    }

    SecurityEvent event{
        .event_id = *event_id,
        .event_type = *event_type,
        .severity = *severity,
        .event_time_utc = *event_time_utc,
        .producer_role = *producer_role,
        .actor_identity = std::move(actor_identity),
        .peer_identity = std::move(peer_identity),
        .correlation_id = std::move(correlation_id),
        .request_id = std::move(request_id),
        .export_id_ref = std::move(export_id_ref),
        .payload_metadata = *payload_metadata,
        .decision_summary = std::move(decision_summary),
        .result_summary = std::move(result_summary),
    };

    validate_security_event_object(event, path, field_state, diagnostics);

    if (validation_status_for(diagnostics) == ValidationStatus::error) {
        return std::nullopt;
    }

    return event;
}

[[nodiscard]] std::optional<OrderingRules> parse_ordering_rules(const JsonValue& object,
                                                                std::string_view path,
                                                                std::vector<Diagnostic>& diagnostics) {
    if (object.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Ordering rules must be an object");
        return std::nullopt;
    }

    const auto ordered_by = required_string_field(object, "ordered_by", std::string{path} + "/ordered_by", diagnostics);
    const auto direction = required_string_field(object, "direction", std::string{path} + "/direction", diagnostics);

    if (!ordered_by.has_value() || !direction.has_value()) {
        return std::nullopt;
    }

    if (*ordered_by != "event_time_utc,event_id") {
        add_diagnostic(diagnostics,
                       "audit.export_ordering_mismatch",
                       DiagnosticSeverity::error,
                       std::string{path} + "/ordered_by",
                       "ordered_by must remain event_time_utc,event_id");
        return std::nullopt;
    }

    if (*direction != "ascending") {
        add_diagnostic(diagnostics,
                       "audit.export_ordering_mismatch",
                       DiagnosticSeverity::error,
                       std::string{path} + "/direction",
                       "direction must remain ascending");
        return std::nullopt;
    }

    return OrderingRules{
        .ordered_by = *ordered_by,
        .direction = *direction,
    };
}

[[nodiscard]] std::optional<ExportMetadata> parse_export_metadata(const JsonValue& object,
                                                                  std::string_view path,
                                                                  std::vector<Diagnostic>& diagnostics) {
    if (object.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Export metadata must be an object");
        return std::nullopt;
    }

    const auto completeness = parse_required_enum<AuditCompleteness>(object,
                                                                     "completeness",
                                                                     std::string{path} + "/completeness",
                                                                     diagnostics,
                                                                     "audit.invalid_export_completeness",
                                                                     parse_audit_completeness_string);
    const auto export_reason =
        required_string_field(object, "export_reason", std::string{path} + "/export_reason", diagnostics);
    const auto source_channel_id =
        required_string_field(object, "source_channel_id", std::string{path} + "/source_channel_id", diagnostics);

    if (!completeness.has_value() || !export_reason.has_value() || !source_channel_id.has_value()) {
        return std::nullopt;
    }

    return ExportMetadata{
        .completeness = *completeness,
        .export_reason = *export_reason,
        .source_channel_id = *source_channel_id,
    };
}

[[nodiscard]] std::optional<IntegrityMetadata> parse_integrity_metadata(const JsonValue& object,
                                                                        std::string_view path,
                                                                        std::vector<Diagnostic>& diagnostics) {
    if (object.type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       std::string{path},
                       "Integrity metadata must be an object");
        return std::nullopt;
    }

    const auto placeholder_type = parse_required_enum<IntegrityPlaceholderType>(object,
                                                                                "placeholder_type",
                                                                                std::string{path} + "/placeholder_type",
                                                                                diagnostics,
                                                                                "audit.invalid_integrity_placeholder",
                                                                                parse_integrity_placeholder_type_string);
    const auto placeholder_ref =
        optional_string_field(object, "placeholder_ref", std::string{path} + "/placeholder_ref", diagnostics);

    if (!placeholder_type.has_value()) {
        return std::nullopt;
    }

    if (*placeholder_type != IntegrityPlaceholderType::none &&
        (!placeholder_ref.has_value() || placeholder_ref->empty())) {
        add_diagnostic(diagnostics,
                       "audit.missing_required_field",
                       DiagnosticSeverity::error,
                       std::string{path} + "/placeholder_ref",
                       "Missing required audit field");
        return std::nullopt;
    }

    return IntegrityMetadata{
        .placeholder_type = *placeholder_type,
        .placeholder_ref = placeholder_ref,
    };
}

[[nodiscard]] bool event_less(const SecurityEvent& lhs, const SecurityEvent& rhs) {
    return std::tie(lhs.event_time_utc, lhs.event_id) < std::tie(rhs.event_time_utc, rhs.event_id);
}

[[nodiscard]] std::string child_path(std::string_view base_path, std::string_view field_name);

[[nodiscard]] std::string child_path(std::string_view base_path, std::string_view field_name) {
    if (base_path == "/") {
        return "/" + std::string{field_name};
    }

    return std::string{base_path} + "/" + std::string{field_name};
}

void append_string_map(std::ostringstream& output,
                       const std::map<std::string, std::string>& values,
                       std::size_t indent_level) {
    if (values.empty()) {
        output << "{}";
        return;
    }

    output << "{\n";
    for (auto iterator = values.begin(); iterator != values.end(); ++iterator) {
        append_indent(output, indent_level + 1U);
        output << "\"" << json_escape(iterator->first) << "\": \"" << json_escape(iterator->second) << "\"";
        if (std::next(iterator) != values.end()) {
            output << ",";
        }
        output << "\n";
    }
    append_indent(output, indent_level);
    output << "}";
}

void append_identity_summary(std::ostringstream& output, const IdentitySummary& identity, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"role\": \"" << secure_channel::to_string(identity.role) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"device_id\": \"" << json_escape(identity.device_id) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"certificate_fingerprint\": \"" << json_escape(identity.certificate_fingerprint) << "\"\n";
    append_indent(output, indent_level);
    output << "}";
}

void append_decision_summary(std::ostringstream& output, const DecisionSummary& summary, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"decision\": \"" << json_escape(summary.decision) << "\"\n";
    append_indent(output, indent_level);
    output << "}";
}

void append_result_summary(std::ostringstream& output, const ResultSummary& summary, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"result\": \"" << json_escape(summary.result) << "\"\n";
    append_indent(output, indent_level);
    output << "}";
}

void append_ordering_rules(std::ostringstream& output, const OrderingRules& ordering, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"ordered_by\": \"" << json_escape(ordering.ordered_by) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"direction\": \"" << json_escape(ordering.direction) << "\"\n";
    append_indent(output, indent_level);
    output << "}";
}

void append_export_metadata(std::ostringstream& output, const ExportMetadata& metadata, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"completeness\": \"" << to_string(metadata.completeness) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"export_reason\": \"" << json_escape(metadata.export_reason) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"source_channel_id\": \"" << json_escape(metadata.source_channel_id) << "\"\n";
    append_indent(output, indent_level);
    output << "}";
}

void append_integrity_metadata(std::ostringstream& output, const IntegrityMetadata& metadata, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"placeholder_type\": \"" << to_string(metadata.placeholder_type) << "\"";
    if (metadata.placeholder_ref.has_value()) {
        output << ",\n";
        append_indent(output, indent_level + 1U);
        output << "\"placeholder_ref\": \"" << json_escape(*metadata.placeholder_ref) << "\"\n";
    } else {
        output << "\n";
    }
    append_indent(output, indent_level);
    output << "}";
}

void append_security_event(std::ostringstream& output, const SecurityEvent& event, std::size_t indent_level) {
    output << "{\n";
    append_indent(output, indent_level + 1U);
    output << "\"event_id\": \"" << json_escape(event.event_id) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"event_type\": \"" << to_string(event.event_type) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"severity\": \"" << to_string(event.severity) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"event_time_utc\": \"" << json_escape(event.event_time_utc) << "\",\n";
    append_indent(output, indent_level + 1U);
    output << "\"producer_role\": \"" << secure_channel::to_string(event.producer_role) << "\"";

    if (event.actor_identity.has_value()) {
        output << ",\n";
        append_indent(output, indent_level + 1U);
        output << "\"actor_identity\": ";
        append_identity_summary(output, *event.actor_identity, indent_level + 1U);
    }

    if (event.peer_identity.has_value()) {
        output << ",\n";
        append_indent(output, indent_level + 1U);
        output << "\"peer_identity\": ";
        append_identity_summary(output, *event.peer_identity, indent_level + 1U);
    }

    if (event.correlation_id.has_value()) {
        output << ",\n";
        append_indent(output, indent_level + 1U);
        output << "\"correlation_id\": \"" << json_escape(*event.correlation_id) << "\"";
    }

    if (event.request_id.has_value()) {
        output << ",\n";
        append_indent(output, indent_level + 1U);
        output << "\"request_id\": \"" << json_escape(*event.request_id) << "\"";
    }

    if (event.export_id_ref.has_value()) {
        output << ",\n";
        append_indent(output, indent_level + 1U);
        output << "\"export_id_ref\": \"" << json_escape(*event.export_id_ref) << "\"";
    }

    output << ",\n";
    append_indent(output, indent_level + 1U);
    output << "\"payload_metadata\": ";
    append_string_map(output, event.payload_metadata, indent_level + 1U);

    if (event.decision_summary.has_value()) {
        output << ",\n";
        append_indent(output, indent_level + 1U);
        output << "\"decision_summary\": ";
        append_decision_summary(output, *event.decision_summary, indent_level + 1U);
    }

    if (event.result_summary.has_value()) {
        output << ",\n";
        append_indent(output, indent_level + 1U);
        output << "\"result_summary\": ";
        append_result_summary(output, *event.result_summary, indent_level + 1U);
    }

    output << "\n";
    append_indent(output, indent_level);
    output << "}";
}

[[nodiscard]] ParseResult<SecurityEvent> make_parse_error(std::string_view code, std::string path, std::string message) {
    return ParseResult<SecurityEvent>{
        .value = std::nullopt,
        .diagnostics =
            {
                Diagnostic{
                    .code = std::string{code},
                    .severity = DiagnosticSeverity::error,
                    .path = std::move(path),
                    .message = std::move(message),
                },
            },
        .validation_status = ValidationStatus::error,
    };
}

[[nodiscard]] ParseResult<AuditExport> make_export_parse_error(std::string_view code, std::string path, std::string message) {
    return ParseResult<AuditExport>{
        .value = std::nullopt,
        .diagnostics =
            {
                Diagnostic{
                    .code = std::string{code},
                    .severity = DiagnosticSeverity::error,
                    .path = std::move(path),
                    .message = std::move(message),
                },
            },
        .validation_status = ValidationStatus::error,
    };
}

}  // namespace

const char* to_string(SecurityEventType event_type) noexcept {
    switch (event_type) {
        case SecurityEventType::auth_session_authenticated:
            return "auth.session_authenticated";
        case SecurityEventType::trust_peer_validated:
            return "trust.peer_validated";
        case SecurityEventType::acl_decision_recorded:
            return "acl.decision_recorded";
        case SecurityEventType::api_request_processed:
            return "api.request_processed";
        case SecurityEventType::tamper_detected:
            return "tamper.tamper_detected";
        case SecurityEventType::tamper_lockout:
            return "tamper.tamper_lockout";
        case SecurityEventType::recovery_started:
            return "recovery.recovery_started";
        case SecurityEventType::recovery_completed:
            return "recovery.recovery_completed";
        case SecurityEventType::recovery_failed:
            return "recovery.recovery_failed";
        case SecurityEventType::export_audit_exported:
            return "export.audit_exported";
    }

    return "auth.session_authenticated";
}

const char* to_string(SecurityEventSeverity severity) noexcept {
    switch (severity) {
        case SecurityEventSeverity::info:
            return "info";
        case SecurityEventSeverity::warning:
            return "warning";
        case SecurityEventSeverity::error:
            return "error";
        case SecurityEventSeverity::critical:
            return "critical";
    }

    return "info";
}

const char* to_string(AuditCompleteness completeness) noexcept {
    switch (completeness) {
        case AuditCompleteness::complete:
            return "complete";
        case AuditCompleteness::partial:
            return "partial";
    }

    return "complete";
}

const char* to_string(IntegrityPlaceholderType placeholder_type) noexcept {
    switch (placeholder_type) {
        case IntegrityPlaceholderType::none:
            return "none";
        case IntegrityPlaceholderType::checksum_pending:
            return "checksum_pending";
        case IntegrityPlaceholderType::signature_pending:
            return "signature_pending";
    }

    return "none";
}

ParseResult<SecurityEvent> parse_security_event(std::string_view json) {
    JsonParseError error;
    JsonParser parser(json);
    const auto root = parser.parse(error);
    if (!root.has_value()) {
        return make_parse_error("audit.json_malformed", std::move(error.path), std::move(error.message));
    }

    std::vector<Diagnostic> diagnostics;
    const auto event = build_security_event(*root, "/", diagnostics);
    sort_diagnostics(diagnostics);
    const auto status = validation_status_for(diagnostics);

    return ParseResult<SecurityEvent>{
        .value = event,
        .diagnostics = std::move(diagnostics),
        .validation_status = status,
    };
}

CheckResult validate_audit_export(const AuditExport& audit_export) {
    std::vector<Diagnostic> diagnostics;

    if (!is_uuid_like(audit_export.export_id)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_identifier",
                       DiagnosticSeverity::error,
                       "/export_id",
                       "Field must be a canonical UUID string");
    }

    if (!is_utc_timestamp(audit_export.export_time_utc)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_export_time",
                       DiagnosticSeverity::error,
                       "/export_time_utc",
                       "Field must use UTC ISO-8601");
    }

    if (audit_export.ordering.ordered_by != "event_time_utc,event_id") {
        add_diagnostic(diagnostics,
                       "audit.export_ordering_mismatch",
                       DiagnosticSeverity::error,
                       "/ordering/ordered_by",
                       "ordered_by must remain event_time_utc,event_id");
    }

    if (audit_export.ordering.direction != "ascending") {
        add_diagnostic(diagnostics,
                       "audit.export_ordering_mismatch",
                       DiagnosticSeverity::error,
                       "/ordering/direction",
                       "direction must remain ascending");
    }

    if (!is_known_audit_completeness(audit_export.export_metadata.completeness)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_export_completeness",
                       DiagnosticSeverity::error,
                       "/export_metadata/completeness",
                       "Field has an unsupported enum value");
    }

    if (!is_known_integrity_placeholder_type(audit_export.integrity_metadata.placeholder_type)) {
        add_diagnostic(diagnostics,
                       "audit.invalid_integrity_placeholder",
                       DiagnosticSeverity::error,
                       "/integrity_metadata/placeholder_type",
                       "Field has an unsupported enum value");
    }

    if (audit_export.integrity_metadata.placeholder_type != IntegrityPlaceholderType::none &&
        (!audit_export.integrity_metadata.placeholder_ref.has_value() ||
         audit_export.integrity_metadata.placeholder_ref->empty())) {
        add_diagnostic(diagnostics,
                       "audit.missing_required_field",
                       DiagnosticSeverity::error,
                       "/integrity_metadata/placeholder_ref",
                       "Missing required audit field");
    }

    std::set<std::string> seen_event_ids;
    for (std::size_t index = 0U; index < audit_export.events.size(); ++index) {
        const auto& event = audit_export.events[index];
        const std::string event_path = "/events[" + std::to_string(index + 1U) + "]";

        validate_security_event_object(event, event_path, diagnostics);

        if (!seen_event_ids.insert(event.event_id).second) {
            add_diagnostic(diagnostics,
                           "audit.duplicate_event_id",
                           DiagnosticSeverity::error,
                           event_path + "/event_id",
                           "event_id must be unique within one export");
        }

        if (index > 0U && event_less(event, audit_export.events[index - 1U])) {
            add_diagnostic(diagnostics,
                           "audit.export_ordering_mismatch",
                           DiagnosticSeverity::error,
                           event_path,
                           "events must be sorted by event_time_utc then event_id");
        }

        if (event.export_id_ref.has_value() && *event.export_id_ref != audit_export.export_id) {
            add_diagnostic(diagnostics,
                           "audit.invalid_event_export_linkage",
                           DiagnosticSeverity::error,
                           event_path + "/export_id_ref",
                           "event export reference must match export_id");
        }
    }

    enum class TamperState {
        clear,
        detected,
        locked_out,
        recovery_in_progress,
    };

    TamperState tamper_state = TamperState::clear;
    for (std::size_t index = 0U; index < audit_export.events.size(); ++index) {
        const auto& event = audit_export.events[index];
        const std::string event_path = "/events[" + std::to_string(index + 1U) + "]/event_type";

        switch (event.event_type) {
            case SecurityEventType::tamper_detected:
                tamper_state = TamperState::detected;
                break;
            case SecurityEventType::tamper_lockout:
                if (tamper_state != TamperState::detected) {
                    add_diagnostic(diagnostics,
                                   "audit.invalid_tamper_transition",
                                   DiagnosticSeverity::error,
                                   event_path,
                                   "tamper_lockout requires a prior tamper_detected");
                } else {
                    tamper_state = TamperState::locked_out;
                }
                break;
            case SecurityEventType::recovery_started:
                if (tamper_state != TamperState::detected && tamper_state != TamperState::locked_out) {
                    add_diagnostic(diagnostics,
                                   "audit.invalid_tamper_transition",
                                   DiagnosticSeverity::error,
                                   event_path,
                                   "recovery_started requires an active tamper state");
                } else {
                    tamper_state = TamperState::recovery_in_progress;
                }
                break;
            case SecurityEventType::recovery_completed:
                if (tamper_state != TamperState::recovery_in_progress) {
                    add_diagnostic(diagnostics,
                                   "audit.invalid_tamper_transition",
                                   DiagnosticSeverity::error,
                                   event_path,
                                   "recovery_completed requires recovery_started");
                } else {
                    tamper_state = TamperState::clear;
                }
                break;
            case SecurityEventType::recovery_failed:
                if (tamper_state != TamperState::recovery_in_progress) {
                    add_diagnostic(diagnostics,
                                   "audit.invalid_tamper_transition",
                                   DiagnosticSeverity::error,
                                   event_path,
                                   "recovery_failed requires recovery_started");
                } else {
                    tamper_state = TamperState::locked_out;
                }
                break;
            case SecurityEventType::auth_session_authenticated:
            case SecurityEventType::trust_peer_validated:
            case SecurityEventType::acl_decision_recorded:
            case SecurityEventType::api_request_processed:
            case SecurityEventType::export_audit_exported:
                break;
        }
    }

    sort_diagnostics(diagnostics);
    const auto status = validation_status_for(diagnostics);
    return CheckResult{
        .diagnostics = std::move(diagnostics),
        .validation_status = status,
    };
}

ParseResult<AuditExport> parse_audit_export(std::string_view json) {
    JsonParseError error;
    JsonParser parser(json);
    const auto root = parser.parse(error);
    if (!root.has_value()) {
        return make_export_parse_error("audit.json_malformed", std::move(error.path), std::move(error.message));
    }

    std::vector<Diagnostic> diagnostics;
    if (root->type != JsonValue::Type::object) {
        add_diagnostic(diagnostics,
                       "audit.invalid_type",
                       DiagnosticSeverity::error,
                       "/",
                       "AuditExport must be an object");
        return ParseResult<AuditExport>{
            .value = std::nullopt,
            .diagnostics = std::move(diagnostics),
            .validation_status = ValidationStatus::error,
        };
    }

    const auto export_id = parse_required_uuid(*root, "export_id", "/export_id", diagnostics);
    const auto export_time_utc =
        parse_required_utc_timestamp(*root, "export_time_utc", "/export_time_utc", diagnostics, "audit.invalid_export_time");

    std::optional<OrderingRules> ordering;
    if (const auto* value = required_object_field(*root, "ordering", "/ordering", diagnostics)) {
        ordering = parse_ordering_rules(*value, "/ordering", diagnostics);
    }

    std::optional<ExportMetadata> export_metadata;
    if (const auto* value = required_object_field(*root, "export_metadata", "/export_metadata", diagnostics)) {
        export_metadata = parse_export_metadata(*value, "/export_metadata", diagnostics);
    }

    std::optional<IntegrityMetadata> integrity_metadata;
    if (const auto* value = required_object_field(*root, "integrity_metadata", "/integrity_metadata", diagnostics)) {
        integrity_metadata = parse_integrity_metadata(*value, "/integrity_metadata", diagnostics);
    }

    std::vector<SecurityEvent> events;
    if (const auto* values = required_array_field(*root, "events", "/events", diagnostics)) {
        events.reserve(values->array_values.size());
        for (std::size_t index = 0U; index < values->array_values.size(); ++index) {
            const auto& item = values->array_values[index];
            const std::string item_path = "/events[" + std::to_string(index + 1U) + "]";
            const auto event = build_security_event(item, item_path, diagnostics);
            if (event.has_value()) {
                events.push_back(*event);
            }
        }
    }

    if (!export_id.has_value() || !export_time_utc.has_value() || !ordering.has_value() || !export_metadata.has_value() ||
        !integrity_metadata.has_value() || validation_status_for(diagnostics) == ValidationStatus::error) {
        sort_diagnostics(diagnostics);
        const auto status = validation_status_for(diagnostics);
        return ParseResult<AuditExport>{
            .value = std::nullopt,
            .diagnostics = std::move(diagnostics),
            .validation_status = status,
        };
    }

    AuditExport audit_export{
        .export_id = *export_id,
        .export_time_utc = *export_time_utc,
        .events = std::move(events),
        .ordering = *ordering,
        .export_metadata = *export_metadata,
        .integrity_metadata = *integrity_metadata,
    };

    auto export_check = validate_audit_export(audit_export);
    diagnostics.insert(diagnostics.end(), export_check.diagnostics.begin(), export_check.diagnostics.end());
    sort_diagnostics(diagnostics);
    const auto status = validation_status_for(diagnostics);

    return ParseResult<AuditExport>{
        .value = status == ValidationStatus::error ? std::nullopt : std::optional<AuditExport>{audit_export},
        .diagnostics = std::move(diagnostics),
        .validation_status = status,
    };
}

std::string to_json(const SecurityEvent& event) {
    std::ostringstream output;
    append_security_event(output, event, 0U);
    output << "\n";
    return output.str();
}

std::string to_json(const AuditExport& audit_export) {
    std::ostringstream output;
    output << "{\n";
    append_indent(output, 1U);
    output << "\"export_id\": \"" << json_escape(audit_export.export_id) << "\",\n";
    append_indent(output, 1U);
    output << "\"export_time_utc\": \"" << json_escape(audit_export.export_time_utc) << "\",\n";
    append_indent(output, 1U);
    output << "\"events\": ";
    if (audit_export.events.empty()) {
        output << "[]";
    } else {
        output << "[\n";
        for (std::size_t index = 0U; index < audit_export.events.size(); ++index) {
            append_indent(output, 2U);
            append_security_event(output, audit_export.events[index], 2U);
            if (index + 1U != audit_export.events.size()) {
                output << ",";
            }
            output << "\n";
        }
        append_indent(output, 1U);
        output << "]";
    }
    output << ",\n";
    append_indent(output, 1U);
    output << "\"ordering\": ";
    append_ordering_rules(output, audit_export.ordering, 1U);
    output << ",\n";
    append_indent(output, 1U);
    output << "\"export_metadata\": ";
    append_export_metadata(output, audit_export.export_metadata, 1U);
    output << ",\n";
    append_indent(output, 1U);
    output << "\"integrity_metadata\": ";
    append_integrity_metadata(output, audit_export.integrity_metadata, 1U);
    output << "\n";
    output << "}\n";
    return output.str();
}

}  // namespace dcplayer::security_api::audit
