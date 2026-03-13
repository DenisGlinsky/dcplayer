#include "secure_time.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace dcplayer::security_api::secure_time {
namespace {

struct JsonValue {
    enum class Type {
        null_value,
        boolean,
        string,
        number,
        array,
        object,
    };

    Type type{Type::null_value};
    bool boolean_value{false};
    std::string string_value;
    std::int64_t number_value{0};
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
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                return parse_number(std::move(path), error);
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
                    .path = path == "/" ? "/" + *key : path + "/" + *key,
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
            auto child = parse_value(path + "[" + std::to_string(index + 1U) + "]", error);
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
        auto string_value = parse_string(std::move(path), error);
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

    [[nodiscard]] std::optional<JsonValue> parse_number(std::string path, JsonParseError& error) {
        const bool negative = consume('-');
        if (position_ >= input_.size() || std::isdigit(static_cast<unsigned char>(input_[position_])) == 0) {
            error = JsonParseError{
                .path = std::move(path),
                .message = "expected integer number",
            };
            return std::nullopt;
        }

        std::uint64_t magnitude = 0U;
        while (position_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[position_])) != 0) {
            const auto digit = static_cast<unsigned>(input_[position_] - '0');
            if (magnitude > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U) {
                error = JsonParseError{
                    .path = std::move(path),
                    .message = "integer overflow",
                };
                return std::nullopt;
            }

            magnitude = magnitude * 10U + digit;
            ++position_;
        }

        if (position_ < input_.size() &&
            (input_[position_] == '.' || input_[position_] == 'e' || input_[position_] == 'E')) {
            error = JsonParseError{
                .path = std::move(path),
                .message = "unsupported number format",
            };
            return std::nullopt;
        }

        JsonValue value;
        value.type = JsonValue::Type::number;

        if (!negative) {
            if (magnitude > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                error = JsonParseError{
                    .path = std::move(path),
                    .message = "integer overflow",
                };
                return std::nullopt;
            }

            value.number_value = static_cast<std::int64_t>(magnitude);
            return value;
        }

        constexpr std::uint64_t min_abs =
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1U;
        if (magnitude > min_abs) {
            error = JsonParseError{
                .path = std::move(path),
                .message = "integer overflow",
            };
            return std::nullopt;
        }

        if (magnitude == min_abs) {
            value.number_value = std::numeric_limits<std::int64_t>::min();
            return value;
        }

        value.number_value = -static_cast<std::int64_t>(magnitude);
        return value;
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
                if (static_cast<unsigned char>(ch) < 0x20U) {
                    error = JsonParseError{
                        .path = std::move(path),
                        .message = "unescaped control character in string",
                    };
                    return std::nullopt;
                }

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
                case 'u': {
                    const auto parse_hex_digit = [](char value) -> std::optional<std::uint32_t> {
                        if (value >= '0' && value <= '9') {
                            return static_cast<std::uint32_t>(value - '0');
                        }
                        if (value >= 'a' && value <= 'f') {
                            return static_cast<std::uint32_t>(10 + (value - 'a'));
                        }
                        if (value >= 'A' && value <= 'F') {
                            return static_cast<std::uint32_t>(10 + (value - 'A'));
                        }
                        return std::nullopt;
                    };

                    const auto parse_code_unit = [&](std::string_view error_message) -> std::optional<std::uint32_t> {
                        if (position_ + 4U > input_.size()) {
                            error = JsonParseError{
                                .path = path,
                                .message = std::string(error_message),
                            };
                            return std::nullopt;
                        }

                        std::uint32_t code_unit = 0U;
                        for (std::size_t index = 0; index < 4U; ++index) {
                            const auto digit = parse_hex_digit(input_[position_ + index]);
                            if (!digit.has_value()) {
                                error = JsonParseError{
                                    .path = path,
                                    .message = std::string(error_message),
                                };
                                return std::nullopt;
                            }

                            code_unit = code_unit * 16U + *digit;
                        }

                        position_ += 4U;
                        return code_unit;
                    };

                    const auto append_utf8 = [](std::string& target, std::uint32_t code_point) {
                        if (code_point <= 0x7FU) {
                            target.push_back(static_cast<char>(code_point));
                            return;
                        }
                        if (code_point <= 0x7FFU) {
                            target.push_back(static_cast<char>(0xC0U | (code_point >> 6U)));
                            target.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
                            return;
                        }
                        if (code_point <= 0xFFFFU) {
                            target.push_back(static_cast<char>(0xE0U | (code_point >> 12U)));
                            target.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
                            target.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
                            return;
                        }

                        target.push_back(static_cast<char>(0xF0U | (code_point >> 18U)));
                        target.push_back(static_cast<char>(0x80U | ((code_point >> 12U) & 0x3FU)));
                        target.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
                        target.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
                    };

                    auto code_point = parse_code_unit("invalid unicode escape");
                    if (!code_point.has_value()) {
                        return std::nullopt;
                    }

                    if (*code_point >= 0xD800U && *code_point <= 0xDBFFU) {
                        if (position_ + 2U > input_.size() || input_[position_] != '\\' || input_[position_ + 1U] != 'u') {
                            error = JsonParseError{
                                .path = std::move(path),
                                .message = "invalid unicode surrogate pair",
                            };
                            return std::nullopt;
                        }

                        position_ += 2U;
                        const auto low_surrogate = parse_code_unit("invalid unicode surrogate pair");
                        if (!low_surrogate.has_value()) {
                            return std::nullopt;
                        }
                        if (*low_surrogate < 0xDC00U || *low_surrogate > 0xDFFFU) {
                            error = JsonParseError{
                                .path = std::move(path),
                                .message = "invalid unicode surrogate pair",
                            };
                            return std::nullopt;
                        }

                        *code_point = 0x10000U + (((*code_point - 0xD800U) << 10U) | (*low_surrogate - 0xDC00U));
                    } else if (*code_point >= 0xDC00U && *code_point <= 0xDFFFU) {
                        error = JsonParseError{
                            .path = std::move(path),
                            .message = "invalid unicode surrogate pair",
                        };
                        return std::nullopt;
                    }

                    append_utf8(output, *code_point);
                    break;
                }
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
        while (position_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[position_])) != 0) {
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

[[nodiscard]] std::string child_path(std::string_view parent, std::string_view field) {
    if (parent == "/") {
        return "/" + std::string(field);
    }
    return std::string(parent) + "/" + std::string(field);
}

void push_diagnostic(std::vector<Diagnostic>& diagnostics,
                     std::string_view code,
                     DiagnosticSeverity severity,
                     std::string path,
                     std::string message) {
    diagnostics.push_back(Diagnostic{
        .code = std::string(code),
        .severity = severity,
        .path = std::move(path),
        .message = std::move(message),
    });
}

[[nodiscard]] const JsonValue* find_field(const JsonValue& object, std::string_view field_name) {
    for (const auto& [key, value] : object.object_values) {
        if (key == field_name) {
            return &value;
        }
    }
    return nullptr;
}

[[nodiscard]] ValidationStatus validation_status_from_diagnostics(const std::vector<Diagnostic>& diagnostics) {
    bool has_warning = false;
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.severity == DiagnosticSeverity::error) {
            return ValidationStatus::error;
        }
        has_warning = true;
    }

    return has_warning ? ValidationStatus::warning : ValidationStatus::ok;
}

