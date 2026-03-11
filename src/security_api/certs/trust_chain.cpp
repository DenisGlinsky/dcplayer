#include "trust_chain.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace dcplayer::security::certs {
namespace {

struct RecordLocation {
    const CertificateRecord* record{nullptr};
    std::string path;
};

struct RevocationLookupEntry {
    RevocationStatus status{RevocationStatus::unknown};
};

struct RevocationLookupKey {
    std::string fingerprint;
    std::string source;

    [[nodiscard]] bool operator==(const RevocationLookupKey& other) const noexcept {
        return fingerprint == other.fingerprint && source == other.source;
    }
};

struct RevocationLookupKeyHash {
    [[nodiscard]] std::size_t operator()(const RevocationLookupKey& key) const noexcept {
        return std::hash<std::string>{}(key.fingerprint) ^ (std::hash<std::string>{}(key.source) << 1U);
    }
};

[[nodiscard]] bool is_error(const Diagnostic& diagnostic) noexcept {
    return diagnostic.severity == DiagnosticSeverity::error;
}

[[nodiscard]] ValidationStatus validation_status_for(const std::vector<Diagnostic>& diagnostics) noexcept {
    if (std::any_of(diagnostics.begin(), diagnostics.end(), is_error)) {
        return ValidationStatus::error;
    }
    if (!diagnostics.empty()) {
        return ValidationStatus::warning;
    }
    return ValidationStatus::ok;
}

void sort_diagnostics(std::vector<Diagnostic>& diagnostics) {
    std::sort(diagnostics.begin(), diagnostics.end(), [](const Diagnostic& lhs, const Diagnostic& rhs) {
        const auto severity_rank = [](DiagnosticSeverity severity) noexcept {
            return severity == DiagnosticSeverity::error ? 0 : 1;
        };

        return std::tuple{severity_rank(lhs.severity), lhs.code, lhs.path, lhs.message} <
               std::tuple{severity_rank(rhs.severity), rhs.code, rhs.path, rhs.message};
    });
}

[[nodiscard]] bool is_leap_year(int year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

[[nodiscard]] int days_in_month(int year, int month) noexcept {
    static constexpr int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2) {
        return is_leap_year(year) ? 29 : 28;
    }
    return kDays[month - 1];
}

[[nodiscard]] bool is_valid_utc_timestamp(std::string_view input) {
    if (input.size() != 20U) {
        return false;
    }

    const auto digit = [&](std::size_t index) noexcept {
        return std::isdigit(static_cast<unsigned char>(input[index])) != 0;
    };
    const bool structure_ok = digit(0U) && digit(1U) && digit(2U) && digit(3U) && input[4U] == '-' && digit(5U) &&
                              digit(6U) && input[7U] == '-' && digit(8U) && digit(9U) && input[10U] == 'T' &&
                              digit(11U) && digit(12U) && input[13U] == ':' && digit(14U) && digit(15U) &&
                              input[16U] == ':' && digit(17U) && digit(18U) && input[19U] == 'Z';
    if (!structure_ok) {
        return false;
    }

    const int year = std::stoi(std::string{input.substr(0U, 4U)});
    const int month = std::stoi(std::string{input.substr(5U, 2U)});
    const int day = std::stoi(std::string{input.substr(8U, 2U)});
    const int hour = std::stoi(std::string{input.substr(11U, 2U)});
    const int minute = std::stoi(std::string{input.substr(14U, 2U)});
    const int second = std::stoi(std::string{input.substr(17U, 2U)});

    if (month < 1 || month > 12) {
        return false;
    }
    if (day < 1 || day > days_in_month(year, month)) {
        return false;
    }
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return false;
    }
    return true;
}

void add_diagnostic(std::vector<Diagnostic>& diagnostics,
                    std::string code,
                    DiagnosticSeverity severity,
                    std::string path,
                    std::string message) {
    diagnostics.push_back(Diagnostic{
        .code = std::move(code),
        .severity = severity,
        .path = std::move(path),
        .message = std::move(message),
    });
}

[[nodiscard]] std::optional<std::string> normalize_fingerprint(std::string_view fingerprint) {
    if (fingerprint.size() != 64U) {
        return std::nullopt;
    }

    std::string normalized;
    normalized.reserve(fingerprint.size());
    for (const char current : fingerprint) {
        if (!std::isxdigit(static_cast<unsigned char>(current))) {
            return std::nullopt;
        }
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(current))));
    }
    return normalized;
}

