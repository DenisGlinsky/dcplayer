#include "certificate_store.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace dcplayer::security::certs {
namespace {

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

[[nodiscard]] bool is_valid_revocation_source(std::string_view source) {
    if (source.empty()) {
        return false;
    }

    if (std::any_of(source.begin(), source.end(), [](unsigned char value) { return std::isspace(value) != 0; })) {
        return false;
    }

    return source.rfind("file://", 0U) == 0U || source.rfind("https://", 0U) == 0U || source.rfind("crl://", 0U) == 0U ||
           source.rfind("ocsp://", 0U) == 0U || source.rfind("urn:", 0U) == 0U;
}

[[nodiscard]] bool role_allowed_in_roots(CertificateRole role) noexcept {
    return role == CertificateRole::root;
}

[[nodiscard]] bool role_allowed_in_intermediates(CertificateRole role) noexcept {
    return role == CertificateRole::intermediate;
}

[[nodiscard]] bool role_allowed_in_device_certs(CertificateRole role) noexcept {
    return role == CertificateRole::server || role == CertificateRole::client || role == CertificateRole::signer;
}

[[nodiscard]] bool role_allowed_in_issuer_role_hint(CertificateRole role) noexcept {
    return role == CertificateRole::root || role == CertificateRole::intermediate || role == CertificateRole::unknown;
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

void append_record_json(std::ostringstream& output, const CertificateRecord& record, std::size_t indent_size) {
    const std::string indent(indent_size, ' ');
    const std::string nested_indent(indent_size + 2U, ' ');

    output << indent << "{\n";
    output << nested_indent << "\"fingerprint\": ";
    append_json_string(output, record.fingerprint);
    output << ",\n";
    output << nested_indent << "\"subject_dn\": ";
    append_json_string(output, record.subject_dn);
    output << ",\n";
    output << nested_indent << "\"issuer_dn\": ";
    append_json_string(output, record.issuer_dn);
    output << ",\n";
    output << nested_indent << "\"role\": ";
    append_json_string(output, to_string(record.role));
    output << ",\n";
    if (record.issuer_role_hint != CertificateRole::unknown) {
        output << nested_indent << "\"issuer_role_hint\": ";
        append_json_string(output, to_string(record.issuer_role_hint));
        output << ",\n";
    }
    output << nested_indent << "\"not_before_utc\": ";
    append_json_string(output, record.not_before_utc);
    output << ",\n";
    output << nested_indent << "\"not_after_utc\": ";
    append_json_string(output, record.not_after_utc);
    output << ",\n";
    output << nested_indent << "\"serial_number\": ";
    append_json_string(output, record.serial_number);
    output << '\n';
    output << indent << '}';
}

void append_record_array_json(std::ostringstream& output,
                              std::string_view field_name,
                              const std::vector<CertificateRecord>& records,
                              bool trailing_comma) {
    output << "  \"";
    output << field_name;
    output << "\": [\n";
    for (std::size_t index = 0U; index < records.size(); ++index) {
        append_record_json(output, records[index], 4U);
        output << (index + 1U == records.size() ? '\n' : ',');
        if (index + 1U != records.size()) {
            output << '\n';
        }
    }
    output << "  ]";
    output << (trailing_comma ? ",\n" : "\n");
}

void validate_collection(const std::vector<CertificateImportRecord>& imports,
                         std::string_view collection_name,
                         bool (*role_validator)(CertificateRole),
                         std::vector<CertificateRecord>& target,
                         std::vector<Diagnostic>& diagnostics,
                         std::unordered_map<std::string, std::string>& fingerprints) {
    for (std::size_t index = 0U; index < imports.size(); ++index) {
        const auto base_path = "/" + std::string{collection_name} + "[" + std::to_string(index + 1U) + "]";
        CertificateRecord normalized = imports[index].certificate;

        if (!imports[index].private_key_material.empty()) {
            add_diagnostic(diagnostics,
                           "cert_store.private_key_material_forbidden",
                           DiagnosticSeverity::error,
                           base_path + "/private_key_material",
                           "Private key material is forbidden in CertificateStore");
        }

        if (const auto normalized_fingerprint = normalize_fingerprint(normalized.fingerprint); normalized_fingerprint.has_value()) {
            normalized.fingerprint = *normalized_fingerprint;
            const auto [iterator, inserted] = fingerprints.emplace(normalized.fingerprint, base_path + "/fingerprint");
            if (!inserted) {
                add_diagnostic(diagnostics,
                               "cert_store.duplicate_fingerprint",
                               DiagnosticSeverity::error,
                               base_path + "/fingerprint",
                               "Fingerprint already exists in CertificateStore");
            }
        } else {
            add_diagnostic(diagnostics,
                           "cert_store.invalid_fingerprint",
                           DiagnosticSeverity::error,
                           base_path + "/fingerprint",
                           "Fingerprint must be a 64-character SHA-256 hex value");
        }

        if (!role_validator(normalized.role)) {
            add_diagnostic(diagnostics,
                           "cert_store.invalid_role",
                           DiagnosticSeverity::error,
                           base_path + "/role",
                           "Certificate role is not allowed in this store collection");
        }

        if (!role_allowed_in_issuer_role_hint(normalized.issuer_role_hint)) {
            add_diagnostic(diagnostics,
                           "cert_store.invalid_role",
                           DiagnosticSeverity::error,
                           base_path + "/issuer_role_hint",
                           "issuer_role_hint is not allowed in CertificateStore");
        }

        if (!is_valid_utc_timestamp(normalized.not_before_utc) || !is_valid_utc_timestamp(normalized.not_after_utc) ||
            normalized.not_before_utc > normalized.not_after_utc) {
            add_diagnostic(diagnostics,
                           "cert_store.invalid_time_window",
                           DiagnosticSeverity::error,
                           base_path + "/not_after_utc",
                           "Certificate time window is invalid");
        }

        if (normalized.role == CertificateRole::root && normalized.subject_dn != normalized.issuer_dn) {
            add_diagnostic(diagnostics,
                           "cert_store.missing_trust_anchor",
                           DiagnosticSeverity::error,
                           base_path + "/issuer_dn",
                           "Root certificate must be self-issued to act as a trust anchor");
        }

        target.push_back(std::move(normalized));
    }
}

}  // namespace

CertificateStoreBuilder::CertificateStoreBuilder(CertificateStoreMetadata metadata) : metadata_(std::move(metadata)) {}

void CertificateStoreBuilder::set_revocation_sources(std::vector<std::string> revocation_sources) {
    revocation_sources_ = std::move(revocation_sources);
}

void CertificateStoreBuilder::import_trust_anchor(CertificateImportRecord record) {
    roots_.push_back(std::move(record));
}

void CertificateStoreBuilder::import_intermediate(CertificateImportRecord record) {
    intermediates_.push_back(std::move(record));
}

void CertificateStoreBuilder::import_device_certificate(CertificateImportRecord record) {
    device_certs_.push_back(std::move(record));
}

StoreBuildResult CertificateStoreBuilder::build() const {
    StoreBuildResult result;
    CertificateStore certificate_store{
        .store_id = metadata_.store_id,
        .store_version = metadata_.store_version,
        .fingerprint_algorithm = FingerprintAlgorithm::sha256,
        .updated_at_utc = metadata_.updated_at_utc,
    };

    if (!is_valid_utc_timestamp(metadata_.updated_at_utc)) {
        add_diagnostic(result.diagnostics,
                       "cert_store.invalid_time_window",
                       DiagnosticSeverity::error,
                       "/updated_at_utc",
                       "CertificateStore updated_at_utc must be a UTC timestamp");
    }

    std::unordered_map<std::string, std::string> fingerprints;
    validate_collection(roots_, "roots", role_allowed_in_roots, certificate_store.roots, result.diagnostics, fingerprints);
    validate_collection(intermediates_,
                        "intermediates",
                        role_allowed_in_intermediates,
                        certificate_store.intermediates,
                        result.diagnostics,
                        fingerprints);
    validate_collection(device_certs_,
                        "device_certs",
                        role_allowed_in_device_certs,
                        certificate_store.device_certs,
                        result.diagnostics,
                        fingerprints);

    certificate_store.revocation_sources = revocation_sources_;
    for (std::size_t index = 0U; index < certificate_store.revocation_sources.size(); ++index) {
        if (!is_valid_revocation_source(certificate_store.revocation_sources[index])) {
            add_diagnostic(result.diagnostics,
                           "cert_store.invalid_revocation_source",
                           DiagnosticSeverity::error,
                           "/revocation_sources[" + std::to_string(index + 1U) + "]",
                           "Revocation source URI is invalid");
        }
    }

    std::sort(certificate_store.revocation_sources.begin(), certificate_store.revocation_sources.end());
    certificate_store.revocation_sources.erase(
        std::unique(certificate_store.revocation_sources.begin(), certificate_store.revocation_sources.end()),
        certificate_store.revocation_sources.end());

    sort_diagnostics(result.diagnostics);
    result.validation_status = validation_status_for(result.diagnostics);
    if (result.validation_status != ValidationStatus::error) {
        result.certificate_store = std::move(certificate_store);
    }
    return result;
}

std::string to_json(const CertificateStore& certificate_store) {
    std::ostringstream output;
    output << "{\n";
    output << "  \"store_id\": ";
    append_json_string(output, certificate_store.store_id);
    output << ",\n";
    output << "  \"store_version\": ";
    append_json_string(output, certificate_store.store_version);
    output << ",\n";
    output << "  \"fingerprint_algorithm\": ";
    append_json_string(output, to_string(certificate_store.fingerprint_algorithm));
    output << ",\n";
    append_record_array_json(output, "roots", certificate_store.roots, true);
    append_record_array_json(output, "intermediates", certificate_store.intermediates, true);
    append_record_array_json(output, "device_certs", certificate_store.device_certs, true);
    output << "  \"revocation_sources\": [\n";
    for (std::size_t index = 0U; index < certificate_store.revocation_sources.size(); ++index) {
        output << "    ";
        append_json_string(output, certificate_store.revocation_sources[index]);
        output << (index + 1U == certificate_store.revocation_sources.size() ? '\n' : ',');
        if (index + 1U != certificate_store.revocation_sources.size()) {
            output << '\n';
        }
    }
    output << "  ],\n";
    output << "  \"updated_at_utc\": ";
    append_json_string(output, certificate_store.updated_at_utc);
    output << '\n';
    output << "}\n";
    return output.str();
}

std::string diagnostics_to_json(const std::vector<Diagnostic>& diagnostics) {
    std::ostringstream output;
    output << "[\n";
    for (std::size_t index = 0U; index < diagnostics.size(); ++index) {
        output << "  {\n";
        output << "    \"code\": ";
        append_json_string(output, diagnostics[index].code);
        output << ",\n";
        output << "    \"severity\": ";
        append_json_string(output, to_string(diagnostics[index].severity));
        output << ",\n";
        output << "    \"path\": ";
        append_json_string(output, diagnostics[index].path);
        output << ",\n";
        output << "    \"message\": ";
        append_json_string(output, diagnostics[index].message);
        output << '\n';
        output << "  }";
        output << (index + 1U == diagnostics.size() ? '\n' : ',');
        if (index + 1U != diagnostics.size()) {
            output << '\n';
        }
    }
    output << "]\n";
    return output.str();
}

const char* to_string(DiagnosticSeverity severity) noexcept {
    switch (severity) {
        case DiagnosticSeverity::error:
            return "error";
        case DiagnosticSeverity::warning:
            return "warning";
    }
    return "unknown";
}

const char* to_string(ValidationStatus status) noexcept {
    switch (status) {
        case ValidationStatus::ok:
            return "ok";
        case ValidationStatus::warning:
            return "warning";
        case ValidationStatus::error:
            return "error";
    }
    return "unknown";
}

const char* to_string(FingerprintAlgorithm algorithm) noexcept {
    switch (algorithm) {
        case FingerprintAlgorithm::sha256:
            return "sha256";
    }
    return "unknown";
}

const char* to_string(CertificateRole role) noexcept {
    switch (role) {
        case CertificateRole::root:
            return "root";
        case CertificateRole::intermediate:
            return "intermediate";
        case CertificateRole::server:
            return "server";
        case CertificateRole::client:
            return "client";
        case CertificateRole::signer:
            return "signer";
        case CertificateRole::unknown:
            return "unknown";
    }
    return "unknown";
}

}  // namespace dcplayer::security::certs