template <typename T>
[[nodiscard]] ParseResult<T> make_parse_result(std::optional<T> value, std::vector<Diagnostic> diagnostics) {
    const auto status = validation_status_from_diagnostics(diagnostics);
    if (status == ValidationStatus::error) {
        value.reset();
    }

    return ParseResult<T>{
        .value = std::move(value),
        .diagnostics = std::move(diagnostics),
        .validation_status = status,
    };
}

template <typename T>
[[nodiscard]] ParseResult<T> make_parse_error(std::string path, std::string message) {
    return ParseResult<T>{
        .value = std::nullopt,
        .diagnostics =
            {
                Diagnostic{
                    .code = "secure_time.json_malformed",
                    .severity = DiagnosticSeverity::error,
                    .path = std::move(path),
                    .message = std::move(message),
                },
            },
        .validation_status = ValidationStatus::error,
    };
}

[[nodiscard]] constexpr std::int64_t days_from_civil(int year, unsigned month, unsigned day) noexcept {
    year -= month <= 2U ? 1 : 0;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned adjusted_month = month > 2U ? month - 3U : month + 9U;
    const unsigned doy = (153U * adjusted_month + 2U) / 5U + day - 1U;
    const unsigned doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
    return static_cast<std::int64_t>(era) * 146097 + static_cast<std::int64_t>(doe) - 719468;
}

