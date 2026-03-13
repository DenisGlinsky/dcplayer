# SecureChannelContract

## Назначение

Формальный machine-readable контракт secure control plane между
`ubuntu_tpm` (client) и `pi_zymkey` (server) для веток `T03b/T03c`.

Контракт описывает только:
- object model mutual-auth канала;
- identity binding;
- trust dependency на результат `T03a`;
- ACL baseline;
- deterministic diagnostics.

Контракт **не** описывает живой transport, socket/session lifetime, retries,
TPM/ZymKey device integration и реальный TLS handshake.

## Канонические поля

- `channel_id` — строка
- `server_role` — enum `pi_zymkey`
- `client_role` — enum `ubuntu_tpm`
- `tls_profile` — строка
- `trust_requirements` — объект
- `server_identity` — объект `PeerIdentity`
- `client_identity` — объект `PeerIdentity`
- `acl` — массив `AclRule`

### `PeerIdentity`

- `role` — enum `pi_zymkey | ubuntu_tpm`
- `device_id` — строка
- `certificate_fingerprint` — lower-case hex строка
- `subject_dn` — строка
- `san_dns_names` — массив строк
- `san_uri_names` — массив строк

### `trust_requirements`

- `certificate_store_ref` — строка, ссылка на baseline trust store из `T03a`
- `required_decision` — enum `trusted | rejected | warning_only`
- `required_decision_reason` — enum из `TrustChain`
- `accepted_revocation_statuses` — массив enum из `TrustChain.revocation_status`
- `required_checked_sources` — массив строк

### `AclRule`

- `caller_role` — enum `pi_zymkey | ubuntu_tpm`
- `allowed_api_names` — массив enum `sign | unwrap | decrypt | health | identity`

В baseline `T03c` допустим только request-side caller:
- `caller_role == ubuntu_tpm`;
- `pi_zymkey` не может выступать caller-ом protected request envelope-а;
- каждая роль может присутствовать в ACL не более одного раза.

## Инварианты

- `server_role` фиксирован как `pi_zymkey`.
- `client_role` фиксирован как `ubuntu_tpm`.
- `server_identity.role == server_role`.
- `client_identity.role == client_role`.
- `trust_requirements.certificate_store_ref` обязателен и ссылается на PKI baseline `T03a`.
- `required_checked_sources` не пустой и задаёт dependency list для trust decision.
- `accepted_revocation_statuses` и `allowed_api_names` не допускают повторов.
- `acl` содержит как минимум одно правило для `client_role`.
- baseline `T03c` не допускает ACL-правил для `server_role`; разрешён только client-side caller `ubuntu_tpm`.
- дублирующие ACL-правила для одного `caller_role` детерминированно отклоняются.
- `allowed_api_names` сериализуются детерминированно.
- дубликаты в `accepted_revocation_statuses`, `allowed_api_names`, `required_checked_sources`, `peer_trust.checked_sources` и SAN-списках не нормализуются silently, а детерминированно отклоняются.

## Authority semantics

- peer certificate chain считается входом из `TrustChain`, а не скрытой реализацией в transport code;
- решение допуска peer-а зависит от:
  - `peer_trust.decision`;
  - `peer_trust.decision_reason`;
  - `peer_trust.revocation_status`;
  - `peer_trust.checked_sources`;
  - binding `peer_trust.subject_fingerprint -> caller_identity.certificate_fingerprint`;
- для всех protected APIs обязателен один и тот же baseline trust context:
  - `auth_context.channel_id == channel_id`;
  - `auth_context.mutual_tls == true`;
  - `auth_context.local_role == server_role`;
  - `peer_trust` удовлетворяет `trust_requirements`;
- role mismatch и ACL deny диагностируются на model/envelope уровне, до реального transport integration.

## Baseline APIs

Контракт `T03c` фиксирует baseline API surface:
- `sign`
- `unwrap`
- `decrypt`
- `health`
- `identity`

ACL matrix baseline:
- `ubuntu_tpm -> sign`
- `ubuntu_tpm -> unwrap`
- `ubuntu_tpm -> decrypt`
- `ubuntu_tpm -> health`
- `ubuntu_tpm -> identity`

Запрещено:
- `pi_zymkey -> *` как caller для request envelope-а;
- любой `caller_role -> api_name`, отсутствующий в `acl`;
- request без trust-binding, даже для `health` и `identity`.

Для `unwrap` и `decrypt` в этой ветке допускается только placeholder-level API body contract;
generic string-map без family validation считается нарушением контракта.

## Связи с другими объектами

- Потребляет `CertificateStore` и `TrustChain` как source of trust decision semantics.
- Ограничивает `ProtectedApiEnvelope`.
- Используется `SecurityModuleContract`.

## Каноническая сериализация

- JSON `snake_case`
- все enum сериализуются строками
- массивы policy/ACL/identity sources упорядочены детерминированно
- fingerprint-ы сериализуются только как lower-case hex
- paths в diagnostics имеют JSON-pointer-like форму (`/field/subfield`, `[...]` для массива)

## Минимальная диагностика

- `secure_channel.invalid_server_role`
- `secure_channel.invalid_client_role`
- `secure_channel.role_mismatch`
- `secure_channel.invalid_peer_identity`
- `secure_channel.invalid_channel_binding`
- `secure_channel.missing_trust_binding`
- `secure_channel.peer_not_trusted`
- `secure_channel.unauthorized_api`
- `secure_channel.invalid_payload_contract`
- `secure_channel.missing_required_field`
- `secure_channel.unexpected_field`
- `secure_channel.invalid_request_body`
- `secure_channel.invalid_response_body`
- `secure_channel.request_response_api_mismatch`
- `secure_channel.invalid_status_body_combination`
- `secure_channel.duplicate_revocation_status`
- `secure_channel.duplicate_allowed_api_name`
- `secure_channel.duplicate_checked_source`
- `secure_channel.duplicate_san_entry`

Одинаковый дефект обязан выдавать одинаковый diagnostic code вне зависимости от платформы.

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- нарушен role/identity invariant;
- нарушен trust binding к `TrustChain`;
- `caller_role -> api_name` запрещён ACL;
- сериализация недетерминирована.

## Замечания по эволюции

- Transport/runtime semantics добавляются только отдельными ветками поверх этого контракта.
- Любое изменение peer roles, trust dependency или ACL baseline требует обновления:
  - `ProtectedApiEnvelope.md`
  - `SecurityModuleContract.md`
  - `docs/IMPLEMENT.md`
  - `docs/STATUS.md`
