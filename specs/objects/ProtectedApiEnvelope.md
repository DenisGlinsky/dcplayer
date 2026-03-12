# ProtectedApiEnvelope

## Назначение

Единый envelope для защищённых вызовов secure control plane в ветке `T03b`.

Контракт покрывает:
- request envelope;
- response envelope;
- auth-context summary;
- payload metadata/body contract;
- deterministic diagnostics.

Контракт не включает transport framing, streaming, chunking и сетевые ошибки.

## Request envelope

### Канонические поля

- `request_id` — canonical UUID строка
- `api_name` — enum `sign | unwrap | decrypt | health | identity`
- `caller_role` — enum `pi_zymkey | ubuntu_tpm`
- `caller_identity` — `PeerIdentity`
- `auth_context` — `AuthContextSummary`
- `payload` — `PayloadContract`

### `AuthContextSummary`

- `channel_id` — строка
- `mutual_tls` — boolean, для baseline всегда `true`
- `local_role` — enum роли принимающей стороны
- `peer_trust` — summary результата `TrustChain`
- `checked_sources` — массив строк

### `peer_trust`

- `subject_fingerprint`
- `issuer_chain`
- `validation_time_utc`
- `revocation_status`
- `decision`
- `decision_reason`
- `checked_sources`

### `PayloadContract`

- `payload_type` — строка (`sign_request`, `sign_response`, `error_response` и т.п.)
- `schema_ref` — строка
- `body` — объект string -> string

## Response envelope

### Канонические поля

- `request_id` — canonical UUID строка
- `status` — enum `ok | denied | error`
- `diagnostics` — массив `Diagnostic`
- `payload` — `PayloadContract`

## Инварианты

- `request_id` детерминированно нормализуется в lower-case canonical UUID.
- `caller_identity.role == caller_role`.
- `auth_context.mutual_tls == true`.
- `auth_context.peer_trust.subject_fingerprint == caller_identity.certificate_fingerprint`.
- `payload.payload_type` должен соответствовать `api_name` и направлению envelope-а.
- `response.request_id == request.request_id`.
- `status=ok` не несёт diagnostics.
- `status=denied | error` несёт хотя бы один diagnostic.
- дубликаты в `caller_identity.san_*` и в любых `checked_sources` внутри envelope-а детерминированно отклоняются.

## Request/response payload baseline

- `sign` -> `sign_request` / `sign_response`
- `unwrap` -> `unwrap_request` / `unwrap_response`
- `decrypt` -> `decrypt_request` / `decrypt_response`
- `health` -> `health_request` / `health_response`
- `identity` -> `identity_request` / `identity_response`
- error path -> `error_response`

В `T03b` payload остаётся contract-level placeholder и не обязан нести реальную крипто-операцию.

## Связи с другими объектами

- Ограничивается `SecureChannelContract`.
- Используется `SecurityModuleContract`.
- `auth_context.peer_trust` должен быть совместим с `TrustChain`.

## Каноническая сериализация

- JSON `snake_case`
- deterministic field order
- `body` сериализуется как отсортированный string-map
- diagnostics сериализуются в формате `code + severity + path + message`

## Минимальная диагностика

- `secure_channel.invalid_request_id`
- `secure_channel.role_mismatch`
- `secure_channel.invalid_channel_binding`
- `secure_channel.missing_trust_binding`
- `secure_channel.invalid_payload_contract`
- `secure_channel.request_id_mismatch`
- `secure_channel.unexpected_response_diagnostics`
- `secure_channel.missing_response_diagnostics`
- `secure_channel.duplicate_checked_source`
- `secure_channel.duplicate_san_entry`

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- невалидный enum/type;
- request/response нарушают envelope invariants;
- trust summary не привязан к caller identity;
- сериализация недетерминирована.

## Замечания по эволюции

- Новые API добавляются через новые `api_name` и payload types.
- Ломающие изменения envelope semantics требуют синхронного обновления:
  - `SecureChannelContract.md`
  - `SecurityModuleContract.md`
  - `docs/IMPLEMENT.md`
  - `docs/STATUS.md`
