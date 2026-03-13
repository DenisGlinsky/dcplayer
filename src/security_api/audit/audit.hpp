#pragma once

#include "secure_channel.hpp"

#include <compare>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dcplayer::security_api::audit {

using Diagnostic = secure_channel::Diagnostic;
using DiagnosticSeverity = secure_channel::DiagnosticSeverity;
using ValidationStatus = secure_channel::ValidationStatus;
using CheckResult = secure_channel::CheckResult;

template <typename T>
using ParseResult = secure_channel::ParseResult<T>;

using PeerRole = secure_channel::PeerRole;

enum class SecurityEventType {
    auth_session_authenticated,
    trust_peer_validated,
    acl_decision_recorded,
    api_request_processed,
    tamper_detected,
    tamper_lockout,
    recovery_started,
    recovery_completed,
    recovery_failed,
    export_audit_exported,
};

enum class SecurityEventSeverity {
    info,
    warning,
    error,
    critical,
};

enum class AuditCompleteness {
    complete,
    partial,
};

enum class IntegrityPlaceholderType {
    none,
    checksum_pending,
    signature_pending,
};

struct IdentitySummary {
    PeerRole role{PeerRole::ubuntu_tpm};
    std::string device_id;
    std::string certificate_fingerprint;

    [[nodiscard]] auto operator<=>(const IdentitySummary&) const = default;
};

struct DecisionSummary {
    std::string decision;

    [[nodiscard]] auto operator<=>(const DecisionSummary&) const = default;
};

struct ResultSummary {
    std::string result;

    [[nodiscard]] auto operator<=>(const ResultSummary&) const = default;
};

struct SecurityEvent {
    std::string event_id;
    SecurityEventType event_type{SecurityEventType::auth_session_authenticated};
    SecurityEventSeverity severity{SecurityEventSeverity::info};
    std::string event_time_utc;
    PeerRole producer_role{PeerRole::ubuntu_tpm};
    std::optional<IdentitySummary> actor_identity;
    std::optional<IdentitySummary> peer_identity;
    std::optional<std::string> correlation_id;
    std::optional<std::string> request_id;
    std::optional<std::string> export_id_ref;
    std::map<std::string, std::string> payload_metadata;
    std::optional<DecisionSummary> decision_summary;
    std::optional<ResultSummary> result_summary;

    [[nodiscard]] auto operator<=>(const SecurityEvent&) const = default;
};

struct OrderingRules {
    std::string ordered_by;
    std::string direction;

    [[nodiscard]] auto operator<=>(const OrderingRules&) const = default;
};

struct ExportMetadata {
    AuditCompleteness completeness{AuditCompleteness::complete};
    std::string export_reason;
    std::string source_channel_id;

    [[nodiscard]] auto operator<=>(const ExportMetadata&) const = default;
};

struct IntegrityMetadata {
    IntegrityPlaceholderType placeholder_type{IntegrityPlaceholderType::none};
    std::optional<std::string> placeholder_ref;

    [[nodiscard]] auto operator<=>(const IntegrityMetadata&) const = default;
};

struct AuditExport {
    std::string export_id;
    std::string export_time_utc;
    std::vector<SecurityEvent> events;
    OrderingRules ordering;
    ExportMetadata export_metadata;
    IntegrityMetadata integrity_metadata;

    [[nodiscard]] auto operator<=>(const AuditExport&) const = default;
};

[[nodiscard]] ParseResult<SecurityEvent> parse_security_event(std::string_view json);
[[nodiscard]] ParseResult<AuditExport> parse_audit_export(std::string_view json);
[[nodiscard]] CheckResult validate_audit_export(const AuditExport& audit_export);

[[nodiscard]] const char* to_string(SecurityEventType event_type) noexcept;
[[nodiscard]] const char* to_string(SecurityEventSeverity severity) noexcept;
[[nodiscard]] const char* to_string(AuditCompleteness completeness) noexcept;
[[nodiscard]] const char* to_string(IntegrityPlaceholderType placeholder_type) noexcept;

[[nodiscard]] std::string to_json(const SecurityEvent& event);
[[nodiscard]] std::string to_json(const AuditExport& audit_export);

}  // namespace dcplayer::security_api::audit
