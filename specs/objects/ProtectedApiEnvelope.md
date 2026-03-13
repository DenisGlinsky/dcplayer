# ProtectedApiEnvelope

## Назначение

Единый envelope для защищённых вызовов secure control plane в ветках `T03b/T03c`.

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
- `body` — объект `string -> string`, но только после API-family validation

В `T03c` `body` больше не считается произвольным string-map:
- request/response family определяется по `payload_type + schema_ref`;
- для каждого baseline API фиксируются required/optional/forbidden fields;
- unexpected поля детерминированно отклоняются;
- одинаковый body defect обязан давать одинаковый diagnostic code.

## Response envelope

### Канонические поля

- `request_id` — canonical UUID строка
- `status` — enum `ok | denied | error`
- `diagnostics` — массив `Diagnostic`
- `payload` — `PayloadContract`

Parse-stage для response envelope проверяет только:
- наличие канонических полей;
- типы JSON-значений;
- canonical `request_id` format;
- `payload_type` как response family;
- `schema_ref` matching для выбранной family;
- body shape / required / optional / forbidden fields для самой family.

Semantic validation stage поверх уже распарсенного response проверяет:
- `status/body` compatibility;
- `request_id` consistency относительно request;
- request/response family matching относительно `request.api_name`;
- diagnostics presence rules для `status`.

## Инварианты

- `request_id` детерминированно нормализуется в lower-case canonical UUID.
- `caller_identity.role == caller_role`.
- `auth_context.mutual_tls == true`.
- `auth_context.peer_trust.subject_fingerprint == caller_identity.certificate_fingerprint`.
- `payload.payload_type` и `payload.schema_ref` должны соответствовать `api_name` и направлению envelope-а.
- `response.request_id == request.request_id`.
- `response.status=ok` допускает только API-specific `*_response`.
- `response.status=denied | error` допускает только `error_response`.
- `response payload family` обязан соответствовать `request.api_name`, даже если `response` не дублирует `api_name` отдельным полем.
- syntactically valid response envelope не отклоняется на parse stage только из-за `status/body` mismatch; этот reject относится к semantic validation.
- precedence rule: если `status/body` pair уже нарушает envelope invariant, этот первичный дефект диагностируется через
  `secure_channel.invalid_status_body_combination`, а поверх него не добавляется вторичный
  `secure_channel.request_response_api_mismatch` для того же ответа.
- `status=ok` не несёт diagnostics.
- `status=denied | error` несёт хотя бы один diagnostic.
- дубликаты в `caller_identity.san_*` и в любых `checked_sources` внутри envelope-а детерминированно отклоняются.

## Request/response payload baseline

### Request families

- `sign` -> `payload_type=sign_request`, `schema_ref=spb1.sign.request.v1`
  - required: `manifest_hash`, `metadata_summary`
  - optional: `key_slot`
- `unwrap` -> `payload_type=unwrap_request`, `schema_ref=spb1.unwrap.request.v1`
  - required: `wrapped_key_ref`, `wrapping_key_slot`
  - optional: `unwrap_context`
- `decrypt` -> `payload_type=decrypt_request`, `schema_ref=spb1.decrypt.request.v1`
  - required: `ciphertext_ref`, `context_summary`
  - optional: `output_handle`
  - semantics: placeholder-level only; не означает runtime decrypt execution
- `health` -> `payload_type=health_request`, `schema_ref=spb1.health.request.v1`
  - required: none
  - optional: none
  - body обязан быть пустым объектом
- `identity` -> `payload_type=identity_request`, `schema_ref=spb1.identity.request.v1`
  - required: none
  - optional: none
  - body обязан быть пустым объектом

### Successful response families

- `sign` -> `payload_type=sign_response`, `schema_ref=spb1.sign.response.v1`
  - required: `signature`, `pubkey`
  - optional: `key_fingerprint`
- `unwrap` -> `payload_type=unwrap_response`, `schema_ref=spb1.unwrap.response.v1`
  - required: `key_handle`
  - optional: `unwrap_receipt`
- `decrypt` -> `payload_type=decrypt_response`, `schema_ref=spb1.decrypt.response.v1`
  - required: `result_handle`
  - optional: `operation_note`
  - semantics: placeholder-level only; не означает публикацию plaintext essence
- `health` -> `payload_type=health_response`, `schema_ref=spb1.health.response.v1`
  - required: `module_status`
  - optional: `status_summary`
- `identity` -> `payload_type=identity_response`, `schema_ref=spb1.identity.response.v1`
  - required: `module_id`, `module_role`, `certificate_fingerprint`
  - optional: `subject_dn`

### Error family

- `status=denied | error` -> `payload_type=error_response`, `schema_ref=spb1.error.response.v1`
  - required: `error_code`, `error_summary`
  - optional: none

## Body normalization rules

- `body` сериализуется как отсортированный string-map.
- `manifest_hash`, `key_fingerprint`, `certificate_fingerprint`, `error_code`, `module_role`, `module_status`
  нормализуются в lower-case.
- `module_role` в `identity_response` обязан совпадать с baseline module role `pi_zymkey`.
- `module_status` в `health_response` ограничен baseline enum `ok | degraded | maintenance`.
- empty string в required/optional field не считается валидным значением.

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
- `secure_channel.missing_required_field`
- `secure_channel.unexpected_field`
- `secure_channel.invalid_request_body`
- `secure_channel.invalid_response_body`
- `secure_channel.request_id_mismatch`
- `secure_channel.request_response_api_mismatch`
- `secure_channel.invalid_status_body_combination`
- `secure_channel.unexpected_response_diagnostics`
- `secure_channel.missing_response_diagnostics`
- `secure_channel.duplicate_checked_source`
- `secure_channel.duplicate_san_entry`

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- невалидный enum/type;
- request/response нарушают envelope invariants;
- trust summary не привязан к caller identity;
- request/response body не соответствует API family;
- response status не соответствует body family;
- сериализация недетерминирована.

## Замечания по эволюции

- Новые API добавляются через новые `api_name` и payload types.
- Ломающие изменения envelope semantics требуют синхронного обновления:
  - `SecureChannelContract.md`
  - `SecurityModuleContract.md`
  - `docs/IMPLEMENT.md`
  - `docs/STATUS.md`
