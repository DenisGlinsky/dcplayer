#include "certificate_store.hpp"
#include "trust_chain.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

using dcplayer::security::certs::CertificateImportRecord;
using dcplayer::security::certs::CertificateRecord;
using dcplayer::security::certs::CertificateRole;
using dcplayer::security::certs::CertificateStoreBuilder;
using dcplayer::security::certs::CertificateStoreMetadata;
using dcplayer::security::certs::ChainEvaluationRequest;
using dcplayer::security::certs::DecisionReason;
using dcplayer::security::certs::RevocationEntry;
using dcplayer::security::certs::RevocationPolicy;
using dcplayer::security::certs::RevocationStatus;
using dcplayer::security::certs::TrustDecision;
using dcplayer::security::certs::ValidationStatus;

struct EvaluationFixture {
    std::string subject_fingerprint;
    std::string validation_time_utc;
    RevocationPolicy revocation_policy{RevocationPolicy::require_good};
    std::vector<std::string> checked_sources;
    std::vector<RevocationEntry> revocation_entries;
};

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string trim_copy(std::string_view input) {
    std::size_t begin = 0U;
    while (begin < input.size() && std::isspace(static_cast<unsigned char>(input[begin])) != 0) {
        ++begin;
    }

    std::size_t end = input.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1U])) != 0) {
        --end;
    }

    return std::string{input.substr(begin, end - begin)};
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

std::vector<std::string> split(std::string_view input, char delimiter) {
    std::vector<std::string> parts;
    std::string current;

    for (const char value : input) {
        if (value == delimiter) {
            parts.push_back(current);
            current.clear();
            continue;
        }
        current.push_back(value);
    }

    parts.push_back(current);
    return parts;
}

CertificateRole parse_role(std::string_view value) {
    const auto normalized = trim_copy(value);
    if (normalized == "root") {
        return CertificateRole::root;
    }
    if (normalized == "intermediate") {
        return CertificateRole::intermediate;
    }
    if (normalized == "server") {
        return CertificateRole::server;
    }
    if (normalized == "client") {
        return CertificateRole::client;
    }
    if (normalized == "signer") {
        return CertificateRole::signer;
    }
    return CertificateRole::unknown;
}

RevocationPolicy parse_revocation_policy(std::string_view value) {
    const auto normalized = trim_copy(value);
    if (normalized == "allow_unknown") {
        return RevocationPolicy::allow_unknown;
    }
    return RevocationPolicy::require_good;
}

