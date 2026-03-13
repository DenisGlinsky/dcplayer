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

template <typename T>
T parse_required(const std::filesystem::path& path,
                 dcplayer::security_api::secure_time::ParseResult<T> (*parser)(std::string_view),
                 const std::string& label) {
    const auto parsed = parser(read_file(path));
    if (!parsed.ok() || !parsed.value.has_value()) {
        throw std::runtime_error("Failed to parse " + label + " fixture: " + path.string());
    }
    return *parsed.value;
}

}  // namespace

int main() {
    namespace secure_time = dcplayer::security_api::secure_time;

    const std::filesystem::path fixture_root{SECURE_TIME_FIXTURE_DIR};

    const auto baseline_policy = parse_required(
        fixture_root / "valid" / "baseline.policy.json", secure_time::parse_secure_clock_policy, "baseline policy");
    const auto fail_open_policy = parse_required(
        fixture_root / "valid" / "fail_open.policy.json", secure_time::parse_secure_clock_policy, "fail-open policy");
    const auto rtc_only_policy = parse_required(
        fixture_root / "valid" / "rtc_only.policy.json", secure_time::parse_secure_clock_policy, "rtc-only policy");

    const auto trusted_status = parse_required(
        fixture_root / "valid" / "trusted.status.json", secure_time::parse_secure_time_status, "trusted status");
    const auto degraded_status = parse_required(
        fixture_root / "valid" / "degraded.status.json", secure_time::parse_secure_time_status, "degraded status");
    const auto policy_source_status = parse_required(fixture_root / "valid" / "policy_source_not_allowed.status.json",
                                                     secure_time::parse_secure_time_status,
                                                     "policy-source status");
    const auto unavailable_status = parse_required(fixture_root / "valid" / "secure_time_unavailable.status.json",
                                                   secure_time::parse_secure_time_status,
                                                   "unavailable status");

    const auto untrusted_source_status = parse_required(fixture_root / "invalid" / "untrusted_time_source.status.json",
                                                        secure_time::parse_secure_time_status,
                                                        "untrusted source status");
    const auto untrusted_status = parse_required(
        fixture_root / "invalid" / "untrusted_status.status.json", secure_time::parse_secure_time_status, "untrusted status");
    const auto stale_status =
        parse_required(fixture_root / "invalid" / "stale_time.status.json", secure_time::parse_secure_time_status, "stale status");
    const auto skew_status = parse_required(
        fixture_root / "invalid" / "skew_exceeded.status.json", secure_time::parse_secure_time_status, "skew status");
    const auto monotonic_status = parse_required(fixture_root / "invalid" / "non_monotonic_time.status.json",
                                                 secure_time::parse_secure_time_status,
                                                 "non-monotonic status");

    {
        const auto result = secure_time::evaluate_secure_time(baseline_policy, trusted_status);

        expect(result.ok(), "Trusted secure time must not fail gating");
        expect(result.trusted, "Trusted secure time must be marked trusted");
        expect(result.usable, "Trusted secure time must be usable");
        expect(result.timestamp_dependent_operations_allowed,
               "Trusted secure time must allow timestamp-dependent operations");
        expect(result.validation_status == secure_time::ValidationStatus::ok,
               "Trusted secure time must produce ok validation status");
        expect(result.diagnostics.empty(), "Trusted secure time must not emit diagnostics");
    }

    {
        const auto result = secure_time::evaluate_secure_time(baseline_policy, untrusted_source_status);

        expect(!result.ok(), "Fail-closed untrusted source must fail secure time gating");
        expect(!result.trusted, "Fail-closed untrusted source must not be trusted");
        expect(!result.usable, "Fail-closed untrusted source must not be usable");
        expect(!result.timestamp_dependent_operations_allowed,
               "Fail-closed untrusted source must block timestamp-dependent operations");
        expect(result.validation_status == secure_time::ValidationStatus::error,
               "Fail-closed untrusted source must produce error validation status");
        expect(secure_time::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "untrusted_time_source.diagnostics.json"),
               "Unexpected fail-closed untrusted_time_source diagnostics");
    }

    {
        const auto result = secure_time::evaluate_secure_time(fail_open_policy, untrusted_source_status);

        expect(result.ok(), "Fail-open untrusted source must remain warning-only");
        expect(!result.trusted, "Fail-open untrusted source must not be trusted");
        expect(result.usable, "Fail-open untrusted source must remain usable");
        expect(result.timestamp_dependent_operations_allowed,
               "Fail-open untrusted source must keep timestamp-dependent operations available");
        expect(result.validation_status == secure_time::ValidationStatus::warning,
               "Fail-open untrusted source must produce warning validation status");
        expect(secure_time::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "untrusted_time_source.warning.diagnostics.json"),
               "Unexpected fail-open untrusted_time_source diagnostics");
    }

    {
        const auto result = secure_time::evaluate_secure_time(fail_open_policy, untrusted_status);

        expect(!result.ok(), "Untrusted secure time status must fail gating even with fail-open policy");
        expect(!result.trusted, "Untrusted secure time status must not be trusted");
        expect(!result.usable, "Untrusted secure time status must not be usable");
        expect(!result.timestamp_dependent_operations_allowed,
               "Untrusted secure time status must block timestamp-dependent operations");
        expect(result.validation_status == secure_time::ValidationStatus::error,
               "Untrusted secure time status must produce error validation status");
        expect(secure_time::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "untrusted_status_blocked.diagnostics.json"),
               "Unexpected untrusted secure time status diagnostics");
    }

    {
        const auto result = secure_time::evaluate_secure_time(fail_open_policy, degraded_status);

        expect(!result.ok(), "Degraded secure time must fail gating even with fail-open policy");
        expect(!result.trusted, "Degraded secure time must not be trusted");
        expect(!result.usable, "Degraded secure time must not be usable");
        expect(!result.timestamp_dependent_operations_allowed,
               "Degraded secure time must block timestamp-dependent operations");
        expect(result.validation_status == secure_time::ValidationStatus::error,
               "Degraded secure time must produce error validation status");
        expect(secure_time::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "degraded_status_blocked.diagnostics.json"),
               "Unexpected degraded secure time diagnostics");
    }

    {
        const auto result = secure_time::evaluate_secure_time(baseline_policy, stale_status);

        expect(!result.ok(), "Stale secure time must fail gating");
        expect(!result.trusted, "Stale secure time must not be trusted");
        expect(!result.usable, "Stale secure time must not be usable");
        expect(!result.timestamp_dependent_operations_allowed,
               "Stale secure time must block timestamp-dependent operations");
        expect(secure_time::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "stale_time.diagnostics.json"),
               "Unexpected stale_time diagnostics");
    }

    {
        const auto result = secure_time::evaluate_secure_time(baseline_policy, skew_status);

        expect(!result.ok(), "Skew-exceeded secure time must fail gating");
        expect(!result.trusted, "Skew-exceeded secure time must not be trusted");
        expect(!result.usable, "Skew-exceeded secure time must not be usable");
        expect(!result.timestamp_dependent_operations_allowed,
               "Skew-exceeded secure time must block timestamp-dependent operations");
        expect(secure_time::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "skew_exceeded.diagnostics.json"),
               "Unexpected skew_exceeded diagnostics");
    }

    {
        const auto result = secure_time::evaluate_secure_time(baseline_policy, monotonic_status);

        expect(!result.ok(), "Non-monotonic secure time must fail gating");
        expect(!result.trusted, "Non-monotonic secure time must not be trusted");
        expect(!result.usable, "Non-monotonic secure time must not be usable");
        expect(!result.timestamp_dependent_operations_allowed,
               "Non-monotonic secure time must block timestamp-dependent operations");
        expect(secure_time::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "non_monotonic_time.diagnostics.json"),
               "Unexpected non_monotonic_time diagnostics");
    }

    {
        const auto result = secure_time::evaluate_secure_time(rtc_only_policy, policy_source_status);

        expect(!result.ok(), "Policy-disallowed source must fail gating");
        expect(!result.trusted, "Policy-disallowed source must not be trusted");
        expect(!result.usable, "Policy-disallowed source must not be usable");
        expect(!result.timestamp_dependent_operations_allowed,
               "Policy-disallowed source must block timestamp-dependent operations");
        expect(secure_time::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "policy_source_not_allowed.diagnostics.json"),
               "Unexpected policy_source_not_allowed diagnostics");
    }

    {
        const auto result = secure_time::evaluate_secure_time(baseline_policy, unavailable_status);

        expect(!result.ok(), "Unavailable secure time must fail gating");
        expect(!result.trusted, "Unavailable secure time must not be trusted");
        expect(!result.usable, "Unavailable secure time must not be usable");
        expect(!result.timestamp_dependent_operations_allowed,
               "Unavailable secure time must block timestamp-dependent operations");
        expect(secure_time::diagnostics_to_json(result.diagnostics) ==
                   read_file(fixture_root / "invalid" / "secure_time_unavailable.diagnostics.json"),
               "Unexpected secure_time_unavailable diagnostics");
    }

    return 0;
}