[[nodiscard]] bool is_leap_year(int year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

[[nodiscard]] unsigned days_in_month(int year, unsigned month) noexcept {
    switch (month) {
        case 1U:
        case 3U:
        case 5U:
        case 7U:
        case 8U:
        case 10U:
        case 12U:
            return 31U;
        case 4U:
        case 6U:
        case 9U:
        case 11U:
            return 30U;
        case 2U:
            return is_leap_year(year) ? 29U : 28U;
        default:
            return 0U;
    }
}

[[nodiscard]] std::optional<std::int64_t> parse_utc_timestamp(std::string_view value) noexcept {
    if (value.size() != 20U) {
        return std::nullopt;
    }

    const auto is_digit = [&](std::size_t index) {
        return std::isdigit(static_cast<unsigned char>(value[index])) != 0;
    };

    const std::size_t digit_indexes[] = {0U, 1U, 2U, 3U, 5U, 6U, 8U, 9U, 11U, 12U, 14U, 15U, 17U, 18U};
    for (const auto index : digit_indexes) {
        if (!is_digit(index)) {
            return std::nullopt;
        }
    }

    if (value[4] != '-' || value[7] != '-' || value[10] != 'T' || value[13] != ':' || value[16] != ':' ||
        value[19] != 'Z') {
        return std::nullopt;
    }

    const auto parse_component = [&](std::size_t offset, std::size_t length) {
        int parsed = 0;
        for (std::size_t index = 0U; index < length; ++index) {
            parsed = parsed * 10 + (value[offset + index] - '0');
        }
        return parsed;
    };

    const int year = parse_component(0U, 4U);
    const unsigned month = static_cast<unsigned>(parse_component(5U, 2U));
    const unsigned day = static_cast<unsigned>(parse_component(8U, 2U));
    const unsigned hour = static_cast<unsigned>(parse_component(11U, 2U));
    const unsigned minute = static_cast<unsigned>(parse_component(14U, 2U));
    const unsigned second = static_cast<unsigned>(parse_component(17U, 2U));

    if (month < 1U || month > 12U) {
        return std::nullopt;
    }
    if (day < 1U || day > days_in_month(year, month)) {
        return std::nullopt;
    }
    if (hour > 23U || minute > 59U || second > 59U) {
        return std::nullopt;
    }

    const auto days = days_from_civil(year, month, day);
    return days * 86400 + static_cast<std::int64_t>(hour) * 3600 + static_cast<std::int64_t>(minute) * 60 +
           static_cast<std::int64_t>(second);
}

template <typename Enum>
[[nodiscard]] bool contains_enum(const std::vector<Enum>& values, Enum value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

template <typename Enum>
[[nodiscard]] bool has_duplicates(const std::vector<Enum>& values) {
    std::vector<Enum> copy = values;
    std::sort(copy.begin(), copy.end(), [](Enum left, Enum right) {
        return static_cast<int>(left) < static_cast<int>(right);
    });
    return std::adjacent_find(copy.begin(), copy.end()) != copy.end();
}

[[nodiscard]] bool is_intrinsically_untrusted_source(TimeSource source) noexcept {
    switch (source) {
        case TimeSource::rtc_secure:
        case TimeSource::imported_secure_time:
            return false;
        case TimeSource::rtc_untrusted:
        case TimeSource::operator_set_time_placeholder:
        case TimeSource::unknown:
            return true;
    }

    return true;
}

[[nodiscard]] bool is_time_data_required(SecureTimeStatusCode status) noexcept {
    return status != SecureTimeStatusCode::unavailable;
}

[[nodiscard]] std::optional<SecureTimeStatusCode> parse_secure_time_status_code_string(std::string_view value) {
    if (value == "trusted") {
        return SecureTimeStatusCode::trusted;
    }
    if (value == "degraded") {
        return SecureTimeStatusCode::degraded;
    }
    if (value == "untrusted") {
        return SecureTimeStatusCode::untrusted;
    }
    if (value == "unavailable") {
        return SecureTimeStatusCode::unavailable;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<TimeSource> parse_time_source_string(std::string_view value) {
    if (value == "rtc_secure") {
        return TimeSource::rtc_secure;
    }
    if (value == "rtc_untrusted") {
        return TimeSource::rtc_untrusted;
    }
    if (value == "imported_secure_time") {
        return TimeSource::imported_secure_time;
    }
    if (value == "operator_set_time_placeholder") {
        return TimeSource::operator_set_time_placeholder;
    }
    if (value == "unknown") {
        return TimeSource::unknown;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<SourceRole> parse_source_role_string(std::string_view value) {
    if (value == "primary") {
        return SourceRole::primary;
    }
    if (value == "fallback") {
        return SourceRole::fallback;
    }
    if (value == "placeholder") {
        return SourceRole::placeholder;
    }
    if (value == "unknown") {
        return SourceRole::unknown;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<SourceTrustState> parse_source_trust_state_string(std::string_view value) {
    if (value == "trusted") {
        return SourceTrustState::trusted;
    }
    if (value == "untrusted") {
        return SourceTrustState::untrusted;
    }
    if (value == "unknown") {
        return SourceTrustState::unknown;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<DriftState> parse_drift_state_string(std::string_view value) {
    if (value == "within_policy") {
        return DriftState::within_policy;
    }
    if (value == "skew_exceeded") {
        return DriftState::skew_exceeded;
    }
    if (value == "unknown") {
        return DriftState::unknown;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<Freshness> parse_freshness_string(std::string_view value) {
    if (value == "fresh") {
        return Freshness::fresh;
    }
    if (value == "stale") {
        return Freshness::stale;
    }
    if (value == "unknown") {
        return Freshness::unknown;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<MonotonicityStatus> parse_monotonicity_status_string(std::string_view value) {
    if (value == "monotonic") {
        return MonotonicityStatus::monotonic;
    }
    if (value == "non_monotonic") {
        return MonotonicityStatus::non_monotonic;
    }
    if (value == "unknown") {
        return MonotonicityStatus::unknown;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<TamperTimeState> parse_tamper_time_state_string(std::string_view value) {
    if (value == "clear") {
        return TamperTimeState::clear;
    }
    if (value == "tamper_detected") {
        return TamperTimeState::tamper_detected;
    }
    if (value == "tamper_lockout") {
        return TamperTimeState::tamper_lockout;
    }
    if (value == "recovery_in_progress") {
        return TamperTimeState::recovery_in_progress;
    }
    return std::nullopt;
}

template <typename Enum>
[[nodiscard]] std::optional<Enum> parse_required_enum(const JsonValue& object,
                                                      std::string_view field_name,
                                                      std::string path,
                                                      std::vector<Diagnostic>& diagnostics,
                                                      std::string_view missing_code,
                                                      std::string_view invalid_code,
                                                      std::optional<Enum> (*parser)(std::string_view)) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        push_diagnostic(diagnostics,
                        missing_code,
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " is required");
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::string) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_type",
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " must be a string");
        return std::nullopt;
    }

    const auto parsed = parser(field->string_value);
    if (!parsed.has_value()) {
        push_diagnostic(diagnostics,
                        invalid_code,
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " contains an unsupported value");
        return std::nullopt;
    }

    return parsed;
}

[[nodiscard]] std::optional<std::string> parse_optional_string(const JsonValue& object,
                                                               std::string_view field_name,
                                                               std::string path,
                                                               std::vector<Diagnostic>& diagnostics) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr || field->type == JsonValue::Type::null_value) {
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::string) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_type",
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " must be a string");
        return std::nullopt;
    }

    return field->string_value;
}

[[nodiscard]] std::optional<std::string> parse_required_string(const JsonValue& object,
                                                               std::string_view field_name,
                                                               std::string path,
                                                               std::vector<Diagnostic>& diagnostics,
                                                               std::string_view missing_code) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        push_diagnostic(diagnostics,
                        missing_code,
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " is required");
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::string) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_type",
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " must be a string");
        return std::nullopt;
    }

    return field->string_value;
}

[[nodiscard]] std::optional<bool> parse_required_boolean(const JsonValue& object,
                                                         std::string_view field_name,
                                                         std::string path,
                                                         std::vector<Diagnostic>& diagnostics,
                                                         std::string_view missing_code) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        push_diagnostic(diagnostics,
                        missing_code,
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " is required");
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::boolean) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_type",
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " must be a boolean");
        return std::nullopt;
    }

    return field->boolean_value;
}

