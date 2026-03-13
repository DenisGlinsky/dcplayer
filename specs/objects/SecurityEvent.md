# SecurityEvent

## Назначение

Machine-readable security event для host-plane и secure-plane audit surface ветки `T03d`.

Контракт фиксирует:
- каноническую модель security event;
- baseline event families;
- actor/peer identity summary;
- tamper/recovery/export semantics на model уровне;
- единый contract-level validator для parse-path и direct object validation;
- deterministic diagnostics и canonical JSON serialization.

Контракт не включает:
- runtime log transport;
- secure clock implementation;
- encrypted log storage;
- signing/encryption реальных log blobs;
- device GPIO / TPM / ZymKey integration.

## Канонические поля

- `event_id` — canonical UUID строка
- `event_type` — enum baseline event type
- `severity` — enum `info | warning | error | critical`
- `event_time_utc` — UTC ISO-8601
- `producer_role` — enum `pi_zymkey | ubuntu_tpm`
- `actor_identity` — optional `IdentitySummary`
- `peer_identity` — optional `IdentitySummary`
- `correlation_id` — optional строка
- `request_id` — optional canonical UUID строка
- `export_id_ref` — optional canonical UUID строка
- `payload_metadata` — объект `string -> string`
- `decision_summary` — optional `DecisionSummary`
- `result_summary` — optional `ResultSummary`

### `IdentitySummary`

- `role` — enum `pi_zymkey | ubuntu_tpm`
- `device_id` — строка
- `certificate_fingerprint` — lowercase hex строка

### `DecisionSummary`

- `decision` — непустая строка

### `ResultSummary`

- `result` — непустая строка

## Baseline event families и event types

- `auth` -> `auth.session_authenticated`
- `trust` -> `trust.peer_validated`
- `acl` -> `acl.decision_recorded`
- `api` -> `api.request_processed`
- `tamper` -> `tamper.tamper_detected`, `tamper.tamper_lockout`
- `recovery` -> `recovery.recovery_started`, `recovery.recovery_completed`, `recovery.recovery_failed`
- `export` -> `export.audit_exported`

## Инварианты

- `event_id` обязан быть canonical UUID.
- `event_time_utc` обязан быть UTC ISO-8601.
- `event_type`, `severity` и `producer_role` обязаны входить в baseline enum surface даже на direct object-level path.
- `producer_role` ограничен baseline ролями secure control plane.
- `payload_metadata` обязателен и сериализуется как отсортированный string-map.
- `actor_identity` и `peer_identity` не должны содержать секретов, key material или plaintext essence.
- `auth.session_authenticated` требует `actor_identity`, `peer_identity` и `decision_summary`.
- `trust.peer_validated` требует `peer_identity` и `decision_summary`.
- `acl.decision_recorded` требует `actor_identity` и `decision_summary`.
- `api.request_processed` требует `actor_identity`, `request_id` и `result_summary`.
- `tamper.*`, `recovery.*` и `export.audit_exported` требуют `result_summary`.
- `export.audit_exported` требует `export_id_ref`.
- `request_id` и `export_id_ref`, если присутствуют, обязаны быть canonical UUID.
- Событие не должно переносить transport/runtime state, который отсутствует в `T03d`.
- direct object-level validation обязан давать тот же diagnostic surface, что и parse-path, для event-level дефектов.
- Для nested optional objects (`actor_identity`, `peer_identity`, `decision_summary`, `result_summary`) `audit.missing_required_field` допускается только для реально absent поля; `present but invalid` сохраняет только свой специфичный diagnostic.

## Tamper / recovery semantics

На уровне модели фиксируются следующие event types:

- `tamper.tamper_detected`
- `tamper.tamper_lockout`
- `recovery.recovery_started`
- `recovery.recovery_completed`
- `recovery.recovery_failed`

Сами runtime reactions, GPIO/device plumbing и аппаратные interrupt flows не входят в этот контракт.

## Связи с другими объектами

- Может агрегироваться в `AuditExport`.
- Может ссылаться на `ProtectedApiEnvelope.request_id`.
- Использует тот же baseline roles/identity vocabulary, что и `SecureChannelContract` и `SecurityModuleContract`.

## Каноническая сериализация

- JSON `snake_case`
- deterministic field order:
  - `event_id`
  - `event_type`
  - `severity`
  - `event_time_utc`
  - `producer_role`
  - optional `actor_identity`
  - optional `peer_identity`
  - optional `correlation_id`
  - optional `request_id`
  - optional `export_id_ref`
  - `payload_metadata`
  - optional `decision_summary`
  - optional `result_summary`
- `payload_metadata` сериализуется как отсортированный string-map.

## Минимальная диагностика

- `audit.json_malformed`
- `audit.missing_required_field`
- `audit.invalid_type`
- `audit.invalid_value`
- `audit.invalid_identifier`
- `audit.invalid_event_type`
- `audit.invalid_severity`
- `audit.invalid_event_time`
- `audit.invalid_producer_role`
- `audit.invalid_identity_role`
- `audit.invalid_fingerprint`

## Причины отклонения / ошибки

- отсутствует обязательное audit field;
- невалиден UUID/time/enum;
- event type не входит в baseline families;
- нарушены event-specific required fields;
- `export_id_ref` или `request_id` не каноничны;
- сериализация недетерминирована.

## Замечания по эволюции

- Новые event families или event types добавляются без ломания baseline fields.
- Ломающие изменения event semantics требуют синхронного обновления:
  - `AuditExport.md`
  - `docs/IMPLEMENT.md`
  - `docs/STATUS.md`
- При изменении security semantics нужно дополнительно проверить trust boundary и audit model.
