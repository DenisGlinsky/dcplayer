# AuditExport

## Назначение

Machine-readable export package для `SecurityEvent` в ветке `T03d`.

Контракт фиксирует:
- export envelope для набора событий;
- deterministic ordering rules;
- completeness/export metadata;
- integrity placeholder metadata;
- tamper/recovery transition checks и event/export linkage.
- direct object-level validation, которая не позволяет обойти nested `SecurityEvent` invariants.

Контракт не включает:
- внешний transport/export delivery;
- signing/encryption реального export blob;
- secure clock source;
- encrypted audit storage.

## Канонические поля

- `export_id` — canonical UUID строка
- `export_time_utc` — UTC ISO-8601
- `events` — массив `SecurityEvent`
- `ordering` — `OrderingRules`
- `export_metadata` — `ExportMetadata`
- `integrity_metadata` — `IntegrityMetadata`

### `OrderingRules`

- `ordered_by` — фиксированная строка `event_time_utc,event_id`
- `direction` — фиксированная строка `ascending`

### `ExportMetadata`

- `completeness` — enum `complete | partial`
- `export_reason` — строка
- `source_channel_id` — строка

### `IntegrityMetadata`

- `placeholder_type` — enum `none | checksum_pending | signature_pending`
- `placeholder_ref` — optional строка; обязательна для `checksum_pending | signature_pending`

## Инварианты

- `export_id` обязан быть canonical UUID.
- `export_time_utc` обязан быть UTC ISO-8601.
- `events` обязаны быть отсортированы по `event_time_utc`, затем по `event_id`.
- `event_id` обязан быть уникален внутри одного export.
- Если `SecurityEvent.export_id_ref` присутствует, он обязан совпадать с `export_id`.
- `export.audit_exported` внутри export-а обязан иметь `export_id_ref`, совпадающий с `export_id`.
- `tamper.tamper_lockout` требует prior `tamper.tamper_detected`.
- `recovery.recovery_started` требует активного tamper state (`detected` или `locked_out`).
- `recovery.recovery_completed` и `recovery.recovery_failed` требуют prior `recovery.recovery_started`.
- Контракт не публикует partial object как valid result при error-level diagnostic.
- direct `validate_audit_export()` обязан возвращать `validation_status=error` при любом error-level diagnostic.
- direct `validate_audit_export()` обязан применять тот же event-level invariant set, что и parse-path `SecurityEvent`.
- parse-path и direct validation не должны наслаивать ложный `audit.missing_required_field` поверх invalid-present nested event field.

## Deterministic validation

Минимально обязательные machine-readable diagnostics:

- `audit.invalid_event_type`
- `audit.invalid_severity`
- `audit.invalid_event_time`
- `audit.duplicate_event_id`
- `audit.export_ordering_mismatch`
- `audit.invalid_event_export_linkage`
- `audit.invalid_tamper_transition`
- `audit.missing_required_field`

Дополнительно допускаются baseline parse/shape diagnostics:

- `audit.json_malformed`
- `audit.invalid_type`
- `audit.invalid_value`
- `audit.invalid_identifier`
- `audit.invalid_export_completeness`
- `audit.invalid_integrity_placeholder`

Для одинакового дефекта код диагностики обязан оставаться стабильным и platform-independent.

## Связи с другими объектами

- Агрегирует `SecurityEvent`.
- Используется как export-level audit contract для последующих security веток.

## Каноническая сериализация

- JSON `snake_case`
- deterministic field order:
  - `export_id`
  - `export_time_utc`
  - `events`
  - `ordering`
  - `export_metadata`
  - `integrity_metadata`
- `events` сериализуются уже в каноническом export order.
- nested `payload_metadata` внутри event-а сериализуется как отсортированный string-map.

## Причины отклонения / ошибки

- отсутствует обязательное поле export-а или nested event-а;
- нарушен UUID/time invariant;
- нарушен порядок `events`;
- обнаружен duplicate `event_id`;
- event/export linkage не совпадает;
- нарушена tamper/recovery state machine;
- сериализация недетерминирована.

## Замечания по эволюции

- Реальная подпись, шифрование и transport добавляются только поверх этого export contract.
- Ломающие изменения export semantics требуют синхронного обновления:
  - `SecurityEvent.md`
  - `docs/IMPLEMENT.md`
  - `docs/STATUS.md`
- При изменении security semantics нужно дополнительно проверить trust boundary и audit model.