[[nodiscard]] std::vector<RecordLocation> collect_records(const CertificateStore& certificate_store) {
    std::vector<RecordLocation> records;
    records.reserve(certificate_store.roots.size() + certificate_store.intermediates.size() + certificate_store.device_certs.size());

    for (std::size_t index = 0U; index < certificate_store.roots.size(); ++index) {
        records.push_back(RecordLocation{
            .record = &certificate_store.roots[index],
            .path = "/roots[" + std::to_string(index + 1U) + "]",
        });
    }
    for (std::size_t index = 0U; index < certificate_store.intermediates.size(); ++index) {
        records.push_back(RecordLocation{
            .record = &certificate_store.intermediates[index],
            .path = "/intermediates[" + std::to_string(index + 1U) + "]",
        });
    }
    for (std::size_t index = 0U; index < certificate_store.device_certs.size(); ++index) {
        records.push_back(RecordLocation{
            .record = &certificate_store.device_certs[index],
            .path = "/device_certs[" + std::to_string(index + 1U) + "]",
        });
    }

    return records;
}

[[nodiscard]] std::optional<RecordLocation> find_by_fingerprint(const std::vector<RecordLocation>& records,
                                                                std::string_view fingerprint) {
    const auto iterator = std::find_if(records.begin(), records.end(), [&](const RecordLocation& location) {
        return location.record->fingerprint == fingerprint;
    });
    if (iterator == records.end()) {
        return std::nullopt;
    }
    return *iterator;
}

[[nodiscard]] std::vector<RecordLocation> find_by_subject_dn(const std::vector<RecordLocation>& records, std::string_view subject_dn) {
    std::vector<RecordLocation> matches;
    for (const auto& location : records) {
        if (location.record->subject_dn == subject_dn) {
            matches.push_back(location);
        }
    }
    return matches;
}

[[nodiscard]] std::string issuer_path(std::size_t issuer_index) {
    return "/issuer_chain[" + std::to_string(issuer_index) + "]";
}

[[nodiscard]] bool is_device_role(CertificateRole role) noexcept {
    return role == CertificateRole::server || role == CertificateRole::client || role == CertificateRole::signer;
}

[[nodiscard]] bool missing_issuer_is_anchor_missing(const CertificateRecord& record, std::size_t resolved_chain_size) noexcept {
    if (record.role == CertificateRole::intermediate) {
        return true;
    }

    return resolved_chain_size == 1U && is_device_role(record.role) && record.issuer_role_hint == CertificateRole::root;
}

[[nodiscard]] std::vector<std::string> normalized_unique_sources(std::vector<std::string> sources) {
    std::sort(sources.begin(), sources.end());
    sources.erase(std::unique(sources.begin(), sources.end()), sources.end());
    return sources;
}

[[nodiscard]] bool contains_source(const std::vector<std::string>& sources, std::string_view source) {
    return std::find(sources.begin(), sources.end(), source) != sources.end();
}

[[nodiscard]] DiagnosticSeverity severity_for_revocation_policy(RevocationPolicy policy) noexcept {
    return policy == RevocationPolicy::allow_unknown ? DiagnosticSeverity::warning : DiagnosticSeverity::error;
}

[[nodiscard]] TrustDecision decision_for_revocation_policy(RevocationPolicy policy) noexcept {
    return policy == RevocationPolicy::allow_unknown ? TrustDecision::warning_only : TrustDecision::rejected;
}

[[nodiscard]] std::optional<DecisionReason> time_window_reason(const CertificateRecord& record, std::string_view validation_time_utc) {
    if (validation_time_utc < record.not_before_utc) {
        return DecisionReason::not_yet_valid;
    }
    if (validation_time_utc > record.not_after_utc) {
        return DecisionReason::expired;
    }
    return std::nullopt;
}

[[nodiscard]] std::string time_window_path(std::string_view record_path, DecisionReason reason) {
    if (reason == DecisionReason::not_yet_valid) {
        return std::string{record_path} + "/not_before_utc";
    }
    return std::string{record_path} + "/not_after_utc";
}

