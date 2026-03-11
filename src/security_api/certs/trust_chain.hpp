#pragma once

#include "certificate_store.hpp"

#include <compare>
#include <string>
#include <vector>

namespace dcplayer::security::certs {

enum class RevocationStatus {
    good,
    revoked,
    unknown,
    not_checked,
};

enum class TrustDecision {
    trusted,
    rejected,
    warning_only,
};

enum class DecisionReason {
    ok,
    anchor_missing,
    expired,
    not_yet_valid,
    revoked,
    chain_broken,
    algorithm_forbidden,
    revocation_unavailable,
    unsupported_certificate_role,
    invalid_revocation_state,
    invalid_validation_time,
};

enum class RevocationPolicy {
    require_good,
    allow_unknown,
};

struct RevocationEntry {
    std::string fingerprint;
    std::string source;
    std::string state;

    [[nodiscard]] auto operator<=>(const RevocationEntry&) const = default;
};

struct TrustChain {
    std::string subject_fingerprint;
    std::vector<std::string> issuer_chain;
    std::string validation_time_utc;
    RevocationStatus revocation_status{RevocationStatus::not_checked};
    TrustDecision decision{TrustDecision::rejected};
    DecisionReason decision_reason{DecisionReason::chain_broken};
    std::vector<std::string> checked_sources;

    [[nodiscard]] auto operator<=>(const TrustChain&) const = default;
};

struct ChainEvaluationRequest {
    CertificateStore certificate_store;
    std::string subject_fingerprint;
    std::string validation_time_utc;
    std::vector<std::string> checked_sources;
    std::vector<RevocationEntry> revocation_entries;
    RevocationPolicy revocation_policy{RevocationPolicy::require_good};
};

struct ChainEvaluationResult {
    TrustChain trust_chain;
    std::vector<Diagnostic> diagnostics;
    ValidationStatus validation_status{ValidationStatus::ok};

    [[nodiscard]] bool ok() const noexcept {
        return validation_status != ValidationStatus::error;
    }
};

[[nodiscard]] ChainEvaluationResult evaluate_chain(const ChainEvaluationRequest& request);
[[nodiscard]] std::string to_json(const TrustChain& trust_chain);
[[nodiscard]] const char* to_string(RevocationStatus status) noexcept;
[[nodiscard]] const char* to_string(TrustDecision decision) noexcept;
[[nodiscard]] const char* to_string(DecisionReason reason) noexcept;
[[nodiscard]] const char* to_string(RevocationPolicy policy) noexcept;

}  // namespace dcplayer::security::certs
