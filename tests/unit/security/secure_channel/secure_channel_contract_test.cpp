#include "host_spb1_contract.hpp"
#include "secure_channel.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

using dcplayer::security_api::secure_channel::SecureChannelContract;
using dcplayer::security_api::secure_channel::SecurityModuleContract;

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

    const std::filesystem::path fixture_root{SECURE_CHANNEL_FIXTURE_DIR};

    {
        const auto json = read_file(fixture_root / "valid" / "baseline_channel.contract.json");
        const auto parsed = secure_channel::parse_secure_channel_contract(json);

        expect(parsed.ok(), "Baseline secure channel contract must parse");
        expect(parsed.value.has_value(), "Expected parsed secure channel contract");
        expect(parsed.diagnostics.empty(), "Baseline secure channel contract must not emit diagnostics");
        expect(secure_channel::to_json(*parsed.value) == json, "Unexpected canonical secure channel JSON");

        const SecureChannelContract expected = host_contract::make_baseline_secure_channel_contract();
        expect(*parsed.value == expected, "Parsed secure channel contract must match host baseline");
    }

    {
        const auto json = read_file(fixture_root / "valid" / "baseline_module.contract.json");
        const auto parsed = secure_channel::parse_security_module_contract(json);

        expect(parsed.ok(), "Baseline security module contract must parse");
        expect(parsed.value.has_value(), "Expected parsed security module contract");
        expect(parsed.diagnostics.empty(), "Baseline security module contract must not emit diagnostics");
        expect(secure_channel::to_json(*parsed.value) == json, "Unexpected canonical security module JSON");

        const SecurityModuleContract expected = host_contract::make_baseline_security_module_contract();
        expect(*parsed.value == expected, "Parsed security module contract must match host baseline");
    }

    {
        const auto json = read_file(fixture_root / "invalid" / "contract_role_mismatch.contract.json");
        const auto parsed = secure_channel::parse_secure_channel_contract(json);

        expect(!parsed.ok(), "Contract role mismatch fixture must fail");
        expect(!parsed.value.has_value(), "Contract role mismatch fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "contract_role_mismatch.diagnostics.json"),
               "Unexpected contract role mismatch diagnostics");
    }

    {
        const auto json = read_file(fixture_root / "invalid" / "duplicate_allowed_api.contract.json");
        const auto parsed = secure_channel::parse_secure_channel_contract(json);

        expect(!parsed.ok(), "Duplicate allowed_api_names fixture must fail");
        expect(!parsed.value.has_value(), "Duplicate allowed_api_names fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "duplicate_allowed_api.diagnostics.json"),
               "Unexpected duplicate allowed_api_names diagnostics");
    }

    {
        const auto json = read_file(fixture_root / "invalid" / "duplicate_revocation_status.contract.json");
        const auto parsed = secure_channel::parse_secure_channel_contract(json);

        expect(!parsed.ok(), "Duplicate accepted_revocation_statuses fixture must fail");
        expect(!parsed.value.has_value(), "Duplicate accepted_revocation_statuses fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "duplicate_revocation_status.diagnostics.json"),
               "Unexpected duplicate accepted_revocation_statuses diagnostics");
    }

    {
        const auto json = read_file(fixture_root / "invalid" / "duplicate_checked_source.contract.json");
        const auto parsed = secure_channel::parse_secure_channel_contract(json);

        expect(!parsed.ok(), "Duplicate checked_sources fixture must fail");
        expect(!parsed.value.has_value(), "Duplicate checked_sources fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "duplicate_checked_source.diagnostics.json"),
               "Unexpected duplicate checked_sources diagnostics");
    }

    {
        const auto json = read_file(fixture_root / "invalid" / "duplicate_supported_api_name.contract.json");
        const auto parsed = secure_channel::parse_security_module_contract(json);

        expect(!parsed.ok(), "Duplicate supported_api_names fixture must fail");
        expect(!parsed.value.has_value(), "Duplicate supported_api_names fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "duplicate_supported_api_name.diagnostics.json"),
               "Unexpected duplicate supported_api_names diagnostics");
    }

    {
        const auto json = read_file(fixture_root / "invalid" / "invalid_module_refs.contract.json");
        const auto parsed = secure_channel::parse_security_module_contract(json);

        expect(!parsed.ok(), "Invalid SecurityModuleContract refs fixture must fail");
        expect(!parsed.value.has_value(), "Invalid SecurityModuleContract refs fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "invalid_module_refs.diagnostics.json"),
               "Unexpected SecurityModuleContract ref diagnostics");
    }

    return 0;
}