[[nodiscard]] std::string time_window_message(DecisionReason reason) {
    if (reason == DecisionReason::not_yet_valid) {
        return "Certificate is not yet valid at validation_time_utc";
    }
    return "Certificate is expired at validation_time_utc";
}

[[nodiscard]] std::optional<RevocationLookupEntry> parse_revocation_entry(const RevocationEntry& entry) {
    if (entry.state == "good") {
        return RevocationLookupEntry{.status = RevocationStatus::good};
    }
    if (entry.state == "revoked") {
        return RevocationLookupEntry{.status = RevocationStatus::revoked};
    }
    if (entry.state == "unknown") {
        return RevocationLookupEntry{.status = RevocationStatus::unknown};
    }
    if (entry.state == "not_checked") {
        return RevocationLookupEntry{.status = RevocationStatus::not_checked};
    }
    return std::nullopt;
}

void append_json_string(std::ostringstream& output, std::string_view value) {
    output << '"';
    for (const char current : value) {
        switch (current) {
            case '\\':
                output << "\\\\";
                break;
            case '"':
                output << "\\\"";
                break;
            case '\n':
                output << "\\n";
                break;
            case '\r':
                output << "\\r";
                break;
            case '\t':
                output << "\\t";
                break;
            default:
                output << current;
                break;
        }
    }
    output << '"';
}

void finalize_result(ChainEvaluationResult& result) {
    sort_diagnostics(result.diagnostics);
    result.validation_status = validation_status_for(result.diagnostics);
}

}  // namespace

