# SecureClockPolicy

## Назначение

Контракт `T03e` фиксирует machine-readable semantics защищённого времени без runtime RTC/NTP/PTP integration.

Документ покрывает два связанных объекта:
- `SecureClockPolicy` — policy-level правила доверия и gating;
- `SecureTimeStatus` — snapshot текущего secure-time состояния, который потребляется policy evaluator-ом.

Контракт не включает:
- реальный доступ к RTC/device;
- live clock synchronization;
- TPM/ZymKey runtime binding;
- runtime playback, KDM parsing и decrypt execution.

## `SecureClockPolicy`

### Канонические поля

- `clock_id` — строка
- `max_allowed_skew_seconds` — целое `>= 0`
- `freshness_window_seconds` — целое `>= 0`
- `monotonic_required` — bool
- `allowed_time_sources` — массив enum `time_source`
- `fail_closed_on_untrusted_time` — bool

### Инварианты

- `clock_id` обязателен.
- `clock_id` обязан быть JSON string; отсутствие ключа даёт `secure_time.missing_required_time_field`, `null` или другой тип дают `secure_time.invalid_type`.
- `allowed_time_sources` обязателен, не пуст и не допускает повторов.
- отрицательные значения budget/window запрещены.
- policy сериализуется детерминированно; `allowed_time_sources` упорядочиваются канонически.

## `SecureTimeStatus`

### Канонические поля

- `secure_time_status` — enum `trusted | degraded | untrusted | unavailable`
- `current_time_utc` — UTC ISO-8601; обязателен, если `secure_time_status != unavailable`
- `time_source` — enum `rtc_secure | rtc_untrusted | imported_secure_time | operator_set_time_placeholder | unknown`
- `source_role` — enum `primary | fallback | placeholder | unknown`
- `source_trust_state` — enum `trusted | untrusted | unknown`
- `skew_seconds` — целое `>= 0`; обязателен, если `secure_time_status != unavailable`
- `drift_state` — enum `within_policy | skew_exceeded | unknown`
- `freshness` — enum `fresh | stale | unknown`
- `last_update_utc` — UTC ISO-8601; обязателен, если `secure_time_status != unavailable`
- `monotonicity_status` — enum `monotonic | non_monotonic | unknown`
- `tamper_time_state` — enum `clear | tamper_detected | tamper_lockout | recovery_in_progress`

### Инварианты

- `current_time_utc` и `last_update_utc` используют canonical UTC form.
- `skew_seconds` не может быть отрицательным.
- для `secure_time_status=unavailable` поля времени/skew могут отсутствовать, но остальные status/policy поля остаются machine-readable.
- для `secure_time_status!=unavailable` отсутствие `current_time_utc`, `last_update_utc` или `skew_seconds` считается contract defect.
- `tamper_time_state != clear` не делает источник автоматически валидным даже при корректных UTC/skew полях.

## Security gating semantics

Secure time считается **trusted**, только если одновременно выполняются условия:
- `secure_time_status == trusted`;
- `time_source` входит в `allowed_time_sources`;
- `source_trust_state == trusted`;
- `freshness == fresh` и `current_time_utc - last_update_utc <= freshness_window_seconds`;
- `drift_state != skew_exceeded` и `skew_seconds <= max_allowed_skew_seconds`;
- при `monotonic_required=true` соблюдается monotonicity;
- `tamper_time_state == clear`.

Secure time считается **usable** для timestamp-dependent operations, только если:
- `secure_time_status == trusted`;
- `time_source` входит в `allowed_time_sources`;
- freshness/skew/monotonicity не нарушают policy;
- `tamper_time_state == clear`;
- источник либо trusted, либо переведён в warning-only состояние правилом `fail_closed_on_untrusted_time=false`.

Secure time считается **unavailable**, если:
- `secure_time_status == unavailable`; или
- активен `tamper_time_state` из lockout/recovery path; или
- отсутствуют обязательные time fields для non-unavailable статуса.

Timestamp-dependent operations на model уровне обязаны блокироваться, если:
- `secure_time_status != trusted`;
- secure time unavailable;
- источник не разрешён policy;
- freshness вышла за `freshness_window_seconds`;
- skew/drift вышли за `max_allowed_skew_seconds`;
- нарушена monotonicity при `monotonic_required=true`;
- источник untrusted и `fail_closed_on_untrusted_time=true`.

При `fail_closed_on_untrusted_time=false` warning-only режим применяется только к source-level untrusted defect для otherwise-trusted snapshot. Он не превращает `secure_time_status != trusted`, `stale`, `skew_exceeded`, `non_monotonic`, `policy_source_not_allowed` или `unavailable` в допустимые случаи.

## Deterministic validation

Минимально обязательные machine-readable diagnostics:
- `secure_time.invalid_time_format`
- `secure_time.invalid_time_source`
- `secure_time.untrusted_time_source`
- `secure_time.stale_time`
- `secure_time.skew_exceeded`
- `secure_time.non_monotonic_time`
- `secure_time.missing_required_time_field`
- `secure_time.policy_source_not_allowed`
- `secure_time.secure_time_unavailable`

Допустимы baseline structural diagnostics:
- `secure_time.json_malformed`
- `secure_time.invalid_type`
- `secure_time.invalid_value`

Одинаковый дефект обязан давать одинаковый diagnostic code вне зависимости от платформы.

## Связи с другими объектами

- Используется `T03e`, `T04a` и последующим KDM/show-window gating.
- `SecurityModuleContract` опирается на этот policy surface для timestamp-dependent APIs.
- `SecurityEvent` / `AuditExport` могут ссылаться на secure-time состояния и tamper/time decisions, но runtime log binding сюда не входит.

## Каноническая сериализация

- JSON `snake_case`
- deterministic field order
- integer budgets/status values сериализуются числами, не строками
- enum сериализуются строками
- строковые поля сериализуются как валидные JSON strings с escaping для `"`, `\` и control chars; parser принимает standard JSON escapes, включая `\uXXXX`
- diagnostics используют JSON-pointer-like `path`

Raw control chars внутри JSON string считаются malformed JSON и отклоняются на parse stage с `secure_time.json_malformed`.

## Причины отклонения / ошибки

- отсутствует обязательное поле policy/status;
- невалиден UTC format, enum или целочисленное значение;
- источник времени нарушает policy/gating invariant;
- tamper/time состояние делает secure time unusable;
- сериализация недетерминирована.

## Замечания по эволюции

- Обратимо-совместимые расширения добавляются новыми полями и enum values.
- Ломающие изменения требуют явного обновления `docs/PLAN.md`, `docs/STATUS.md` и связанных task-specific `AGENTS.md`.
- При изменении security semantics нужно дополнительно проверить trust boundary, audit model и `SecurityModuleContract`.
