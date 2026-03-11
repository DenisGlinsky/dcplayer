# WatermarkState

## Назначение

Состояние backend-а forensic watermark во время self-test и runtime.

## Канонические поля

- `backend_id` — строка
- `enabled` — bool
- `policy_mode` — строка
- `lifecycle_state` — enum
- `last_self_test_result` — enum
- `failure_reason` — строка или `null`
- `changed_at_utc` — UTC ISO-8601

## Enum baseline

### `lifecycle_state`
- `disabled`
- `self_test`
- `armed`
- `inserting`
- `faulted`

### `last_self_test_result`
- `unknown`
- `pass`
- `fail`

## Инварианты

- При `enabled=false` причина должна быть объяснима operator-facing state.
- `lifecycle_state=inserting` допустим только при `enabled=true`.
- `last_self_test_result=fail` несовместим с `policy_mode=required` и `lifecycle_state=armed|inserting`.

## Связи с другими объектами

- Используется secure playback integration и operator alerts.
- Совместим с `WatermarkPayload`.

## Каноническая сериализация

- JSON `snake_case`.

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- нарушен инвариант;
- формат времени, enum или идентификатора невалиден;
- объект противоречит связанным контрактам;
- сериализация недетерминирована.

## Замечания по эволюции

- Обратимо-совместимые расширения добавляются новыми полями.
- Ломающие изменения требуют явного обновления `docs/PLAN.md`, `docs/STATUS.md` и связанных task-specific `AGENTS.md`.
- При изменении security semantics нужно дополнительно проверить trust boundary и audit model.
