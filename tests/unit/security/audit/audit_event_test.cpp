#include "audit.hpp"
#include "secure_channel.hpp"

#include <array>
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
    namespace audit = dcplayer::security_api::audit;
    namespace secure_channel = dcplayer::security_api::secure_channel;

    const std::filesystem::path fixture_root{AUDIT_FIXTURE_DIR};

    {
        constexpr std::array<const char*, 3> valid_cases{
            "auth_session_authenticated.event.json",
            "tamper_detected.event.json",
            "export_audit_exported.event.json",
        };

        for (const char* fixture_name : valid_cases) {
            const auto json = read_file(fixture_root / "valid" / fixture_name);
            const auto parsed = audit::parse_security_event(json);

            expect(parsed.ok(), std::string{"Valid event fixture must parse: "} + fixture_name);
            expect(parsed.value.has_value(), std::string{"Expected parsed event value: "} + fixture_name);
            expect(parsed.diagnostics.empty(), std::string{"Valid event fixture emitted diagnostics: "} + fixture_name);
            expect(audit::to_json(*parsed.value) == json, std::string{"Unexpected canonical event JSON: "} + fixture_name);
        }
    }

    {
        const auto parsed =
            audit::parse_security_event(read_file(fixture_root / "invalid" / "invalid_event_type.event.json"));

        expect(!parsed.ok(), "Invalid event_type fixture must fail");
        expect(!parsed.value.has_value(), "Invalid event_type fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "invalid_event_type.diagnostics.json"),
               "Unexpected invalid_event_type diagnostics");
    }

    {
        const auto parsed =
            audit::parse_security_event(read_file(fixture_root / "invalid" / "invalid_severity.event.json"));

        expect(!parsed.ok(), "Invalid severity fixture must fail");
        expect(!parsed.value.has_value(), "Invalid severity fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "invalid_severity.diagnostics.json"),
               "Unexpected invalid_severity diagnostics");
    }

    {
        const auto parsed =
            audit::parse_security_event(read_file(fixture_root / "invalid" / "malformed_event_time.event.json"));

        expect(!parsed.ok(), "Malformed event_time_utc fixture must fail");
        expect(!parsed.value.has_value(), "Malformed event_time_utc fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "malformed_event_time.diagnostics.json"),
               "Unexpected malformed event_time_utc diagnostics");
    }

    {
        const auto parsed =
            audit::parse_security_event(read_file(fixture_root / "invalid" / "missing_api_request_id.event.json"));

        expect(!parsed.ok(), "Missing request_id fixture must fail");
        expect(!parsed.value.has_value(), "Missing request_id fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "missing_api_request_id.diagnostics.json"),
               "Unexpected missing_required_field diagnostics");
    }

    return 0;
}