ChainEvaluationResult evaluate_chain(const ChainEvaluationRequest& request) {
    const auto configured_sources = normalized_unique_sources(request.certificate_store.revocation_sources);
    const auto requested_checked_sources = normalized_unique_sources(request.checked_sources);

    std::vector<std::string> effective_checked_sources;
    if (requested_checked_sources.empty()) {
        effective_checked_sources = configured_sources;
    } else {
        for (const auto& source : requested_checked_sources) {
            if (contains_source(configured_sources, source)) {
                effective_checked_sources.push_back(source);
            }
        }
    }

    ChainEvaluationResult result{
        .trust_chain =
            TrustChain{
                .subject_fingerprint = request.subject_fingerprint,
                .validation_time_utc = request.validation_time_utc,
                .revocation_status = RevocationStatus::not_checked,
                .decision = TrustDecision::rejected,
                .decision_reason = DecisionReason::chain_broken,
                .checked_sources = std::move(effective_checked_sources),
            },
    };

    if (!is_valid_utc_timestamp(request.validation_time_utc)) {
        add_diagnostic(result.diagnostics,
                       "trust_chain.invalid_validation_time",
                       DiagnosticSeverity::error,
                       "/validation_time_utc",
                       "validation_time_utc must be a UTC timestamp");
        result.trust_chain.decision_reason = DecisionReason::invalid_validation_time;
        finalize_result(result);
        return result;
    }

    const auto normalized_subject = normalize_fingerprint(request.subject_fingerprint);
    if (!normalized_subject.has_value()) {
        add_diagnostic(result.diagnostics,
                       "trust_chain.chain_broken",
                       DiagnosticSeverity::error,
                       "/subject_fingerprint",
                       "Subject certificate record is missing from CertificateStore");
        finalize_result(result);
        return result;
    }
    result.trust_chain.subject_fingerprint = *normalized_subject;

    const auto records = collect_records(request.certificate_store);
    const auto subject = find_by_fingerprint(records, *normalized_subject);
    if (!subject.has_value()) {
        add_diagnostic(result.diagnostics,
                       "trust_chain.chain_broken",
                       DiagnosticSeverity::error,
                       "/subject_fingerprint",
                       "Subject certificate record is missing from CertificateStore");
        finalize_result(result);
        return result;
    }

    if (subject->record->role != CertificateRole::server && subject->record->role != CertificateRole::client &&
        subject->record->role != CertificateRole::signer) {
        add_diagnostic(result.diagnostics,
                       "trust_chain.unsupported_certificate_role",
                       DiagnosticSeverity::error,
                       "/subject_fingerprint",
                       "Certificate role is not supported for chain evaluation");
        result.trust_chain.decision_reason = DecisionReason::unsupported_certificate_role;
        finalize_result(result);
        return result;
    }

    if (const auto reason = time_window_reason(*subject->record, request.validation_time_utc); reason.has_value()) {
        add_diagnostic(result.diagnostics,
                       reason == DecisionReason::not_yet_valid ? "trust_chain.not_yet_valid" : "trust_chain.expired",
                       DiagnosticSeverity::error,
                       time_window_path("/subject", *reason),
                       time_window_message(*reason));
        result.trust_chain.decision_reason = *reason;
        finalize_result(result);
        return result;
    }

    std::vector<RecordLocation> resolved_chain;
    resolved_chain.push_back(*subject);
    std::unordered_set<std::string> visited{subject->record->fingerprint};
    RecordLocation current = *subject;

    while (true) {
        if (current.record->role == CertificateRole::root) {
            result.trust_chain.decision = TrustDecision::trusted;
            result.trust_chain.decision_reason = DecisionReason::ok;
            break;
        }

        if (current.record->subject_dn == current.record->issuer_dn) {
            const bool anchor_missing = current.record->role == CertificateRole::intermediate;
            add_diagnostic(result.diagnostics,
                           anchor_missing ? "trust_chain.anchor_missing" : "trust_chain.chain_broken",
                           DiagnosticSeverity::error,
                           issuer_path(resolved_chain.size()),
                           anchor_missing ? "Trust anchor is missing for the issuer chain"
                                          : "Issuer chain contains a non-root self-issued certificate");
            result.trust_chain.decision_reason = anchor_missing ? DecisionReason::anchor_missing : DecisionReason::chain_broken;
            finalize_result(result);
            return result;
        }

        auto issuer_candidates = find_by_subject_dn(records, current.record->issuer_dn);
        if (issuer_candidates.empty()) {
            const bool anchor_missing = missing_issuer_is_anchor_missing(*current.record, resolved_chain.size());
            add_diagnostic(result.diagnostics,
                           anchor_missing ? "trust_chain.anchor_missing" : "trust_chain.chain_broken",
                           DiagnosticSeverity::error,
                           issuer_path(resolved_chain.size()),
                           anchor_missing ? "Trust anchor is missing for the issuer chain"
                                          : "Issuer certificate record is missing from CertificateStore");
            result.trust_chain.decision_reason = anchor_missing ? DecisionReason::anchor_missing : DecisionReason::chain_broken;
            finalize_result(result);
            return result;
        }

        if (issuer_candidates.size() > 1U) {
            add_diagnostic(result.diagnostics,
                           "trust_chain.chain_broken",
                           DiagnosticSeverity::error,
                           issuer_path(resolved_chain.size()),
                           "Issuer chain is ambiguous");
            result.trust_chain.decision_reason = DecisionReason::chain_broken;
            finalize_result(result);
            return result;
        }

        const RecordLocation issuer = issuer_candidates.front();
        if (visited.contains(issuer.record->fingerprint)) {
            add_diagnostic(result.diagnostics,
                           "trust_chain.chain_broken",
                           DiagnosticSeverity::error,
                           issuer_path(resolved_chain.size()),
                           "Issuer chain contains a cycle");
            result.trust_chain.decision_reason = DecisionReason::chain_broken;
            finalize_result(result);
            return result;
        }

        if (issuer.record->role != CertificateRole::intermediate && issuer.record->role != CertificateRole::root) {
            add_diagnostic(result.diagnostics,
                           "trust_chain.unsupported_certificate_role",
                           DiagnosticSeverity::error,
                           issuer_path(resolved_chain.size()) + "/role",
                           "Certificate role is not supported for chain evaluation");
            result.trust_chain.decision_reason = DecisionReason::unsupported_certificate_role;
            finalize_result(result);
            return result;
        }

        resolved_chain.push_back(issuer);
        result.trust_chain.issuer_chain.push_back(issuer.record->fingerprint);
        visited.insert(issuer.record->fingerprint);

        if (const auto reason = time_window_reason(*issuer.record, request.validation_time_utc); reason.has_value()) {
            const std::string record_path = issuer_path(result.trust_chain.issuer_chain.size());
            add_diagnostic(result.diagnostics,
                           reason == DecisionReason::not_yet_valid ? "trust_chain.not_yet_valid" : "trust_chain.expired",
                           DiagnosticSeverity::error,
                           time_window_path(record_path, *reason),
                           time_window_message(*reason));
            result.trust_chain.decision_reason = *reason;
            finalize_result(result);
            return result;
        }

        current = issuer;
    }

    if (request.certificate_store.revocation_sources.empty()) {
        finalize_result(result);
        return result;
    }

    std::unordered_map<RevocationLookupKey, RevocationLookupEntry, RevocationLookupKeyHash> revocation_lookup;
    bool has_invalid_revocation_state = false;
    for (std::size_t index = 0U; index < request.revocation_entries.size(); ++index) {
        const auto normalized_fingerprint = normalize_fingerprint(request.revocation_entries[index].fingerprint);
        const auto parsed_state = parse_revocation_entry(request.revocation_entries[index]);
        if (!normalized_fingerprint.has_value() || request.revocation_entries[index].source.empty() || !parsed_state.has_value()) {
            add_diagnostic(result.diagnostics,
                           "trust_chain.invalid_revocation_state",
                           DiagnosticSeverity::error,
                           "/revocation_entries[" + std::to_string(index + 1U) + "]",
                           "Revocation entry state is invalid for chain evaluation");
            has_invalid_revocation_state = true;
            continue;
        }

        const bool source_configured = contains_source(configured_sources, request.revocation_entries[index].source);
        const bool source_checked = requested_checked_sources.empty() ||
                                    contains_source(requested_checked_sources, request.revocation_entries[index].source);
        if (!source_configured || !source_checked) {
            add_diagnostic(result.diagnostics,
                           "trust_chain.unknown_revocation_source",
                           DiagnosticSeverity::warning,
                           "/revocation_entries[" + std::to_string(index + 1U) + "]/source",
                           "Revocation source is not authoritative for chain evaluation");
            continue;
        }

        const RevocationLookupKey key{
            .fingerprint = *normalized_fingerprint,
            .source = request.revocation_entries[index].source,
        };
        const auto [iterator, inserted] = revocation_lookup.emplace(key, *parsed_state);
        if (!inserted && iterator->second.status != parsed_state->status) {
            add_diagnostic(result.diagnostics,
                           "trust_chain.invalid_revocation_state",
                           DiagnosticSeverity::error,
                           "/revocation_entries[" + std::to_string(index + 1U) + "]",
                           "Revocation entry state is invalid for chain evaluation");
            has_invalid_revocation_state = true;
        }
    }

    if (has_invalid_revocation_state) {
        result.trust_chain.revocation_status = RevocationStatus::unknown;
        result.trust_chain.decision_reason = DecisionReason::invalid_revocation_state;
        finalize_result(result);
        return result;
    }

    for (std::size_t index = 0U; index < resolved_chain.size(); ++index) {
        const auto& record = resolved_chain[index];
        const auto path = index == 0U ? "/subject/revocation_status" : issuer_path(index) + "/revocation_status";
        bool has_authoritative_evidence = false;
        for (const auto& source : result.trust_chain.checked_sources) {
            const auto iterator = revocation_lookup.find(RevocationLookupKey{
                .fingerprint = record.record->fingerprint,
                .source = source,
            });
            if (iterator == revocation_lookup.end()) {
                continue;
            }

            has_authoritative_evidence = true;
            if (iterator->second.status == RevocationStatus::revoked) {
                result.trust_chain.revocation_status = RevocationStatus::revoked;
                add_diagnostic(result.diagnostics,
                               "trust_chain.revoked",
                               DiagnosticSeverity::error,
                               path,
                               "Certificate is revoked by revocation contract");
                result.trust_chain.decision = TrustDecision::rejected;
                result.trust_chain.decision_reason = DecisionReason::revoked;
                finalize_result(result);
                return result;
            }

            if (iterator->second.status == RevocationStatus::unknown || iterator->second.status == RevocationStatus::not_checked) {
                result.trust_chain.revocation_status = RevocationStatus::unknown;
                add_diagnostic(result.diagnostics,
                               "trust_chain.revocation_unavailable",
                               request.revocation_policy == RevocationPolicy::allow_unknown ? DiagnosticSeverity::warning
                                                                                            : DiagnosticSeverity::error,
                               path,
                               "Revocation status is unavailable for the issuer chain");
                result.trust_chain.decision =
                    request.revocation_policy == RevocationPolicy::allow_unknown ? TrustDecision::warning_only
                                                                                 : TrustDecision::rejected;
                result.trust_chain.decision_reason = DecisionReason::revocation_unavailable;
                finalize_result(result);
                return result;
            }
        }

        if (!has_authoritative_evidence) {
            result.trust_chain.revocation_status = RevocationStatus::unknown;
            add_diagnostic(result.diagnostics,
                           "trust_chain.revocation_unavailable",
                           severity_for_revocation_policy(request.revocation_policy),
                           path,
                           "Revocation status is unavailable for the issuer chain");
            result.trust_chain.decision = decision_for_revocation_policy(request.revocation_policy);
            result.trust_chain.decision_reason = DecisionReason::revocation_unavailable;
            finalize_result(result);
            return result;
        }
    }

    result.trust_chain.revocation_status = RevocationStatus::good;
    finalize_result(result);
    return result;
}

