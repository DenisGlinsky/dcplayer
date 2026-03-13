#pragma once

#include "secure_channel.hpp"

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dcplayer::security_api::secure_time {

using Diagnostic = secure_channel::Diagnostic;
using DiagnosticSeverity = secure_channel::DiagnosticSeverity;
using ValidationStatus = secure_channel::ValidationStatus;
using CheckResult = secure_channel::CheckResult;

template <typename T>
using ParseResult = secure_channel::ParseResult<T>;

enum class SecureTimeStatusCode {
    trusted,
    degraded,
    untrusted,
    unavailable,
};

enum class TimeSource {
    rtc_secure,
    rtc_untrusted,
    imported_secure_time,
    operator_set_time_placeholder,
    unknown,
};

enum class SourceRole {
    primary,
    fallback,
    placeholder,
    unknown,
};

enum class SourceTrustState {
    trusted,
    untrusted,
    unknown,
};

enum class DriftState {
    within_policy,
    skew_exceeded,
    unknown,
};

enum class Freshness {
    fresh,
    stale,
    unknown,
};

enum class MonotonicityStatus {
    monotonic,
    non_monotonic,
    unknown,
};

enum class TamperTimeState {
    clear,
    tamper_detected,
    tamper_lockout,
    recovery_in_progress,
};

struct SecureClockPolicy {
    std::string clock_id;
    std::uint64_t max_allowed_skew_seconds{0U};
    std::uint64_t freshness_window_seconds{0U};
    bool monotonic_required{false};
    std::vector<TimeSource> allowed_time_sources;
    bool fail_closed_on_untrusted_time{true};

    [[nodiscard]] auto operator<=>(const SecureClockPolicy&) const = default;
};

struct SecureTimeStatus {
    SecureTimeStatusCode secure_time_status{SecureTimeStatusCode::unavailable};
    std::optional<std::string> current_time_utc;
    TimeSource time_source{TimeSource::unknown};
    SourceRole source_role{SourceRole::unknown};
    SourceTrustState source_trust_state{SourceTrustState::unknown};
    std::optional<std::uint64_t> skew_seconds;
    DriftState drift_state{DriftState::unknown};
    Freshness freshness{Freshness::unknown};
    std::optional<std::string> last_update_utc;
    MonotonicityStatus monotonicity_status{MonotonicityStatus::unknown};
    TamperTimeState tamper_time_state{TamperTimeState::clear};

    [[nodiscard]] auto operator<=>(const SecureTimeStatus&) const = default;
};

struct SecureTimeGateResult {
    bool trusted{false};
    bool usable{false};
    bool timestamp_dependent_operations_allowed{false};
    std::vector<Diagnostic> diagnostics;
    ValidationStatus validation_status{ValidationStatus::ok};

    [[nodiscard]] bool ok() const noexcept {
        return validation_status != ValidationStatus::error;
    }
};

[[nodiscard]] ParseResult<SecureClockPolicy> parse_secure_clock_policy(std::string_view json);
[[nodiscard]] ParseResult<SecureTimeStatus> parse_secure_time_status(std::string_view json);

[[nodiscard]] CheckResult validate_secure_clock_policy(const SecureClockPolicy& policy);
[[nodiscard]] CheckResult validate_secure_time_status(const SecureTimeStatus& status);
[[nodiscard]] SecureTimeGateResult evaluate_secure_time(const SecureClockPolicy& policy,
                                                        const SecureTimeStatus& status);

[[nodiscard]] const char* to_string(SecureTimeStatusCode status) noexcept;
[[nodiscard]] const char* to_string(TimeSource source) noexcept;
[[nodiscard]] const char* to_string(SourceRole role) noexcept;
[[nodiscard]] const char* to_string(SourceTrustState state) noexcept;
[[nodiscard]] const char* to_string(DriftState state) noexcept;
[[nodiscard]] const char* to_string(Freshness freshness) noexcept;
[[nodiscard]] const char* to_string(MonotonicityStatus status) noexcept;
[[nodiscard]] const char* to_string(TamperTimeState state) noexcept;

[[nodiscard]] std::string to_json(const SecureClockPolicy& policy);
[[nodiscard]] std::string to_json(const SecureTimeStatus& status);
[[nodiscard]] std::string diagnostics_to_json(const std::vector<Diagnostic>& diagnostics);

}  // namespace dcplayer::security_api::secure_time
