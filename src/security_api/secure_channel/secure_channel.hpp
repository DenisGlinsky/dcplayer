#pragma once

#include <compare>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dcplayer::security_api::secure_channel {

enum class DiagnosticSeverity {
    error,
    warning,
};

enum class ValidationStatus {
    ok,
    warning,
    error,
};

enum class PeerRole {
    pi_zymkey,
    ubuntu_tpm,
};

enum class ApiName {
    sign,
    unwrap,
    decrypt,
    health,
    identity,
};

enum class TrustDecision {
    trusted,
    rejected,
    warning_only,
};

enum class TrustDecisionReason {
    ok,
    anchor_missing,
    expired,
    not_yet_valid,
    revoked,
    chain_broken,
    algorithm_forbidden,
    revocation_unavailable,
};

enum class RevocationStatus {
    good,
    revoked,
    unknown,
    not_checked,
};

enum class EnvelopeStatus {
    ok,
    denied,
    error,
};

struct Diagnostic {
    std::string code;
    DiagnosticSeverity severity;
    std::string path;
    std::string message;

    [[nodiscard]] auto operator<=>(const Diagnostic&) const = default;
};

struct PeerIdentity {
    PeerRole role{PeerRole::ubuntu_tpm};
    std::string device_id;
    std::string certificate_fingerprint;
    std::string subject_dn;
    std::vector<std::string> san_dns_names;
    std::vector<std::string> san_uri_names;

    [[nodiscard]] auto operator<=>(const PeerIdentity&) const = default;
};

struct TrustRequirements {
    std::string certificate_store_ref;
    TrustDecision required_decision{TrustDecision::trusted};
    TrustDecisionReason required_decision_reason{TrustDecisionReason::ok};
    std::vector<RevocationStatus> accepted_revocation_statuses;
    std::vector<std::string> required_checked_sources;

    [[nodiscard]] auto operator<=>(const TrustRequirements&) const = default;
};

struct TrustSummary {
    std::string subject_fingerprint;
    std::vector<std::string> issuer_chain;
    std::string validation_time_utc;
    RevocationStatus revocation_status{RevocationStatus::good};
    TrustDecision decision{TrustDecision::trusted};
    TrustDecisionReason decision_reason{TrustDecisionReason::ok};
    std::vector<std::string> checked_sources;

    [[nodiscard]] auto operator<=>(const TrustSummary&) const = default;
};

struct AuthContextSummary {
    std::string channel_id;
    bool mutual_tls{false};
    PeerRole local_role{PeerRole::pi_zymkey};
    TrustSummary peer_trust;
    std::vector<std::string> checked_sources;

    [[nodiscard]] auto operator<=>(const AuthContextSummary&) const = default;
};

struct PayloadContract {
    std::string payload_type;
    std::string schema_ref;
    std::map<std::string, std::string> body;

    [[nodiscard]] auto operator<=>(const PayloadContract&) const = default;
};

struct AclRule {
    PeerRole caller_role{PeerRole::ubuntu_tpm};
    std::vector<ApiName> allowed_api_names;

    [[nodiscard]] auto operator<=>(const AclRule&) const = default;
};

struct SecureChannelContract {
    std::string channel_id;
    PeerRole server_role{PeerRole::pi_zymkey};
    PeerRole client_role{PeerRole::ubuntu_tpm};
    std::string tls_profile;
    TrustRequirements trust_requirements;
    PeerIdentity server_identity;
    PeerIdentity client_identity;
    std::vector<AclRule> acl;

    [[nodiscard]] auto operator<=>(const SecureChannelContract&) const = default;
};

struct ProtectedRequestEnvelope {
    std::string request_id;
    ApiName api_name{ApiName::health};
    PeerRole caller_role{PeerRole::ubuntu_tpm};
    PeerIdentity caller_identity;
    AuthContextSummary auth_context;
    PayloadContract payload;

    [[nodiscard]] auto operator<=>(const ProtectedRequestEnvelope&) const = default;
};

struct ProtectedResponseEnvelope {
    std::string request_id;
    EnvelopeStatus status{EnvelopeStatus::ok};
    std::vector<Diagnostic> diagnostics;
    PayloadContract payload;

    [[nodiscard]] auto operator<=>(const ProtectedResponseEnvelope&) const = default;
};

struct SecurityModuleContract {
    std::string module_id;
    PeerRole module_role{PeerRole::pi_zymkey};
    std::string secure_channel_id;
    std::string request_envelope_ref;
    std::string response_envelope_ref;
    std::vector<ApiName> supported_api_names;
    PeerIdentity server_identity;

    [[nodiscard]] auto operator<=>(const SecurityModuleContract&) const = default;
};

template <typename T>
struct ParseResult {
    std::optional<T> value;
    std::vector<Diagnostic> diagnostics;
    ValidationStatus validation_status{ValidationStatus::ok};

    [[nodiscard]] bool ok() const noexcept {
        return validation_status != ValidationStatus::error;
    }
};

struct CheckResult {
    std::vector<Diagnostic> diagnostics;
    ValidationStatus validation_status{ValidationStatus::ok};

    [[nodiscard]] bool ok() const noexcept {
        return validation_status != ValidationStatus::error;
    }
};

[[nodiscard]] ParseResult<SecureChannelContract> parse_secure_channel_contract(std::string_view json);
[[nodiscard]] ParseResult<ProtectedRequestEnvelope> parse_protected_request_envelope(std::string_view json);
[[nodiscard]] ParseResult<ProtectedResponseEnvelope> parse_protected_response_envelope(std::string_view json);
[[nodiscard]] ParseResult<SecurityModuleContract> parse_security_module_contract(std::string_view json);

[[nodiscard]] CheckResult authorize_request(const SecureChannelContract& contract,
                                            const ProtectedRequestEnvelope& request);
[[nodiscard]] CheckResult validate_response(const SecureChannelContract& contract,
                                            const ProtectedRequestEnvelope& request,
                                            const ProtectedResponseEnvelope& response);

[[nodiscard]] const char* to_string(DiagnosticSeverity severity) noexcept;
[[nodiscard]] const char* to_string(ValidationStatus status) noexcept;
[[nodiscard]] const char* to_string(PeerRole role) noexcept;
[[nodiscard]] const char* to_string(ApiName api_name) noexcept;
[[nodiscard]] const char* to_string(TrustDecision decision) noexcept;
[[nodiscard]] const char* to_string(TrustDecisionReason reason) noexcept;
[[nodiscard]] const char* to_string(RevocationStatus status) noexcept;
[[nodiscard]] const char* to_string(EnvelopeStatus status) noexcept;

[[nodiscard]] std::string to_json(const SecureChannelContract& contract);
[[nodiscard]] std::string to_json(const ProtectedRequestEnvelope& envelope);
[[nodiscard]] std::string to_json(const ProtectedResponseEnvelope& envelope);
[[nodiscard]] std::string to_json(const SecurityModuleContract& contract);
[[nodiscard]] std::string diagnostics_to_json(const std::vector<Diagnostic>& diagnostics);

}  // namespace dcplayer::security_api::secure_channel