dcplayer::security::certs::StoreBuildResult load_store_fixture(const std::filesystem::path& path) {
    CertificateStoreMetadata metadata;
    std::vector<std::string> revocation_sources;
    std::vector<CertificateImportRecord> roots;
    std::vector<CertificateImportRecord> intermediates;
    std::vector<CertificateImportRecord> device_certs;
    std::unordered_map<std::string, CertificateRole> issuer_role_hints;

    std::istringstream input(read_file(path));
    std::string line;
    while (std::getline(input, line)) {
        const auto trimmed = trim_copy(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        if (trimmed.rfind("record|", 0U) == 0U) {
            const auto parts = split(trimmed, '|');
            expect(parts.size() == 9U || parts.size() == 10U, "Invalid record fixture line: " + trimmed);

            CertificateImportRecord import_record{
                .certificate =
                    CertificateRecord{
                        .fingerprint = trim_copy(parts[2]),
                        .subject_dn = trim_copy(parts[3]),
                        .issuer_dn = trim_copy(parts[4]),
                        .role = parse_role(parts[5]),
                        .not_before_utc = trim_copy(parts[6]),
                        .not_after_utc = trim_copy(parts[7]),
                        .serial_number = trim_copy(parts[8]),
                    },
                .private_key_material = parts.size() == 10U ? trim_copy(parts[9]) : std::string{},
            };

            const auto collection = trim_copy(parts[1]);
            if (collection == "roots") {
                roots.push_back(std::move(import_record));
            } else if (collection == "intermediates") {
                intermediates.push_back(std::move(import_record));
            } else if (collection == "device_certs") {
                device_certs.push_back(std::move(import_record));
            } else {
                throw std::runtime_error("Unknown collection in fixture: " + collection);
            }
            continue;
        }

        if (trimmed.rfind("issuer_role_hint|", 0U) == 0U) {
            const auto parts = split(trimmed, '|');
            expect(parts.size() == 3U, "Invalid issuer_role_hint fixture line: " + trimmed);
            issuer_role_hints.emplace(trim_copy(parts[1]), parse_role(parts[2]));
            continue;
        }

        const auto delimiter = trimmed.find('=');
        expect(delimiter != std::string::npos, "Invalid key-value fixture line: " + trimmed);
        const auto key = trim_copy(std::string_view{trimmed}.substr(0U, delimiter));
        const auto value = trim_copy(std::string_view{trimmed}.substr(delimiter + 1U));

        if (key == "store_id") {
            metadata.store_id = value;
        } else if (key == "store_version") {
            metadata.store_version = value;
        } else if (key == "updated_at_utc") {
            metadata.updated_at_utc = value;
        } else if (key == "revocation_source") {
            revocation_sources.push_back(value);
        } else {
            throw std::runtime_error("Unknown fixture key: " + key);
        }
    }

    CertificateStoreBuilder builder(std::move(metadata));
    builder.set_revocation_sources(std::move(revocation_sources));
    for (auto& record : roots) {
        if (const auto iterator = issuer_role_hints.find(record.certificate.fingerprint); iterator != issuer_role_hints.end()) {
            record.certificate.issuer_role_hint = iterator->second;
        }
        builder.import_trust_anchor(std::move(record));
    }
    for (auto& record : intermediates) {
        if (const auto iterator = issuer_role_hints.find(record.certificate.fingerprint); iterator != issuer_role_hints.end()) {
            record.certificate.issuer_role_hint = iterator->second;
        }
        builder.import_intermediate(std::move(record));
    }
    for (auto& record : device_certs) {
        if (const auto iterator = issuer_role_hints.find(record.certificate.fingerprint); iterator != issuer_role_hints.end()) {
            record.certificate.issuer_role_hint = iterator->second;
        }
        builder.import_device_certificate(std::move(record));
    }
    return builder.build();
}

EvaluationFixture load_eval_fixture(const std::filesystem::path& path) {
    EvaluationFixture fixture;

    std::istringstream input(read_file(path));
    std::string line;
    while (std::getline(input, line)) {
        const auto trimmed = trim_copy(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        if (trimmed.rfind("revocation|", 0U) == 0U) {
            const auto parts = split(trimmed, '|');
            expect(parts.size() == 4U, "Invalid revocation fixture line: " + trimmed);
            fixture.revocation_entries.push_back(RevocationEntry{
                .fingerprint = trim_copy(parts[1]),
                .source = trim_copy(parts[3]),
                .state = trim_copy(parts[2]),
            });
            continue;
        }

        const auto delimiter = trimmed.find('=');
        expect(delimiter != std::string::npos, "Invalid key-value fixture line: " + trimmed);
        const auto key = trim_copy(std::string_view{trimmed}.substr(0U, delimiter));
        const auto value = trim_copy(std::string_view{trimmed}.substr(delimiter + 1U));

        if (key == "subject_fingerprint") {
            fixture.subject_fingerprint = value;
        } else if (key == "validation_time_utc") {
            fixture.validation_time_utc = value;
        } else if (key == "revocation_policy") {
            fixture.revocation_policy = parse_revocation_policy(value);
        } else if (key == "checked_source") {
            fixture.checked_sources.push_back(value);
        } else {
            throw std::runtime_error("Unknown evaluation fixture key: " + key);
        }
    }

    return fixture;
}

void expect_file_text(const std::string& actual, const std::filesystem::path& path) {
    expect(actual == read_file(path), "Unexpected fixture output at " + path.string());
}

dcplayer::security::certs::ChainEvaluationResult evaluate_fixture(
    const dcplayer::security::certs::CertificateStore& certificate_store,
    const std::filesystem::path& path) {
    const auto fixture = load_eval_fixture(path);
    return dcplayer::security::certs::evaluate_chain(ChainEvaluationRequest{
        .certificate_store = certificate_store,
        .subject_fingerprint = fixture.subject_fingerprint,
        .validation_time_utc = fixture.validation_time_utc,
        .checked_sources = fixture.checked_sources,
        .revocation_entries = fixture.revocation_entries,
        .revocation_policy = fixture.revocation_policy,
    });
}

}  // namespace

int main() {
    const std::filesystem::path fixture_root{PKI_FIXTURE_DIR};
    const auto valid_store_result = load_store_fixture(fixture_root / "valid" / "trusted_client.store");

    expect(valid_store_result.ok(), "Trusted client store fixture should build");
    expect(valid_store_result.validation_status == ValidationStatus::ok, "Trusted client store should be clean");
    expect(valid_store_result.certificate_store.has_value(), "Expected normalized certificate store");
    expect_file_text(dcplayer::security::certs::to_json(*valid_store_result.certificate_store),
                     fixture_root / "valid" / "trusted_client.store.json");

    {
        const auto result = evaluate_fixture(*valid_store_result.certificate_store, fixture_root / "valid" / "trusted_client.eval");

        expect(result.ok(), "Trusted client chain should evaluate cleanly");
        expect(result.validation_status == ValidationStatus::ok, "Trusted client chain should be ok");
        expect(result.diagnostics.empty(), "Trusted client chain should not emit diagnostics");
        expect(result.trust_chain.decision == TrustDecision::trusted, "Expected trusted decision");
        expect(result.trust_chain.decision_reason == DecisionReason::ok, "Expected ok reason");
        expect(result.trust_chain.revocation_status == RevocationStatus::good, "Expected good revocation status");
        expect_file_text(dcplayer::security::certs::to_json(result.trust_chain),
                         fixture_root / "valid" / "trusted_client.trust_chain.json");
    }

    {
        const auto result =
            evaluate_fixture(*valid_store_result.certificate_store, fixture_root / "valid" / "checked_source_good.eval");

        expect(result.ok(), "Configured checked revocation source should remain trusted");
        expect(result.validation_status == ValidationStatus::ok, "Expected ok validation status for checked source");
        expect(result.trust_chain.decision == TrustDecision::trusted, "Expected trusted decision for checked source");
        expect_file_text(dcplayer::security::certs::to_json(result.trust_chain),
                         fixture_root / "valid" / "checked_source_good.trust_chain.json");
    }

    {
        const auto result =
            evaluate_fixture(*valid_store_result.certificate_store, fixture_root / "valid" / "checked_source_with_extra_unknown.eval");

        expect(result.ok(), "Authoritative evidence with extra fabricated source should stay non-fatal");
        expect(result.validation_status == ValidationStatus::warning, "Expected warning validation status for extra unknown source");
        expect(result.trust_chain.decision == TrustDecision::trusted, "Expected trusted decision with authoritative evidence");
        expect(result.trust_chain.decision_reason == DecisionReason::ok, "Expected ok reason with authoritative evidence");
        expect(result.trust_chain.revocation_status == RevocationStatus::good, "Expected good revocation status");
        expect_file_text(dcplayer::security::certs::to_json(result.trust_chain),
                         fixture_root / "valid" / "checked_source_good.trust_chain.json");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(result.diagnostics),
                         fixture_root / "valid" / "checked_source_with_extra_unknown.diagnostics.json");
    }

    {
        const auto result = evaluate_fixture(*valid_store_result.certificate_store, fixture_root / "valid" / "allow_unknown.eval");

        expect(result.ok(), "allow_unknown revocation policy should stay non-fatal");
        expect(result.validation_status == ValidationStatus::warning, "Expected warning validation status");
        expect(result.trust_chain.decision == TrustDecision::warning_only, "Expected warning_only decision");
        expect(result.trust_chain.decision_reason == DecisionReason::revocation_unavailable,
               "Expected revocation_unavailable reason");
        expect(result.trust_chain.revocation_status == RevocationStatus::unknown, "Expected unknown revocation status");
        expect_file_text(dcplayer::security::certs::to_json(result.trust_chain),
                         fixture_root / "valid" / "allow_unknown.trust_chain.json");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(result.diagnostics),
                         fixture_root / "valid" / "allow_unknown.diagnostics.json");
    }

    {
        const auto result =
            evaluate_fixture(*valid_store_result.certificate_store, fixture_root / "invalid" / "unknown_revocation_source.eval");

        expect(!result.ok(), "Unknown revocation source must fail");
        expect(result.validation_status == ValidationStatus::error, "Unknown revocation source must fail closed");
        expect(result.trust_chain.decision_reason == DecisionReason::revocation_unavailable,
               "Expected revocation_unavailable reason for unknown source");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(result.diagnostics),
                         fixture_root / "invalid" / "unknown_revocation_source.diagnostics.json");
    }

    {
        const auto duplicate_result = load_store_fixture(fixture_root / "invalid" / "duplicate_fingerprint.store");

        expect(!duplicate_result.ok(), "Duplicate fingerprint fixture must fail");
        expect(duplicate_result.validation_status == ValidationStatus::error, "Expected duplicate fingerprint error");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(duplicate_result.diagnostics),
                         fixture_root / "invalid" / "duplicate_fingerprint.diagnostics.json");
    }

    {
        const auto invalid_role_result = load_store_fixture(fixture_root / "invalid" / "invalid_role.store");

        expect(!invalid_role_result.ok(), "Invalid role fixture must fail");
        expect(invalid_role_result.validation_status == ValidationStatus::error, "Expected invalid role error");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(invalid_role_result.diagnostics),
                         fixture_root / "invalid" / "invalid_role.diagnostics.json");
    }

    {
        const auto direct_missing_root_store = load_store_fixture(fixture_root / "invalid" / "direct_missing_root.store");

        expect(direct_missing_root_store.ok(), "Direct missing root store should still build");
        expect(direct_missing_root_store.certificate_store.has_value(), "Expected direct missing root store object");

        const auto result =
            evaluate_fixture(*direct_missing_root_store.certificate_store, fixture_root / "invalid" / "direct_missing_root.eval");

        expect(!result.ok(), "Direct missing root chain must fail");
        expect(result.trust_chain.decision_reason == DecisionReason::anchor_missing,
               "Expected anchor_missing reason for direct leaf missing root");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(result.diagnostics),
                         fixture_root / "invalid" / "direct_missing_root.diagnostics.json");
    }

    {
        const auto self_issued_leaf_store = load_store_fixture(fixture_root / "invalid" / "self_issued_leaf.store");

        expect(self_issued_leaf_store.ok(), "Self-issued leaf store should still build");
        expect(self_issued_leaf_store.certificate_store.has_value(), "Expected self-issued leaf store object");

        const auto result =
            evaluate_fixture(*self_issued_leaf_store.certificate_store, fixture_root / "invalid" / "self_issued_leaf.eval");

        expect(!result.ok(), "Self-issued leaf chain must fail");
        expect(result.trust_chain.decision_reason == DecisionReason::chain_broken,
               "Expected chain_broken reason for self-issued non-root leaf");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(result.diagnostics),
                         fixture_root / "invalid" / "self_issued_leaf.diagnostics.json");
    }

    {
        const auto missing_anchor_store = load_store_fixture(fixture_root / "invalid" / "missing_trust_anchor.store");

        expect(missing_anchor_store.ok(), "Missing anchor store should still build");
        expect(missing_anchor_store.certificate_store.has_value(), "Expected missing anchor store object");

        const auto result =
            evaluate_fixture(*missing_anchor_store.certificate_store, fixture_root / "invalid" / "missing_trust_anchor.eval");

        expect(!result.ok(), "Missing anchor chain must fail");
        expect(result.trust_chain.decision == TrustDecision::rejected, "Expected rejected decision for missing anchor");
        expect(result.trust_chain.decision_reason == DecisionReason::anchor_missing, "Expected anchor_missing reason");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(result.diagnostics),
                         fixture_root / "invalid" / "missing_trust_anchor.diagnostics.json");
    }

    {
        const auto missing_intermediate_store = load_store_fixture(fixture_root / "invalid" / "missing_intermediate.store");

        expect(missing_intermediate_store.ok(), "Missing intermediate store should still build");
        expect(missing_intermediate_store.certificate_store.has_value(), "Expected missing intermediate store object");

        const auto result = evaluate_fixture(*missing_intermediate_store.certificate_store,
                                             fixture_root / "invalid" / "missing_intermediate.eval");

        expect(!result.ok(), "Missing intermediate chain must fail");
        expect(result.trust_chain.decision_reason == DecisionReason::chain_broken,
               "Expected chain_broken reason for missing intermediate");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(result.diagnostics),
                         fixture_root / "invalid" / "missing_intermediate.diagnostics.json");
    }

    {
        const auto broken_chain_store = load_store_fixture(fixture_root / "invalid" / "broken_chain.store");

        expect(broken_chain_store.ok(), "Broken chain store should still build");
        expect(broken_chain_store.certificate_store.has_value(), "Expected broken chain store object");

        const auto result = evaluate_fixture(*broken_chain_store.certificate_store, fixture_root / "invalid" / "broken_chain.eval");

        expect(!result.ok(), "Broken chain must fail");
        expect(result.trust_chain.decision_reason == DecisionReason::chain_broken, "Expected chain_broken reason");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(result.diagnostics),
                         fixture_root / "invalid" / "broken_chain.diagnostics.json");
    }

    {
        const auto result =
            evaluate_fixture(*valid_store_result.certificate_store, fixture_root / "invalid" / "invalid_validation_time.eval");

        expect(!result.ok(), "Malformed validation_time_utc must fail");
        expect(result.trust_chain.decision_reason == DecisionReason::invalid_validation_time,
               "Expected invalid_validation_time reason");
        expect(result.trust_chain.revocation_status == RevocationStatus::not_checked,
               "Expected revocation status to remain not_checked");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(result.diagnostics),
                         fixture_root / "invalid" / "invalid_validation_time.diagnostics.json");
    }

    {
        const auto result = evaluate_fixture(*valid_store_result.certificate_store, fixture_root / "invalid" / "revoked.eval");

        expect(!result.ok(), "Revoked chain must fail");
        expect(result.trust_chain.decision_reason == DecisionReason::revoked, "Expected revoked reason");
        expect(result.trust_chain.revocation_status == RevocationStatus::revoked, "Expected revoked status");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(result.diagnostics),
                         fixture_root / "invalid" / "revoked.diagnostics.json");
    }

    {
        const auto result =
            evaluate_fixture(*valid_store_result.certificate_store, fixture_root / "invalid" / "invalid_revocation_state.eval");

        expect(!result.ok(), "Invalid revocation state must fail");
        expect(result.trust_chain.decision_reason == DecisionReason::invalid_revocation_state,
               "Expected invalid_revocation_state reason");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(result.diagnostics),
                         fixture_root / "invalid" / "invalid_revocation_state.diagnostics.json");
    }

    {
        const auto result =
            evaluate_fixture(*valid_store_result.certificate_store, fixture_root / "invalid" / "mixed_revocation_input.eval");

        expect(!result.ok(), "Mixed invalid and non-authoritative revocation input must fail");
        expect(result.trust_chain.decision_reason == DecisionReason::invalid_revocation_state,
               "Expected invalid_revocation_state reason for mixed revocation input");
        expect_file_text(dcplayer::security::certs::diagnostics_to_json(result.diagnostics),
                         fixture_root / "invalid" / "mixed_revocation_input.diagnostics.json");
    }

    return 0;
}