std::string to_json(const TrustChain& trust_chain) {
    std::ostringstream output;
    output << "{\n";
    output << "  \"subject_fingerprint\": ";
    append_json_string(output, trust_chain.subject_fingerprint);
    output << ",\n";
    output << "  \"issuer_chain\": [\n";
    for (std::size_t index = 0U; index < trust_chain.issuer_chain.size(); ++index) {
        output << "    ";
        append_json_string(output, trust_chain.issuer_chain[index]);
        output << (index + 1U == trust_chain.issuer_chain.size() ? '\n' : ',');
        if (index + 1U != trust_chain.issuer_chain.size()) {
            output << '\n';
        }
    }
    output << "  ],\n";
    output << "  \"validation_time_utc\": ";
    append_json_string(output, trust_chain.validation_time_utc);
    output << ",\n";
    output << "  \"revocation_status\": ";
    append_json_string(output, to_string(trust_chain.revocation_status));
    output << ",\n";
    output << "  \"decision\": ";
    append_json_string(output, to_string(trust_chain.decision));
    output << ",\n";
    output << "  \"decision_reason\": ";
    append_json_string(output, to_string(trust_chain.decision_reason));
    output << ",\n";
    output << "  \"checked_sources\": [\n";
    for (std::size_t index = 0U; index < trust_chain.checked_sources.size(); ++index) {
        output << "    ";
        append_json_string(output, trust_chain.checked_sources[index]);
        output << (index + 1U == trust_chain.checked_sources.size() ? '\n' : ',');
        if (index + 1U != trust_chain.checked_sources.size()) {
            output << '\n';
        }
    }
    output << "  ]\n";
    output << "}\n";
    return output.str();
}