[[nodiscard]] std::optional<std::int64_t> parse_required_integer(const JsonValue& object,
                                                                 std::string_view field_name,
                                                                 std::string path,
                                                                 std::vector<Diagnostic>& diagnostics,
                                                                 std::string_view missing_code) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        push_diagnostic(diagnostics,
                        missing_code,
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " is required");
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::number) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_type",
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " must be an integer");
        return std::nullopt;
    }

    return field->number_value;
}

[[nodiscard]] std::optional<std::int64_t> parse_optional_integer(const JsonValue& object,
                                                                 std::string_view field_name,
                                                                 std::string path,
                                                                 std::vector<Diagnostic>& diagnostics) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr || field->type == JsonValue::Type::null_value) {
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::number) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_type",
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " must be an integer");
        return std::nullopt;
    }

    return field->number_value;
}

[[nodiscard]] std::optional<std::vector<TimeSource>> parse_required_time_source_array(const JsonValue& object,
                                                                                       std::string_view field_name,
                                                                                       std::string path,
                                                                                       std::vector<Diagnostic>& diagnostics,
                                                                                       std::string_view missing_code) {
    const auto* field = find_field(object, field_name);
    if (field == nullptr) {
        push_diagnostic(diagnostics,
                        missing_code,
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " is required");
        return std::nullopt;
    }

    if (field->type != JsonValue::Type::array) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_type",
                        DiagnosticSeverity::error,
                        std::move(path),
                        std::string(field_name) + " must be an array");
        return std::nullopt;
    }

    std::vector<TimeSource> sources;
    for (std::size_t index = 0; index < field->array_values.size(); ++index) {
        const auto item_path = path + "[" + std::to_string(index + 1U) + "]";
        const auto& item = field->array_values[index];
        if (item.type != JsonValue::Type::string) {
            push_diagnostic(diagnostics,
                            "secure_time.invalid_type",
                            DiagnosticSeverity::error,
                            item_path,
                            "allowed_time_sources entries must be strings");
            continue;
        }

        const auto parsed = parse_time_source_string(item.string_value);
        if (!parsed.has_value()) {
            push_diagnostic(diagnostics,
                            "secure_time.invalid_time_source",
                            DiagnosticSeverity::error,
                            item_path,
                            "allowed_time_sources contains an unsupported value");
            continue;
        }

        sources.push_back(*parsed);
    }

    return sources;
}

[[nodiscard]] std::optional<SecureClockPolicy> parse_secure_clock_policy_object(const JsonValue& root,
                                                                                std::vector<Diagnostic>& diagnostics) {
    if (root.type != JsonValue::Type::object) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_type",
                        DiagnosticSeverity::error,
                        "/",
                        "secure clock policy must be a JSON object");
        return std::nullopt;
    }

    const auto clock_id =
        parse_required_string(root, "clock_id", "/clock_id", diagnostics, "secure_time.missing_required_time_field");
    const auto max_allowed_skew_seconds = parse_required_integer(
        root, "max_allowed_skew_seconds", "/max_allowed_skew_seconds", diagnostics, "secure_time.missing_required_time_field");
    const auto freshness_window_seconds = parse_required_integer(
        root, "freshness_window_seconds", "/freshness_window_seconds", diagnostics, "secure_time.missing_required_time_field");
    const auto monotonic_required =
        parse_required_boolean(root, "monotonic_required", "/monotonic_required", diagnostics, "secure_time.missing_required_time_field");
    auto allowed_time_sources = parse_required_time_source_array(
        root, "allowed_time_sources", "/allowed_time_sources", diagnostics, "secure_time.missing_required_time_field");
    const auto fail_closed_on_untrusted_time = parse_required_boolean(root,
                                                                      "fail_closed_on_untrusted_time",
                                                                      "/fail_closed_on_untrusted_time",
                                                                      diagnostics,
                                                                      "secure_time.missing_required_time_field");

    if (max_allowed_skew_seconds.has_value() && *max_allowed_skew_seconds < 0) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_value",
                        DiagnosticSeverity::error,
                        "/max_allowed_skew_seconds",
                        "max_allowed_skew_seconds must be non-negative");
    }
    if (freshness_window_seconds.has_value() && *freshness_window_seconds < 0) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_value",
                        DiagnosticSeverity::error,
                        "/freshness_window_seconds",
                        "freshness_window_seconds must be non-negative");
    }
    if (allowed_time_sources.has_value() && has_duplicates(*allowed_time_sources)) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_value",
                        DiagnosticSeverity::error,
                        "/allowed_time_sources",
                        "allowed_time_sources must not contain duplicates");
    }

    if (validation_status_from_diagnostics(diagnostics) == ValidationStatus::error || !clock_id.has_value() ||
        !max_allowed_skew_seconds.has_value() || !freshness_window_seconds.has_value() || !monotonic_required.has_value() ||
        !allowed_time_sources.has_value() || !fail_closed_on_untrusted_time.has_value()) {
        return std::nullopt;
    }

    std::sort(allowed_time_sources->begin(), allowed_time_sources->end(), [](TimeSource left, TimeSource right) {
        return static_cast<int>(left) < static_cast<int>(right);
    });

    return SecureClockPolicy{
        .clock_id = std::move(*clock_id),
        .max_allowed_skew_seconds = static_cast<std::uint64_t>(*max_allowed_skew_seconds),
        .freshness_window_seconds = static_cast<std::uint64_t>(*freshness_window_seconds),
        .monotonic_required = *monotonic_required,
        .allowed_time_sources = std::move(*allowed_time_sources),
        .fail_closed_on_untrusted_time = *fail_closed_on_untrusted_time,
    };
}

