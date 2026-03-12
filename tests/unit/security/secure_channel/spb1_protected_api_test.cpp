#include "host_spb1_contract.hpp"
#include "secure_channel.hpp"

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
    namespace secure_channel = dcplayer::security_api::secure_channel;

    const std::filesystem::path fixture_root{SECURE_CHANNEL_FIXTURE_DIR};

    const auto baseline_contract =
        secure_channel::parse_secure_channel_contract(read_file(fixture_root / "valid" / "baseline_channel.contract.json"));
    expect(baseline_contract.ok(), "Baseline secure channel contract must parse for API tests");
    expect(baseline_contract.value.has_value(), "Expected baseline contract value");

    {
        const auto request_json = read_file(fixture_root / "valid" / "sign.request.json");
        const auto response_json = read_file(fixture_root / "valid" / "sign.response.json");

        const auto request = secure_channel::parse_protected_request_envelope(request_json);
        const auto response = secure_channel::parse_protected_response_envelope(response_json);

        expect(request.ok(), "Valid sign request must parse");
        expect(request.value.has_value(), "Expected parsed sign request");
        expect(response.ok(), "Valid sign response must parse");
        expect(response.value.has_value(), "Expected parsed sign response");
        expect(secure_channel::to_json(*request.value) == request_json, "Unexpected canonical sign request JSON");
        expect(secure_channel::to_json(*response.value) == response_json, "Unexpected canonical sign response JSON");

        const auto authorization = secure_channel::authorize_request(*baseline_contract.value, *request.value);
        expect(authorization.ok(), "Valid sign request must pass authorization");
        expect(authorization.diagnostics.empty(), "Valid sign request must not emit authorization diagnostics");

        const auto response_check = secure_channel::validate_response(*baseline_contract.value, *request.value, *response.value);
        expect(response_check.ok(), "Valid sign response must pass validation");
        expect(response_check.diagnostics.empty(), "Valid sign response must not emit validation diagnostics");
    }

    {
        const auto request =
            secure_channel::parse_protected_request_envelope(read_file(fixture_root / "invalid" / "role_mismatch.request.json"));

        expect(!request.ok(), "Role mismatch request fixture must fail");
        expect(!request.value.has_value(), "Role mismatch request must not yield a value");
        expect(secure_channel::diagnostics_to_json(request.diagnostics) ==
                   read_file(fixture_root / "invalid" / "role_mismatch.diagnostics.json"),
               "Unexpected role mismatch diagnostics");
    }

    {
        const auto request = secure_channel::parse_protected_request_envelope(
            read_file(fixture_root / "invalid" / "missing_trust_binding.request.json"));

        expect(!request.ok(), "Missing trust binding request fixture must fail");
        expect(!request.value.has_value(), "Missing trust binding request must not yield a value");
        expect(secure_channel::diagnostics_to_json(request.diagnostics) ==
                   read_file(fixture_root / "invalid" / "missing_trust_binding.diagnostics.json"),
               "Unexpected missing trust binding diagnostics");
    }

    {
        const auto request = secure_channel::parse_protected_request_envelope(
            read_file(fixture_root / "invalid" / "duplicate_san_entry.request.json"));

        expect(!request.ok(), "Duplicate SAN entry request fixture must fail");
        expect(!request.value.has_value(), "Duplicate SAN entry request must not yield a value");
        expect(secure_channel::diagnostics_to_json(request.diagnostics) ==
                   read_file(fixture_root / "invalid" / "duplicate_san_entry.diagnostics.json"),
               "Unexpected duplicate SAN entry diagnostics");
    }

    {
        const auto restricted_contract =
            secure_channel::parse_secure_channel_contract(read_file(fixture_root / "valid" / "restricted_channel.contract.json"));
        const auto request = secure_channel::parse_protected_request_envelope(
            read_file(fixture_root / "invalid" / "unauthorized_sign.request.json"));

        expect(restricted_contract.ok(), "Restricted contract fixture must parse");
        expect(restricted_contract.value.has_value(), "Expected restricted contract value");
        expect(request.ok(), "Unauthorized sign request fixture must still parse");
        expect(request.value.has_value(), "Expected unauthorized sign request value");

        const auto authorization = secure_channel::authorize_request(*restricted_contract.value, *request.value);
        expect(!authorization.ok(), "Unauthorized sign request must fail authorization");
        expect(secure_channel::diagnostics_to_json(authorization.diagnostics) ==
                   read_file(fixture_root / "invalid" / "unauthorized_sign.diagnostics.json"),
               "Unexpected unauthorized sign diagnostics");
    }

    {
        const auto request = secure_channel::parse_protected_request_envelope(read_file(fixture_root / "valid" / "sign.request.json"));
        const auto response = secure_channel::parse_protected_response_envelope(
            read_file(fixture_root / "invalid" / "request_id_mismatch.response.json"));

        expect(request.ok(), "Baseline sign request must parse for response mismatch test");
        expect(request.value.has_value(), "Expected baseline sign request value");
        expect(response.ok(), "Request-id mismatch response must still parse");
        expect(response.value.has_value(), "Expected mismatch response value");

        const auto result = secure_channel::validate_response(*baseline_contract.value, *request.value, *response.value);
        expect(!result.ok(), "Response request_id mismatch must fail validation");
        expect(secure_channel::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "request_id_mismatch.diagnostics.json"),
               "Unexpected request_id mismatch diagnostics");
    }

    return 0;
}
