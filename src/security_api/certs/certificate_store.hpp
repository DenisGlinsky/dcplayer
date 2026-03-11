#pragma once

#include <compare>
#include <optional>
#include <string>
#include <vector>

namespace dcplayer::security::certs {

enum class DiagnosticSeverity {
    error,
    warning,
};

enum class ValidationStatus {
    ok,
    warning,
    error,
};

enum class FingerprintAlgorithm {
    sha256,
};

enum class CertificateRole {
    root,
    intermediate,
    server,
    client,
    signer,
    unknown,
};

struct Diagnostic {
    std::string code;
    DiagnosticSeverity severity;
    std::string path;
    std::string message;

    [[nodiscard]] auto operator<=>(const Diagnostic&) const = default;
};

struct CertificateRecord {
    std::string fingerprint;
    std::string subject_dn;
    std::string issuer_dn;
    CertificateRole role{CertificateRole::unknown};
    CertificateRole issuer_role_hint{CertificateRole::unknown};
    std::string not_before_utc;
    std::string not_after_utc;
    std::string serial_number;

    [[nodiscard]] auto operator<=>(const CertificateRecord&) const = default;
};

struct CertificateImportRecord {
    CertificateRecord certificate;
    std::string private_key_material;

    [[nodiscard]] auto operator<=>(const CertificateImportRecord&) const = default;
};

struct CertificateStoreMetadata {
    std::string store_id;
    std::string store_version;
    std::string updated_at_utc;

    [[nodiscard]] auto operator<=>(const CertificateStoreMetadata&) const = default;
};

struct CertificateStore {
    std::string store_id;
    std::string store_version;
    FingerprintAlgorithm fingerprint_algorithm{FingerprintAlgorithm::sha256};
    std::vector<CertificateRecord> roots;
    std::vector<CertificateRecord> intermediates;
    std::vector<CertificateRecord> device_certs;
    std::vector<std::string> revocation_sources;
    std::string updated_at_utc;

    [[nodiscard]] auto operator<=>(const CertificateStore&) const = default;
};

struct StoreBuildResult {
    std::optional<CertificateStore> certificate_store;
    std::vector<Diagnostic> diagnostics;
    ValidationStatus validation_status{ValidationStatus::ok};

    [[nodiscard]] bool ok() const noexcept {
        return validation_status != ValidationStatus::error;
    }
};

class CertificateStoreBuilder {
  public:
    explicit CertificateStoreBuilder(CertificateStoreMetadata metadata);

    void set_revocation_sources(std::vector<std::string> revocation_sources);
    void import_trust_anchor(CertificateImportRecord record);
    void import_intermediate(CertificateImportRecord record);
    void import_device_certificate(CertificateImportRecord record);

    [[nodiscard]] StoreBuildResult build() const;

  private:
    CertificateStoreMetadata metadata_;
    std::vector<CertificateImportRecord> roots_;
    std::vector<CertificateImportRecord> intermediates_;
    std::vector<CertificateImportRecord> device_certs_;
    std::vector<std::string> revocation_sources_;
};

[[nodiscard]] std::string to_json(const CertificateStore& certificate_store);
[[nodiscard]] std::string diagnostics_to_json(const std::vector<Diagnostic>& diagnostics);
[[nodiscard]] const char* to_string(DiagnosticSeverity severity) noexcept;
[[nodiscard]] const char* to_string(ValidationStatus status) noexcept;
[[nodiscard]] const char* to_string(FingerprintAlgorithm algorithm) noexcept;
[[nodiscard]] const char* to_string(CertificateRole role) noexcept;

}  // namespace dcplayer::security::certs