[[nodiscard]] std::optional<SecureTimeStatus> parse_secure_time_status_object(const JsonValue& root,
                                                                              std::vector<Diagnostic>& diagnostics) {
    if (root.type != JsonValue::Type::object) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_type",
                        DiagnosticSeverity::error,
                        "/",
                        "secure time status must be a JSON object");
        return std::nullopt;
    }

    const auto secure_time_status = parse_required_enum<SecureTimeStatusCode>(root,
                                                                              "secure_time_status",
                                                                              "/secure_time_status",
                                                                              diagnostics,
                                                                              "secure_time.missing_required_time_field",
                                                                              "secure_time.invalid_value",
                                                                              parse_secure_time_status_code_string);
    const auto current_time_utc = parse_optional_string(root, "current_time_utc", "/current_time_utc", diagnostics);
    const auto time_source = parse_required_enum<TimeSource>(root,
                                                             "time_source",
                                                             "/time_source",
                                                             diagnostics,
                                                             "secure_time.missing_required_time_field",
                                                             "secure_time.invalid_time_source",
                                                             parse_time_source_string);
    const auto source_role = parse_required_enum<SourceRole>(root,
                                                             "source_role",
                                                             "/source_role",
                                                             diagnostics,
                                                             "secure_time.missing_required_time_field",
                                                             "secure_time.invalid_value",
                                                             parse_source_role_string);
    const auto source_trust_state = parse_required_enum<SourceTrustState>(root,
                                                                          "source_trust_state",
                                                                          "/source_trust_state",
                                                                          diagnostics,
                                                                          "secure_time.missing_required_time_field",
                                                                          "secure_time.invalid_value",
                                                                          parse_source_trust_state_string);
    const auto skew_seconds = parse_optional_integer(root, "skew_seconds", "/skew_seconds", diagnostics);
    const auto drift_state = parse_required_enum<DriftState>(root,
                                                             "drift_state",
                                                             "/drift_state",
                                                             diagnostics,
                                                             "secure_time.missing_required_time_field",
                                                             "secure_time.invalid_value",
                                                             parse_drift_state_string);
    const auto freshness = parse_required_enum<Freshness>(root,
                                                          "freshness",
                                                          "/freshness",
                                                          diagnostics,
                                                          "secure_time.missing_required_time_field",
                                                          "secure_time.invalid_value",
                                                          parse_freshness_string);
    const auto last_update_utc = parse_optional_string(root, "last_update_utc", "/last_update_utc", diagnostics);
    const auto monotonicity_status = parse_required_enum<MonotonicityStatus>(root,
                                                                             "monotonicity_status",
                                                                             "/monotonicity_status",
                                                                             diagnostics,
                                                                             "secure_time.missing_required_time_field",
                                                                             "secure_time.invalid_value",
                                                                             parse_monotonicity_status_string);
    const auto tamper_time_state = parse_required_enum<TamperTimeState>(root,
                                                                        "tamper_time_state",
                                                                        "/tamper_time_state",
                                                                        diagnostics,
                                                                        "secure_time.missing_required_time_field",
                                                                        "secure_time.invalid_value",
                                                                        parse_tamper_time_state_string);

    if (skew_seconds.has_value() && *skew_seconds < 0) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_value",
                        DiagnosticSeverity::error,
                        "/skew_seconds",
                        "skew_seconds must be non-negative");
    }

    if (validation_status_from_diagnostics(diagnostics) == ValidationStatus::error || !secure_time_status.has_value() ||
        !time_source.has_value() || !source_role.has_value() || !source_trust_state.has_value() || !drift_state.has_value() ||
        !freshness.has_value() || !monotonicity_status.has_value() || !tamper_time_state.has_value()) {
        return std::nullopt;
    }

    SecureTimeStatus status{
        .secure_time_status = *secure_time_status,
        .current_time_utc = current_time_utc,
        .time_source = *time_source,
        .source_role = *source_role,
        .source_trust_state = *source_trust_state,
        .drift_state = *drift_state,
        .freshness = *freshness,
        .last_update_utc = last_update_utc,
        .monotonicity_status = *monotonicity_status,
        .tamper_time_state = *tamper_time_state,
    };
    if (skew_seconds.has_value()) {
        status.skew_seconds = static_cast<std::uint64_t>(*skew_seconds);
    }
    return status;
}

void validate_secure_clock_policy_object(const SecureClockPolicy& policy,
                                         std::string_view path,
                                         std::vector<Diagnostic>& diagnostics) {
    if (policy.clock_id.empty()) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_value",
                        DiagnosticSeverity::error,
                        child_path(path, "clock_id"),
                        "clock_id must not be empty");
    }
    if (policy.allowed_time_sources.empty()) {
        push_diagnostic(diagnostics,
                        "secure_time.missing_required_time_field",
                        DiagnosticSeverity::error,
                        child_path(path, "allowed_time_sources"),
                        "allowed_time_sources must not be empty");
    } else if (has_duplicates(policy.allowed_time_sources)) {
        push_diagnostic(diagnostics,
                        "secure_time.invalid_value",
                        DiagnosticSeverity::error,
                        child_path(path, "allowed_time_sources"),
                        "allowed_time_sources must not contain duplicates");
    }
}

