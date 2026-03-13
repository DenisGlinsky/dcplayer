#include "audit.hpp"
#include "secure_channel.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

void expect_diagnostics_json(const std::vector<dcplayer::security_api::audit::Diagnostic>& diagnostics,
                             const std::string& expected_json,
                             const std::string& label) {
    namespace secure_channel = dcplayer::security_api::secure_channel;

    expect(secure_channel::diagnostics_to_json(diagnostics) == expected_json, label);
}

dcplayer::security_api::audit::IdentitySummary make_identity(dcplayer::security_api::audit::PeerRole role,
                                                             std::string device_id,
                                                             std::string fingerprint) {
    return dcplayer::security_api::audit::IdentitySummary{
        .role = role,
        .device_id = std::move(device_id),
        .certificate_fingerprint = std::move(fingerprint),
    };
}

dcplayer::security_api::audit::SecurityEvent make_api_event() {
    namespace audit = dcplayer::security_api::audit;

    return audit::SecurityEvent{
        .event_id = "11111111-1111-4111-8111-111111111111",
        .event_type = audit::SecurityEventType::api_request_processed,
        .severity = audit::SecurityEventSeverity::info,
        .event_time_utc = "2026-03-13T10:00:00Z",
        .producer_role = audit::PeerRole::ubuntu_tpm,
        .actor_identity = make_identity(
            audit::PeerRole::ubuntu_tpm, "ubuntu-tpm-spb1-01", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"),
        .payload_metadata = {{"api_name", "identity"}, {"status", "ok"}},
        .result_summary = audit::ResultSummary{.result = "ok"},
    };
}

dcplayer::security_api::audit::SecurityEvent make_export_event() {
    namespace audit = dcplayer::security_api::audit;

    return audit::SecurityEvent{
        .event_id = "33333333-3333-4333-8333-333333333333",
        .event_type = audit::SecurityEventType::export_audit_exported,
        .severity = audit::SecurityEventSeverity::info,
        .event_time_utc = "2026-03-13T10:10:00Z",
        .producer_role = audit::PeerRole::ubuntu_tpm,
        .export_id_ref = "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa",
        .payload_metadata = {{"destination", "local_bundle"}, {"integrity_state", "checksum_pending"}},
        .result_summary = audit::ResultSummary{.result = "exported"},
    };
}

dcplayer::security_api::audit::SecurityEvent make_acl_event() {
    namespace audit = dcplayer::security_api::audit;

    return audit::SecurityEvent{
        .event_id = "22222222-2222-4222-8222-222222222222",
        .event_type = audit::SecurityEventType::acl_decision_recorded,
        .severity = audit::SecurityEventSeverity::warning,
        .event_time_utc = "2026-03-13T10:05:00Z",
        .producer_role = audit::PeerRole::ubuntu_tpm,
        .actor_identity = make_identity(
            audit::PeerRole::ubuntu_tpm, "ubuntu-tpm-spb1-01", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"),
        .payload_metadata = {{"acl_policy", "spb1-admin"}, {"effect", "deny"}},
        .decision_summary = audit::DecisionSummary{.decision = "deny"},
    };
}

dcplayer::security_api::audit::AuditExport make_export(std::vector<dcplayer::security_api::audit::SecurityEvent> events) {
    namespace audit = dcplayer::security_api::audit;

    return audit::AuditExport{
        .export_id = "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa",
        .export_time_utc = "2026-03-13T10:11:00Z",
        .events = std::move(events),
        .ordering = audit::OrderingRules{.ordered_by = "event_time_utc,event_id", .direction = "ascending"},
        .export_metadata =
            audit::ExportMetadata{.completeness = audit::AuditCompleteness::complete,
                                  .export_reason = "manual",
                                  .source_channel_id = "spb1.control.v1"},
        .integrity_metadata =
            audit::IntegrityMetadata{.placeholder_type = audit::IntegrityPlaceholderType::checksum_pending,
                                     .placeholder_ref = "sha256:pending"},
    };
}

void expect_parse_and_direct_diagnostics(const std::filesystem::path& fixture_root,
                                         std::string_view fixture_name,
                                         const dcplayer::security_api::audit::AuditExport& audit_export,
                                         const std::string& label) {
    namespace audit = dcplayer::security_api::audit;

    const auto expected_json = read_file(fixture_root / "invalid" / (std::string{fixture_name} + ".diagnostics.json"));
    const auto parsed =
        audit::parse_audit_export(read_file(fixture_root / "invalid" / (std::string{fixture_name} + ".audit_export.json")));

    expect(!parsed.ok(), label + " parse-path must fail");
    expect(!parsed.value.has_value(), label + " parse-path must not yield a value");
    expect_diagnostics_json(parsed.diagnostics, expected_json, label + " parse-path diagnostics mismatch");

    const auto validation = audit::validate_audit_export(audit_export);

    expect(!validation.ok(), label + " direct validate path must fail");
    expect(validation.validation_status == audit::ValidationStatus::error, label + " direct validation must stay in error");
    expect_diagnostics_json(validation.diagnostics, expected_json, label + " direct diagnostics mismatch");
}

}  // namespace

int main() {
    namespace audit = dcplayer::security_api::audit;
    namespace secure_channel = dcplayer::security_api::secure_channel;

    const std::filesystem::path fixture_root{AUDIT_FIXTURE_DIR};

    {
        const auto json = read_file(fixture_root / "valid" / "baseline.audit_export.json");
        const auto parsed = audit::parse_audit_export(json);

        expect(parsed.ok(), "Valid AuditExport fixture must parse");
        expect(parsed.value.has_value(), "Expected parsed AuditExport value");
        expect(parsed.diagnostics.empty(), "Valid AuditExport fixture must not emit diagnostics");
        expect(audit::to_json(*parsed.value) == json, "Unexpected canonical AuditExport JSON");

        const auto validation = audit::validate_audit_export(*parsed.value);
        expect(validation.ok(), "Valid AuditExport must pass validation");
        expect(validation.diagnostics.empty(), "Valid AuditExport must not emit validation diagnostics");
    }

    {
        const auto parsed =
            audit::parse_audit_export(read_file(fixture_root / "invalid" / "duplicate_event_id.audit_export.json"));

        expect(!parsed.ok(), "Duplicate event_id export fixture must fail");
        expect(!parsed.value.has_value(), "Duplicate event_id export fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "duplicate_event_id.diagnostics.json"),
               "Unexpected duplicate_event_id diagnostics");
    }

    {
        const auto parsed =
            audit::parse_audit_export(read_file(fixture_root / "invalid" / "ordering_mismatch.audit_export.json"));

        expect(!parsed.ok(), "Ordering mismatch export fixture must fail");
        expect(!parsed.value.has_value(), "Ordering mismatch export fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "ordering_mismatch.diagnostics.json"),
               "Unexpected export_ordering_mismatch diagnostics");
    }

    {
        const auto parsed =
            audit::parse_audit_export(read_file(fixture_root / "invalid" / "invalid_event_export_linkage.audit_export.json"));

        expect(!parsed.ok(), "Invalid event/export linkage fixture must fail");
        expect(!parsed.value.has_value(), "Invalid event/export linkage fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "invalid_event_export_linkage.diagnostics.json"),
               "Unexpected invalid_event_export_linkage diagnostics");
    }

    {
        const auto parsed =
            audit::parse_audit_export(read_file(fixture_root / "invalid" / "invalid_tamper_transition.audit_export.json"));

        expect(!parsed.ok(), "Invalid tamper transition fixture must fail");
        expect(!parsed.value.has_value(), "Invalid tamper transition fixture must not yield a value");
        expect(secure_channel::diagnostics_to_json(parsed.diagnostics) ==
                   read_file(fixture_root / "invalid" / "invalid_tamper_transition.diagnostics.json"),
               "Unexpected invalid_tamper_transition diagnostics");
    }

    {
        auto first_event = make_api_event();
        first_event.request_id = "99999999-9999-4999-8999-999999999999";

        auto second_event = first_event;
        second_event.event_time_utc = "2026-03-13T10:01:00Z";

        const auto validation = audit::validate_audit_export(make_export({first_event, second_event}));

        expect(!validation.ok(), "Direct validate path must not return ok on duplicate event_id");
        expect(validation.validation_status == audit::ValidationStatus::error,
               "Direct validate path must surface error validation_status");
        expect_diagnostics_json(validation.diagnostics,
                                "[\n"
                                "  {\n"
                                "    \"code\": \"audit.duplicate_event_id\",\n"
                                "    \"severity\": \"error\",\n"
                                "    \"path\": \"/events[2]/event_id\",\n"
                                "    \"message\": \"event_id must be unique within one export\"\n"
                                "  }\n"
                                "]\n",
                                "Unexpected direct duplicate_event_id diagnostics");
    }

    {
        auto event = make_api_event();
        event.request_id.reset();

        const auto validation = audit::validate_audit_export(make_export({event}));

        expect(!validation.ok(), "Direct validate path must reject api.request_processed without request_id");
        expect(validation.validation_status == audit::ValidationStatus::error,
               "Missing request_id must keep validation_status in error state");
        expect_diagnostics_json(validation.diagnostics,
                                "[\n"
                                "  {\n"
                                "    \"code\": \"audit.missing_required_field\",\n"
                                "    \"severity\": \"error\",\n"
                                "    \"path\": \"/events[1]/request_id\",\n"
                                "    \"message\": \"Missing required audit field\"\n"
                                "  }\n"
                                "]\n",
                                "Unexpected direct missing request_id diagnostics");
    }

    {
        auto event = make_export_event();
        event.export_id_ref.reset();

        const auto validation = audit::validate_audit_export(make_export({event}));

        expect(!validation.ok(), "Direct validate path must reject export.audit_exported without export_id_ref");
        expect_diagnostics_json(validation.diagnostics,
                                "[\n"
                                "  {\n"
                                "    \"code\": \"audit.missing_required_field\",\n"
                                "    \"severity\": \"error\",\n"
                                "    \"path\": \"/events[1]/export_id_ref\",\n"
                                "    \"message\": \"Missing required audit field\"\n"
                                "  }\n"
                                "]\n",
                                "Unexpected direct missing export_id_ref diagnostics");
    }

    {
        auto event = make_api_event();
        event.request_id = "99999999-9999-4999-8999-999999999999";
        event.event_time_utc = "2026-03-13 10:00:00";

        const auto validation = audit::validate_audit_export(make_export({event}));

        expect(!validation.ok(), "Direct validate path must reject malformed event_time_utc");
        expect_diagnostics_json(validation.diagnostics,
                                "[\n"
                                "  {\n"
                                "    \"code\": \"audit.invalid_event_time\",\n"
                                "    \"severity\": \"error\",\n"
                                "    \"path\": \"/events[1]/event_time_utc\",\n"
                                "    \"message\": \"Field must use UTC ISO-8601\"\n"
                                "  }\n"
                                "]\n",
                                "Unexpected direct invalid_event_time diagnostics");
    }

    {
        auto event = make_api_event();
        event.request_id = "99999999-9999-4999-8999-999999999999";
        event.event_type = static_cast<audit::SecurityEventType>(999);

        const auto validation = audit::validate_audit_export(make_export({event}));

        expect(!validation.ok(), "Direct validate path must reject invalid event_type enum");
        expect_diagnostics_json(validation.diagnostics,
                                "[\n"
                                "  {\n"
                                "    \"code\": \"audit.invalid_event_type\",\n"
                                "    \"severity\": \"error\",\n"
                                "    \"path\": \"/events[1]/event_type\",\n"
                                "    \"message\": \"Field has an unsupported enum value\"\n"
                                "  }\n"
                                "]\n",
                                "Unexpected direct invalid_event_type diagnostics");
    }

    {
        auto event = make_api_event();
        event.request_id = "99999999-9999-4999-8999-999999999999";
        event.severity = static_cast<audit::SecurityEventSeverity>(999);

        const auto validation = audit::validate_audit_export(make_export({event}));

        expect(!validation.ok(), "Direct validate path must reject invalid severity enum");
        expect_diagnostics_json(validation.diagnostics,
                                "[\n"
                                "  {\n"
                                "    \"code\": \"audit.invalid_severity\",\n"
                                "    \"severity\": \"error\",\n"
                                "    \"path\": \"/events[1]/severity\",\n"
                                "    \"message\": \"Field has an unsupported enum value\"\n"
                                "  }\n"
                                "]\n",
                                "Unexpected direct invalid_severity diagnostics");
    }

    {
        auto event = make_api_event();
        event.request_id = "99999999-9999-4999-8999-999999999999";
        event.result_summary = audit::ResultSummary{.result = ""};

        expect_parse_and_direct_diagnostics(
            fixture_root, "invalid_nested_result_summary", make_export({event}), "Invalid nested result_summary regression");
    }

    {
        auto event = make_api_event();
        event.request_id = "99999999-9999-4999-8999-999999999999";
        event.actor_identity = make_identity(
            audit::PeerRole::ubuntu_tpm, "ubuntu-tpm-spb1-01", "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");

        expect_parse_and_direct_diagnostics(
            fixture_root, "invalid_nested_actor_identity", make_export({event}), "Invalid nested actor_identity regression");
    }

    {
        auto event = make_acl_event();
        event.decision_summary = audit::DecisionSummary{.decision = ""};

        expect_parse_and_direct_diagnostics(
            fixture_root, "invalid_nested_decision_summary", make_export({event}), "Invalid nested decision_summary regression");
    }

    return 0;
}
