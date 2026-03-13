#include "host_spb1_contract.hpp"
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
    namespace secure_channel = dcplayer::security_api::secure_channel;

    const std::filesystem::path fixture_root{SECURE_CHANNEL_FIXTURE_DIR};

    const auto baseline_contract =
        secure_channel::parse_secure_channel_contract(read_file(fixture_root / "valid" / "baseline_channel.contract.json"));
    expect(baseline_contract.ok(), "Baseline secure channel contract must parse for API tests");
    expect(baseline_contract.value.has_value(), "Expected baseline contract value");

    struct RoundTripCase {
        const char* request_file;
        const char* response_file;
        const char* label;
    };

    {
        const std::array<RoundTripCase, 5> round_trip_cases{{
            {"sign.request.json", "sign.response.json", "sign"},
            {"unwrap.request.json", "unwrap.response.json", "unwrap"},
            {"decrypt.request.json", "decrypt.response.json", "decrypt"},
            {"health.request.json", "health.response.json", "health"},
            {"identity.request.json", "identity.response.json", "identity"},
        }};

        for (const auto& round_trip : round_trip_cases) {
            const auto request_json = read_file(fixture_root / "valid" / round_trip.request_file);
            const auto response_json = read_file(fixture_root / "valid" / round_trip.response_file);

            const auto request = secure_channel::parse_protected_request_envelope(request_json);
            const auto response = secure_channel::parse_protected_response_envelope(response_json);

            expect(request.ok(), std::string{"Valid "} + round_trip.label + " request must parse");
            expect(request.value.has_value(), std::string{"Expected parsed "} + round_trip.label + " request");
            expect(response.ok(), std::string{"Valid "} + round_trip.label + " response must parse");
            expect(response.value.has_value(), std::string{"Expected parsed "} + round_trip.label + " response");
            expect(secure_channel::to_json(*request.value) == request_json,
                   std::string{"Unexpected canonical "} + round_trip.label + " request JSON");
            expect(secure_channel::to_json(*response.value) == response_json,
                   std::string{"Unexpected canonical "} + round_trip.label + " response JSON");

            const auto authorization = secure_channel::authorize_request(*baseline_contract.value, *request.value);
            expect(authorization.ok(), std::string{"Valid "} + round_trip.label + " request must pass authorization");
            expect(authorization.diagnostics.empty(),
                   std::string{"Valid "} + round_trip.label + " request must not emit authorization diagnostics");

            const auto response_check =
                secure_channel::validate_response(*baseline_contract.value, *request.value, *response.value);
            expect(response_check.ok(), std::string{"Valid "} + round_trip.label + " response must pass validation");
            expect(response_check.diagnostics.empty(),
                   std::string{"Valid "} + round_trip.label + " response must not emit validation diagnostics");
        }
    }

    {
        const auto request = secure_channel::parse_protected_request_envelope(read_file(fixture_root / "valid" / "sign.request.json"));
        const auto response =
            secure_channel::parse_protected_response_envelope(read_file(fixture_root / "valid" / "sign.denied.response.json"));

        expect(request.ok(), "Baseline sign request must parse for denied response test");
        expect(request.value.has_value(), "Expected baseline sign request value");
        expect(response.ok(), "Valid denied response must parse");
        expect(response.value.has_value(), "Expected denied response value");
        expect(secure_channel::to_json(*response.value) ==
                   read_file(fixture_root / "valid" / "sign.denied.response.json"),
               "Unexpected canonical denied response JSON");

        const auto result = secure_channel::validate_response(*baseline_contract.value, *request.value, *response.value);
        expect(result.ok(), "Denied error_response must pass validation");
        expect(result.diagnostics.empty(), "Denied error_response must not emit validation diagnostics");
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
        const auto request = secure_channel::parse_protected_request_envelope(
            read_file(fixture_root / "invalid" / "sign_missing_manifest.request.json"));

        expect(!request.ok(), "Sign request missing manifest_hash must fail");
        expect(!request.value.has_value(), "Sign request missing manifest_hash must not yield a value");
        expect(secure_channel::diagnostics_to_json(request.diagnostics) ==
                   read_file(fixture_root / "invalid" / "sign_missing_manifest.diagnostics.json"),
               "Unexpected missing_required_field diagnostics");
    }

    {
        const auto request = secure_channel::parse_protected_request_envelope(
            read_file(fixture_root / "invalid" / "sign_empty_metadata.request.json"));

        expect(!request.ok(), "Sign request with empty metadata_summary must fail");
        expect(!request.value.has_value(), "Sign request with empty metadata_summary must not yield a value");
        expect(secure_channel::diagnostics_to_json(request.diagnostics) ==
                   read_file(fixture_root / "invalid" / "sign_empty_metadata.diagnostics.json"),
               "Unexpected invalid_request_body diagnostics");
    }

    {
        const auto request = secure_channel::parse_protected_request_envelope(
            read_file(fixture_root / "invalid" / "health_unexpected_field.request.json"));

        expect(!request.ok(), "Health request with unexpected body field must fail");
        expect(!request.value.has_value(), "Health request with unexpected body field must not yield a value");
        expect(secure_channel::diagnostics_to_json(request.diagnostics) ==
                   read_file(fixture_root / "invalid" / "health_unexpected_field.diagnostics.json"),
               "Unexpected unexpected_field diagnostics");
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
        const auto request = secure_channel::parse_protected_request_envelope(
            read_file(fixture_root / "invalid" / "server_caller.request.json"));

        expect(request.ok(), "Server caller request must still parse");
        expect(request.value.has_value(), "Expected parsed server caller request");

        const auto authorization = secure_channel::authorize_request(*baseline_contract.value, *request.value);
        expect(!authorization.ok(), "Server caller request must fail authorization");
        expect(secure_channel::diagnostics_to_json(authorization.diagnostics) ==
                   read_file(fixture_root / "invalid" / "server_caller.diagnostics.json"),
               "Unexpected server caller diagnostics");
    }

    {
        const auto request = secure_channel::parse_protected_request_envelope(
            read_file(fixture_root / "invalid" / "peer_not_trusted.request.json"));

        expect(request.ok(), "Untrusted peer request must still parse");
        expect(request.value.has_value(), "Expected parsed untrusted peer request");

        const auto authorization = secure_channel::authorize_request(*baseline_contract.value, *request.value);
        expect(!authorization.ok(), "Untrusted peer request must fail authorization");
        expect(secure_channel::diagnostics_to_json(authorization.diagnostics) ==
                   read_file(fixture_root / "invalid" / "peer_not_trusted.diagnostics.json"),
               "Unexpected peer_not_trusted diagnostics");
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

    {
        const auto response = secure_channel::parse_protected_response_envelope(
            read_file(fixture_root / "invalid" / "identity_wrong_role.response.json"));

        expect(!response.ok(), "Identity response with wrong module_role must fail");
        expect(!response.value.has_value(), "Identity response with wrong module_role must not yield a value");
        expect(secure_channel::diagnostics_to_json(response.diagnostics) ==
                   read_file(fixture_root / "invalid" / "identity_wrong_role.diagnostics.json"),
               "Unexpected invalid_response_body diagnostics");
    }

    {
        const auto request = secure_channel::parse_protected_request_envelope(read_file(fixture_root / "valid" / "sign.request.json"));
        const auto response =
            secure_channel::parse_protected_response_envelope(read_file(fixture_root / "invalid" / "invalid_status_body.response.json"));

        expect(request.ok(), "Baseline sign request must parse for denied+sign_response precedence test");
        expect(request.value.has_value(), "Expected baseline sign request value");
        expect(response.ok(), "Denied response with sign_response payload must parse");
        expect(response.value.has_value(), "Expected parsed denied+sign_response fixture");

        const auto result = secure_channel::validate_response(*baseline_contract.value, *request.value, *response.value);
        expect(!result.ok(), "Denied response with sign_response payload must fail validation");
        expect(secure_channel::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "invalid_status_body.diagnostics.json"),
               "Unexpected invalid_status_body_combination diagnostics");
    }

    {
        const auto request = secure_channel::parse_protected_request_envelope(read_file(fixture_root / "valid" / "sign.request.json"));
        const auto response =
            secure_channel::parse_protected_response_envelope(read_file(fixture_root / "invalid" / "ok_error_response.response.json"));

        expect(request.ok(), "Baseline sign request must parse for ok+error_response precedence test");
        expect(request.value.has_value(), "Expected baseline sign request value");
        expect(response.ok(), "ok+error_response fixture must parse");
        expect(response.value.has_value(), "Expected parsed ok+error_response fixture");

        const auto result = secure_channel::validate_response(*baseline_contract.value, *request.value, *response.value);
        expect(!result.ok(), "ok+error_response must fail validation");
        expect(secure_channel::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "ok_error_response.diagnostics.json"),
               "Unexpected precedence diagnostics for ok+error_response");
    }

    {
        const auto request = secure_channel::parse_protected_request_envelope(read_file(fixture_root / "valid" / "sign.request.json"));
        const auto response =
            secure_channel::parse_protected_response_envelope(read_file(fixture_root / "invalid" / "request_response_api_mismatch.response.json"));

        expect(request.ok(), "Baseline sign request must parse for request/response family mismatch test");
        expect(request.value.has_value(), "Expected baseline sign request value");
        expect(response.ok(), "request/response family mismatch fixture must parse");
        expect(response.value.has_value(), "Expected parsed request/response family mismatch fixture");

        const auto result = secure_channel::validate_response(*baseline_contract.value, *request.value, *response.value);
        expect(!result.ok(), "Health response must not validate against sign request");
        expect(secure_channel::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "request_response_api_mismatch.diagnostics.json"),
               "Unexpected request_response_api_mismatch diagnostics");
    }

    return 0;
}