void validate_secure_time_status_object(const SecureTimeStatus& status,
                                        std::string_view path,
                                        std::vector<Diagnostic>& diagnostics) {
    if (is_time_data_required(status.secure_time_status)) {
        if (!status.current_time_utc.has_value()) {
            push_diagnostic(diagnostics,
                            "secure_time.missing_required_time_field",
                            DiagnosticSeverity::error,
                            child_path(path, "current_time_utc"),
                            "current_time_utc is required when secure_time_status is not unavailable");
        }
        if (!status.last_update_utc.has_value()) {
            push_diagnostic(diagnostics,
                            "secure_time.missing_required_time_field",
                            DiagnosticSeverity::error,
                            child_path(path, "last_update_utc"),
                            "last_update_utc is required when secure_time_status is not unavailable");
        }
        if (!status.skew_seconds.has_value()) {
            push_diagnostic(diagnostics,
                            "secure_time.missing_required_time_field",
                            DiagnosticSeverity::error,
                            child_path(path, "skew_seconds"),
                            "skew_seconds is required when secure_time_status is not unavailable");
        }
    }

    std::optional<std::int64_t> current_epoch;
    if (status.current_time_utc.has_value()) {
        current_epoch = parse_utc_timestamp(*status.current_time_utc);
        if (!current_epoch.has_value()) {
            push_diagnostic(diagnostics,
                            "secure_time.invalid_time_format",
                            DiagnosticSeverity::error,
                            child_path(path, "current_time_utc"),
                            "current_time_utc must be canonical UTC ISO-8601");
        }
    }

    std::optional<std::int64_t> last_update_epoch;
    if (status.last_update_utc.has_value()) {
        last_update_epoch = parse_utc_timestamp(*status.last_update_utc);
        if (!last_update_epoch.has_value()) {
            push_diagnostic(diagnostics,
                            "secure_time.invalid_time_format",
                            DiagnosticSeverity::error,
                            child_path(path, "last_update_utc"),
                            "last_update_utc must be canonical UTC ISO-8601");
        }
    }

    if (current_epoch.has_value() && last_update_epoch.has_value() && *current_epoch < *last_update_epoch) {
        push_diagnostic(diagnostics,
                        "secure_time.non_monotonic_time",
                        DiagnosticSeverity::error,
                        child_path(path, "last_update_utc"),
                        "last_update_utc must not be later than current_time_utc");
    }
}