const char* to_string(RevocationStatus status) noexcept {
    switch (status) {
        case RevocationStatus::good:
            return "good";
        case RevocationStatus::revoked:
            return "revoked";
        case RevocationStatus::unknown:
            return "unknown";
        case RevocationStatus::not_checked:
            return "not_checked";
    }
    return "unknown";
}

const char* to_string(TrustDecision decision) noexcept {
    switch (decision) {
        case TrustDecision::trusted:
            return "trusted";
        case TrustDecision::rejected:
            return "rejected";
        case TrustDecision::warning_only:
            return "warning_only";
    }
    return "unknown";
}

const char* to_string(DecisionReason reason) noexcept {
    switch (reason) {
        case DecisionReason::ok:
            return "ok";
        case DecisionReason::anchor_missing:
            return "anchor_missing";
        case DecisionReason::expired:
            return "expired";
        case DecisionReason::not_yet_valid:
            return "not_yet_valid";
        case DecisionReason::revoked:
            return "revoked";
        case DecisionReason::chain_broken:
            return "chain_broken";
        case DecisionReason::algorithm_forbidden:
            return "algorithm_forbidden";
        case DecisionReason::revocation_unavailable:
            return "revocation_unavailable";
        case DecisionReason::unsupported_certificate_role:
            return "unsupported_certificate_role";
        case DecisionReason::invalid_revocation_state:
            return "invalid_revocation_state";
        case DecisionReason::invalid_validation_time:
            return "invalid_validation_time";
    }
    return "unknown";
}

const char* to_string(RevocationPolicy policy) noexcept {
    switch (policy) {
        case RevocationPolicy::require_good:
            return "require_good";
        case RevocationPolicy::allow_unknown:
            return "allow_unknown";
    }
    return "unknown";
}

}  // namespace dcplayer::security::certs
