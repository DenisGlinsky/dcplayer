#pragma once

#include "secure_channel.hpp"
#include "secure_time.hpp"

#include <map>
#include <string>
#include <vector>

namespace dcplayer::security_api::host_spb1_contract {

[[nodiscard]] inline secure_channel::SecureChannelContract make_baseline_secure_channel_contract() {
    using namespace secure_channel;

    return SecureChannelContract{
        .channel_id = "spb1.control.v1",
        .server_role = PeerRole::pi_zymkey,
        .client_role = PeerRole::ubuntu_tpm,
        .tls_profile = "mtls_device_control_v1",
        .trust_requirements =
            TrustRequirements{
                .certificate_store_ref = "cert_store.control_plane.v1",
                .required_decision = TrustDecision::trusted,
                .required_decision_reason = TrustDecisionReason::ok,
                .accepted_revocation_statuses = {RevocationStatus::good},
                .required_checked_sources = {"crl_cache", "intermediates", "roots"},
            },
        .server_identity =
            PeerIdentity{
                .role = PeerRole::pi_zymkey,
                .device_id = "pi-zymkey-spb1-01",
                .certificate_fingerprint = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                .subject_dn = "CN=dcplayer-spb1-server,O=dcplayer",
                .san_dns_names = {"spb1.local"},
                .san_uri_names = {"urn:dcplayer:spb1:pi_zymkey:01"},
            },
        .client_identity =
            PeerIdentity{
                .role = PeerRole::ubuntu_tpm,
                .device_id = "ubuntu-tpm-spb1-01",
                .certificate_fingerprint = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
                .subject_dn = "CN=dcplayer-host-client,O=dcplayer",
                .san_dns_names = {"media-host.local"},
                .san_uri_names = {"urn:dcplayer:spb1:ubuntu_tpm:01"},
            },
        .acl =
            {
                AclRule{
                    .caller_role = PeerRole::ubuntu_tpm,
                    .allowed_api_names = {ApiName::sign, ApiName::unwrap, ApiName::decrypt, ApiName::health, ApiName::identity},
                },
            },
    };
}

[[nodiscard]] inline secure_channel::SecurityModuleContract make_baseline_security_module_contract() {
    using namespace secure_channel;

    return SecurityModuleContract{
        .module_id = "spb1.pi_zymkey.module.v1",
        .module_role = PeerRole::pi_zymkey,
        .secure_channel_id = "spb1.control.v1",
        .request_envelope_ref = "protected_request.v1",
        .response_envelope_ref = "protected_response.v1",
        .supported_api_names = {ApiName::sign, ApiName::unwrap, ApiName::decrypt, ApiName::health, ApiName::identity},
        .server_identity =
            PeerIdentity{
                .role = PeerRole::pi_zymkey,
                .device_id = "pi-zymkey-spb1-01",
                .certificate_fingerprint = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                .subject_dn = "CN=dcplayer-spb1-server,O=dcplayer",
                .san_dns_names = {"spb1.local"},
                .san_uri_names = {"urn:dcplayer:spb1:pi_zymkey:01"},
            },
    };
}

[[nodiscard]] inline secure_time::SecureClockPolicy make_baseline_secure_clock_policy() {
    return secure_time::SecureClockPolicy{
        .clock_id = "spb1.secure_clock.v1",
        .max_allowed_skew_seconds = 30U,
        .freshness_window_seconds = 300U,
        .monotonic_required = true,
        .allowed_time_sources =
            {
                secure_time::TimeSource::rtc_secure,
                secure_time::TimeSource::imported_secure_time,
            },
        .fail_closed_on_untrusted_time = true,
    };
}

[[nodiscard]] inline std::vector<secure_channel::ApiName> secure_time_gated_api_names() {
    return {
        secure_channel::ApiName::sign,
        secure_channel::ApiName::unwrap,
        secure_channel::ApiName::decrypt,
    };
}

[[nodiscard]] inline secure_channel::PayloadContract make_sign_payload(std::string manifest_hash,
                                                                       std::string metadata_summary,
                                                                       std::string key_slot = {}) {
    std::map<std::string, std::string> body{
        {"manifest_hash", std::move(manifest_hash)},
        {"metadata_summary", std::move(metadata_summary)},
    };
    if (!key_slot.empty()) {
        body.insert_or_assign("key_slot", std::move(key_slot));
    }

    return secure_channel::PayloadContract{
        .payload_type = "sign_request",
        .schema_ref = "spb1.sign.request.v1",
        .body = std::move(body),
    };
}

}  // namespace dcplayer::security_api::host_spb1_contract
