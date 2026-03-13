#include "host_spb1_contract.hpp"
#include "secure_time.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open fixture: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

}  // namespace

int main() {
    namespace host_contract = dcplayer::security_api::host_spb1_contract;
    namespace secure_channel = dcplayer::security_api::secure_channel;
    namespace secure_time = dcplayer::security_api::secure_time;

    const std::filesystem::path fixture_root{SECURE_TIME_FIXTURE_DIR};

    {
        const auto json = read_file(fixture_root / "valid" / "baseline.policy.json");
        const auto parsed = secure_time::parse_secure_clock_policy(json);

        expect(parsed.ok(), "Baseline secure clock policy must parse");
        expect(parsed.value.has_value(), "Expected parsed secure clock policy");
        expect(parsed.diagnostics.empty(), "Baseline secure clock policy must not emit diagnostics");
        expect(secure_time::to_json(*parsed.value) == json, "Unexpected canonical secure clock policy JSON");
        expect(*parsed.value == host_contract::make_baseline_secure_clock_policy(),
               "Parsed secure clock policy must match host baseline");

        const auto validation = secure_time::validate_secure_clock_policy(*parsed.value);
        expect(validation.ok(), "Baseline secure clock policy must validate");
        expect(validation.diagnostics.empty(), "Baseline secure clock policy validation must stay clean");
    }

    {
        const auto json = read_file(fixture_root / "valid" / "escaped.policy.json");
        const auto parsed = secure_time::parse_secure_clock_policy(json);

        expect(parsed.ok(), "Escaped secure clock policy must parse");
        expect(parsed.value.has_value(), "Expected parsed escaped secure clock policy");
        expect(parsed.diagnostics.empty(), "Escaped secure clock policy must not emit diagnostics");
        expect(secure_time::to_json(*parsed.value) == json, "Unexpected canonical escaped secure clock policy JSON");

        const auto round_trip = secure_time::parse_secure_clock_policy(secure_time::to_json(*parsed.value));
        expect(round_trip.ok(), "Escaped secure clock policy must survive round-trip parse");
        expect(round_trip.value.has_value(), "Expected round-tripped escaped secure clock policy");
        expect(*round_trip.value == *parsed.value, "Round-tripped escaped secure clock policy must match source");
    }

    {
        secure_time::SecureClockPolicy control_char_policy{
            .clock_id = [] {
                std::string value = "spb1";
                value.push_back('\x01');
                value += "clock";
                return value;
            }(),
            .max_allowed_skew_seconds = 30U,
            .freshness_window_seconds = 300U,
            .monotonic_required = true,
            .allowed_time_sources = {secure_time::TimeSource::rtc_secure},
            .fail_closed_on_untrusted_time = true,
        };

        const auto json = secure_time::to_json(control_char_policy);
        expect(json.find("\"clock_id\": \"spb1\\u0001clock\"") != std::string::npos,
               "Control characters in clock_id must be escaped canonically");

        const auto round_trip = secure_time::parse_secure_clock_policy(json);
        expect(round_trip.ok(), "Serializer output with control chars must parse back");
        expect(round_trip.value.has_value(), "Expected parsed control-char secure clock policy");
        expect(*round_trip.value == control_char_policy,
               "Round-tripped control-char secure clock policy must match source");
        expect(secure_time::to_json(*round_trip.value) == json,
               "Control-char secure clock policy must keep canonical JSON after round-trip");
    }

    {
        const auto gated_api_names = host_contract::secure_time_gated_api_names();
        expect(gated_api_names.size() == 3U, "Expected exactly three secure-time-gated API names");
        expect(gated_api_names[0] == secure_channel::ApiName::sign, "sign must be secure-time-gated");
        expect(gated_api_names[1] == secure_channel::ApiName::unwrap, "unwrap must be secure-time-gated");
        expect(gated_api_names[2] == secure_channel::ApiName::decrypt, "decrypt must be secure-time-gated");
    }

    {
        const auto json = read_file(fixture_root / "valid" / "trusted.status.json");
        const auto parsed = secure_time::parse_secure_time_status(json);

        expect(parsed.ok(), "Trusted secure time status must parse");
        expect(parsed.value.has_value(), "Expected parsed trusted secure time status");
        expect(parsed.diagnostics.empty(), "Trusted secure time status must not emit diagnostics");
        expect(secure_time::to_json(*parsed.value) == json, "Unexpected canonical secure time status JSON");

        const auto validation = secure_time::validate_secure_time_status(*parsed.value);
        expect(validation.ok(), "Trusted secure time status must validate");
        expect(validation.diagnostics.empty(), "Trusted secure time status validation must stay clean");
    }

    {
        const auto json = read_file(fixture_root / "invalid" / "invalid_time_format.status.json");
        const auto parsed = secure_time::parse_secure_time_status(json);

        expect(!parsed.ok(), "Invalid secure time format fixture must fail");
        expect(!parsed.value.has_value(), "Invalid secure time format fixture must not yield a value");
        expect(secure_time::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "invalid_time_format.diagnostics.json"),
               "Unexpected invalid_time_format diagnostics");
    }

    {
        const auto json = read_file(fixture_root / "invalid" / "invalid_time_source.status.json");
        const auto parsed = secure_time::parse_secure_time_status(json);

        expect(!parsed.ok(), "Invalid time_source fixture must fail");
        expect(!parsed.value.has_value(), "Invalid time_source fixture must not yield a value");
        expect(secure_time::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "invalid_time_source.diagnostics.json"),
               "Unexpected invalid_time_source diagnostics");
    }

    {
        const auto json = read_file(fixture_root / "invalid" / "missing_clock_id.policy.json");
        const auto parsed = secure_time::parse_secure_clock_policy(json);

        expect(!parsed.ok(), "Missing clock_id fixture must fail");
        expect(!parsed.value.has_value(), "Missing clock_id fixture must not yield a value");
        expect(secure_time::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "missing_clock_id.diagnostics.json"),
               "Unexpected missing clock_id diagnostics");
    }

    {
        const auto json = read_file(fixture_root / "invalid" / "null_clock_id.policy.json");
        const auto parsed = secure_time::parse_secure_clock_policy(json);

        expect(!parsed.ok(), "Null clock_id fixture must fail");
        expect(!parsed.value.has_value(), "Null clock_id fixture must not yield a value");
        expect(secure_time::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "null_clock_id.diagnostics.json"),
               "Unexpected null clock_id diagnostics");
    }

    {
        const auto json = read_file(fixture_root / "invalid" / "missing_required_time_field.status.json");
        const auto parsed = secure_time::parse_secure_time_status(json);

        expect(!parsed.ok(), "Missing current_time_utc fixture must fail");
        expect(!parsed.value.has_value(), "Missing current_time_utc fixture must not yield a value");
        expect(secure_time::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "missing_required_time_field.diagnostics.json"),
               "Unexpected missing_required_time_field diagnostics");
    }

    {
        const auto expect_json_malformed = [&](const std::string& json, const std::string& label) {
            const auto parsed = secure_time::parse_secure_clock_policy(json);

            expect(!parsed.ok(), label + " must fail");
            expect(!parsed.value.has_value(), label + " must not yield a value");
            expect(parsed.diagnostics.size() == 1U, label + " must emit exactly one diagnostic");
            expect(parsed.diagnostics.front().code == "secure_time.json_malformed",
                   label + " must emit secure_time.json_malformed");
            expect(parsed.diagnostics.front().severity == secure_time::DiagnosticSeverity::error,
                   label + " must emit error severity");
            expect(parsed.diagnostics.front().path == "/clock_id", label + " must point at /clock_id");
            expect(parsed.diagnostics.front().message == "unescaped control character in string",
                   label + " must report raw control chars");
        };

        expect_json_malformed("{\n"
                              "  \"clock_id\": \"bad\nclock\",\n"
                              "  \"max_allowed_skew_seconds\": 30,\n"
                              "  \"freshness_window_seconds\": 300,\n"
                              "  \"monotonic_required\": true,\n"
                              "  \"allowed_time_sources\": [\n"
                              "    \"rtc_secure\"\n"
                              "  ],\n"
                              "  \"fail_closed_on_untrusted_time\": true\n"
                              "}\n",
                              "Raw newline inside JSON string");
        expect_json_malformed("{\n"
                              "  \"clock_id\": \"bad\tclock\",\n"
                              "  \"max_allowed_skew_seconds\": 30,\n"
                              "  \"freshness_window_seconds\": 300,\n"
                              "  \"monotonic_required\": true,\n"
                              "  \"allowed_time_sources\": [\n"
                              "    \"rtc_secure\"\n"
                              "  ],\n"
                              "  \"fail_closed_on_untrusted_time\": true\n"
                              "}\n",
                              "Raw tab inside JSON string");
        expect_json_malformed(std::string("{\n"
                                          "  \"clock_id\": \"bad") +
                                  std::string(1U, '\x01') +
                                  "clock\",\n"
                                  "  \"max_allowed_skew_seconds\": 30,\n"
                                  "  \"freshness_window_seconds\": 300,\n"
                                  "  \"monotonic_required\": true,\n"
                                  "  \"allowed_time_sources\": [\n"
                                  "    \"rtc_secure\"\n"
                                  "  ],\n"
                                  "  \"fail_closed_on_untrusted_time\": true\n"
                                  "}\n",
                              "Raw control char inside JSON string");
    }

    return 0;
}