[[nodiscard]] bool has_diagnostic_code(const std::vector<Diagnostic>& diagnostics, std::string_view code) {
    return std::any_of(diagnostics.begin(), diagnostics.end(), [&](const Diagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

void push_gate_diagnostic(std::vector<Diagnostic>& diagnostics,
                          std::string_view code,
                          DiagnosticSeverity severity,
                          std::string path,
                          std::string message) {
    if (has_diagnostic_code(diagnostics, code)) {
        return;
    }

    push_diagnostic(diagnostics, code, severity, std::move(path), std::move(message));
}

[[nodiscard]] std::string hex_escape(unsigned char value) {
    constexpr char digits[] = "0123456789abcdef";
    std::string escaped = "\\u00";
    escaped.push_back(digits[(value >> 4U) & 0x0FU]);
    escaped.push_back(digits[value & 0x0FU]);
    return escaped;
}

[[nodiscard]] std::string json_escape(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (const char raw_ch : value) {
        const auto ch = static_cast<unsigned char>(raw_ch);
        switch (ch) {
            case '\"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\b':
                escaped += "\\b";
                break;
            case '\f':
                escaped += "\\f";
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
                if (ch < 0x20U) {
                    escaped += hex_escape(ch);
                } else {
                    escaped.push_back(static_cast<char>(ch));
                }
                break;
        }
    }

    return escaped;
}

}  // namespace

ParseResult<SecureClockPolicy> parse_secure_clock_policy(std::string_view json) {
    JsonParseError error;
    JsonParser parser(json);
    const auto root = parser.parse(error);
    if (!root.has_value()) {
        return make_parse_error<SecureClockPolicy>(std::move(error.path), std::move(error.message));
    }

    std::vector<Diagnostic> diagnostics;
    auto policy = parse_secure_clock_policy_object(*root, diagnostics);
    if (policy.has_value()) {
        auto validation = validate_secure_clock_policy(*policy);
        diagnostics.insert(diagnostics.end(), validation.diagnostics.begin(), validation.diagnostics.end());
    }

    return make_parse_result(std::move(policy), std::move(diagnostics));
}

ParseResult<SecureTimeStatus> parse_secure_time_status(std::string_view json) {
    JsonParseError error;
    JsonParser parser(json);
    const auto root = parser.parse(error);
    if (!root.has_value()) {
        return make_parse_error<SecureTimeStatus>(std::move(error.path), std::move(error.message));
    }

    std::vector<Diagnostic> diagnostics;
    auto status = parse_secure_time_status_object(*root, diagnostics);
    if (status.has_value()) {
        auto validation = validate_secure_time_status(*status);
        diagnostics.insert(diagnostics.end(), validation.diagnostics.begin(), validation.diagnostics.end());
    }

    return make_parse_result(std::move(status), std::move(diagnostics));
}

CheckResult validate_secure_clock_policy(const SecureClockPolicy& policy) {
    std::vector<Diagnostic> diagnostics;
    validate_secure_clock_policy_object(policy, "/", diagnostics);
    const auto status = validation_status_from_diagnostics(diagnostics);
    return CheckResult{
        .diagnostics = std::move(diagnostics),
        .validation_status = status,
    };
}

CheckResult validate_secure_time_status(const SecureTimeStatus& status) {
    std::vector<Diagnostic> diagnostics;
    validate_secure_time_status_object(status, "/", diagnostics);
    const auto validation_status = validation_status_from_diagnostics(diagnostics);
    return CheckResult{
        .diagnostics = std::move(diagnostics),
        .validation_status = validation_status,
    };
}

SecureTimeGateResult evaluate_secure_time(const SecureClockPolicy& policy, const SecureTimeStatus& status) {
    auto policy_validation = validate_secure_clock_policy(policy);
    auto status_validation = validate_secure_time_status(status);

    std::vector<Diagnostic> diagnostics = std::move(policy_validation.diagnostics);
    diagnostics.insert(diagnostics.end(), status_validation.diagnostics.begin(), status_validation.diagnostics.end());

    const auto direct_status = validation_status_from_diagnostics(diagnostics);
    if (direct_status == ValidationStatus::error) {
        return SecureTimeGateResult{
            .trusted = false,
            .usable = false,
            .timestamp_dependent_operations_allowed = false,
            .diagnostics = std::move(diagnostics),
            .validation_status = ValidationStatus::error,
        };
    }

    if (status.tamper_time_state != TamperTimeState::clear) {
        push_gate_diagnostic(diagnostics,
                             "secure_time.secure_time_unavailable",
                             DiagnosticSeverity::error,
                             "/tamper_time_state",
                             "tamper state makes secure time unavailable");
        return SecureTimeGateResult{
            .trusted = false,
            .usable = false,
            .timestamp_dependent_operations_allowed = false,
            .diagnostics = std::move(diagnostics),
            .validation_status = ValidationStatus::error,
        };
    }

    if (status.secure_time_status == SecureTimeStatusCode::unavailable) {
        push_gate_diagnostic(diagnostics,
                             "secure_time.secure_time_unavailable",
                             DiagnosticSeverity::error,
                             "/secure_time_status",
                             "secure time is unavailable");
        return SecureTimeGateResult{
            .trusted = false,
            .usable = false,
            .timestamp_dependent_operations_allowed = false,
            .diagnostics = std::move(diagnostics),
            .validation_status = ValidationStatus::error,
        };
    }

    bool hard_block = false;
    bool trust_issue = false;
    const auto trust_issue_severity =
        policy.fail_closed_on_untrusted_time ? DiagnosticSeverity::error : DiagnosticSeverity::warning;

    if (!contains_enum(policy.allowed_time_sources, status.time_source)) {
        hard_block = true;
        push_gate_diagnostic(diagnostics,
                             "secure_time.policy_source_not_allowed",
                             DiagnosticSeverity::error,
                             "/allowed_time_sources",
                             "time_source is not allowed by policy");
    }

    if (status.secure_time_status != SecureTimeStatusCode::trusted) {
        hard_block = true;
        push_gate_diagnostic(diagnostics,
                             "secure_time.untrusted_time_source",
                             DiagnosticSeverity::error,
                             "/secure_time_status",
                             "secure_time_status must be trusted for usable time");
    } else if (status.source_trust_state != SourceTrustState::trusted) {
        trust_issue = true;
        push_gate_diagnostic(diagnostics,
                             "secure_time.untrusted_time_source",
                             trust_issue_severity,
                             "/source_trust_state",
                             "time source is not trusted");
    } else if (is_intrinsically_untrusted_source(status.time_source)) {
        trust_issue = true;
        push_gate_diagnostic(diagnostics,
                             "secure_time.untrusted_time_source",
                             trust_issue_severity,
                             "/time_source",
                             "time source is not trusted");
    }

    if (status.current_time_utc.has_value() && status.last_update_utc.has_value()) {
        const auto current_epoch = parse_utc_timestamp(*status.current_time_utc);
        const auto last_update_epoch = parse_utc_timestamp(*status.last_update_utc);
        if (current_epoch.has_value() && last_update_epoch.has_value()) {
            if (*current_epoch < *last_update_epoch) {
                hard_block = true;
                push_gate_diagnostic(diagnostics,
                                     "secure_time.non_monotonic_time",
                                     DiagnosticSeverity::error,
                                     "/last_update_utc",
                                     "last_update_utc must not be later than current_time_utc");
            } else {
                const auto age_seconds = static_cast<std::uint64_t>(*current_epoch - *last_update_epoch);
                if (status.freshness != Freshness::fresh || age_seconds > policy.freshness_window_seconds) {
                    hard_block = true;
                    push_gate_diagnostic(diagnostics,
                                         "secure_time.stale_time",
                                         DiagnosticSeverity::error,
                                         "/last_update_utc",
                                         "time snapshot is stale");
                }
            }
        }
    }

    if (status.skew_seconds.has_value() &&
        (status.drift_state == DriftState::skew_exceeded || *status.skew_seconds > policy.max_allowed_skew_seconds)) {
        hard_block = true;
        push_gate_diagnostic(diagnostics,
                             "secure_time.skew_exceeded",
                             DiagnosticSeverity::error,
                             "/skew_seconds",
                             "skew exceeds policy limit");
    }

    if (policy.monotonic_required && status.monotonicity_status != MonotonicityStatus::monotonic) {
        hard_block = true;
        push_gate_diagnostic(diagnostics,
                             "secure_time.non_monotonic_time",
                             DiagnosticSeverity::error,
                             "/monotonicity_status",
                             "monotonicity is required by policy");
    }

    const bool trusted = !hard_block && !trust_issue;
    const bool usable = !hard_block && status.secure_time_status == SecureTimeStatusCode::trusted &&
                        (!trust_issue || !policy.fail_closed_on_untrusted_time);
    const auto status_code = validation_status_from_diagnostics(diagnostics);

    return SecureTimeGateResult{
        .trusted = trusted,
        .usable = usable,
        .timestamp_dependent_operations_allowed = usable,
        .diagnostics = std::move(diagnostics),
        .validation_status = status_code,
    };
}

const char* to_string(SecureTimeStatusCode status) noexcept {
    switch (status) {
        case SecureTimeStatusCode::trusted:
            return "trusted";
        case SecureTimeStatusCode::degraded:
            return "degraded";
        case SecureTimeStatusCode::untrusted:
            return "untrusted";
        case SecureTimeStatusCode::unavailable:
            return "unavailable";
    }

    return "unavailable";
}

const char* to_string(TimeSource source) noexcept {
    switch (source) {
        case TimeSource::rtc_secure:
            return "rtc_secure";
        case TimeSource::rtc_untrusted:
            return "rtc_untrusted";
        case TimeSource::imported_secure_time:
            return "imported_secure_time";
        case TimeSource::operator_set_time_placeholder:
            return "operator_set_time_placeholder";
        case TimeSource::unknown:
            return "unknown";
    }

    return "unknown";
}

const char* to_string(SourceRole role) noexcept {
    switch (role) {
        case SourceRole::primary:
            return "primary";
        case SourceRole::fallback:
            return "fallback";
        case SourceRole::placeholder:
            return "placeholder";
        case SourceRole::unknown:
            return "unknown";
    }

    return "unknown";
}

const char* to_string(SourceTrustState state) noexcept {
    switch (state) {
        case SourceTrustState::trusted:
            return "trusted";
        case SourceTrustState::untrusted:
            return "untrusted";
        case SourceTrustState::unknown:
            return "unknown";
    }

    return "unknown";
}

const char* to_string(DriftState state) noexcept {
    switch (state) {
        case DriftState::within_policy:
            return "within_policy";
        case DriftState::skew_exceeded:
            return "skew_exceeded";
        case DriftState::unknown:
            return "unknown";
    }

    return "unknown";
}

const char* to_string(Freshness freshness) noexcept {
    switch (freshness) {
        case Freshness::fresh:
            return "fresh";
        case Freshness::stale:
            return "stale";
        case Freshness::unknown:
            return "unknown";
    }

    return "unknown";
}

const char* to_string(MonotonicityStatus status) noexcept {
    switch (status) {
        case MonotonicityStatus::monotonic:
            return "monotonic";
        case MonotonicityStatus::non_monotonic:
            return "non_monotonic";
        case MonotonicityStatus::unknown:
            return "unknown";
    }

    return "unknown";
}

const char* to_string(TamperTimeState state) noexcept {
    switch (state) {
        case TamperTimeState::clear:
            return "clear";
        case TamperTimeState::tamper_detected:
            return "tamper_detected";
        case TamperTimeState::tamper_lockout:
            return "tamper_lockout";
        case TamperTimeState::recovery_in_progress:
            return "recovery_in_progress";
    }

    return "clear";
}

std::string to_json(const SecureClockPolicy& policy) {
    std::ostringstream output;
    output << "{\n";
    output << "  \"clock_id\": \"" << json_escape(policy.clock_id) << "\",\n";
    output << "  \"max_allowed_skew_seconds\": " << policy.max_allowed_skew_seconds << ",\n";
    output << "  \"freshness_window_seconds\": " << policy.freshness_window_seconds << ",\n";
    output << "  \"monotonic_required\": " << (policy.monotonic_required ? "true" : "false") << ",\n";
    output << "  \"allowed_time_sources\": [\n";
    for (std::size_t index = 0; index < policy.allowed_time_sources.size(); ++index) {
        output << "    \"" << to_string(policy.allowed_time_sources[index]) << "\"";
        if (index + 1U != policy.allowed_time_sources.size()) {
            output << ",";
        }
        output << "\n";
    }
    output << "  ],\n";
    output << "  \"fail_closed_on_untrusted_time\": " << (policy.fail_closed_on_untrusted_time ? "true" : "false")
           << "\n";
    output << "}\n";
    return output.str();
}

std::string to_json(const SecureTimeStatus& status) {
    std::ostringstream output;
    output << "{\n";
    output << "  \"secure_time_status\": \"" << to_string(status.secure_time_status) << "\"";
    if (status.current_time_utc.has_value()) {
        output << ",\n  \"current_time_utc\": \"" << json_escape(*status.current_time_utc) << "\"";
    }
    output << ",\n  \"time_source\": \"" << to_string(status.time_source) << "\"";
    output << ",\n  \"source_role\": \"" << to_string(status.source_role) << "\"";
    output << ",\n  \"source_trust_state\": \"" << to_string(status.source_trust_state) << "\"";
    if (status.skew_seconds.has_value()) {
        output << ",\n  \"skew_seconds\": " << *status.skew_seconds;
    }
    output << ",\n  \"drift_state\": \"" << to_string(status.drift_state) << "\"";
    output << ",\n  \"freshness\": \"" << to_string(status.freshness) << "\"";
    if (status.last_update_utc.has_value()) {
        output << ",\n  \"last_update_utc\": \"" << json_escape(*status.last_update_utc) << "\"";
    }
    output << ",\n  \"monotonicity_status\": \"" << to_string(status.monotonicity_status) << "\"";
    output << ",\n  \"tamper_time_state\": \"" << to_string(status.tamper_time_state) << "\"\n";
    output << "}\n";
    return output.str();
}

std::string diagnostics_to_json(const std::vector<Diagnostic>& diagnostics) {
    return secure_channel::diagnostics_to_json(diagnostics);
}

}  // namespace dcplayer::security_api::secure_time
