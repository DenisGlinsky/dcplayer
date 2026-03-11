# WatermarkPayload

## Назначение

Канонический payload forensic watermark. Это единый объектный контракт для inserter и detector.

## Канонические поля

- `payload_id` — UUID строка
- `session_id` — UUID строка
- `site_code` — строка
- `screen_code` — строка
- `time_slice` — строка или номер окна
- `attack_matrix_id` — строка
- `format_version` — строка
- `content_kind` — enum `picture | sound`

## Инварианты

- Payload version обязателен.
- Payload не должен зависеть от конкретного backend implementation.
- `attack_matrix_id` обязателен для любых claims уровня “100% идентификация”.
- Один payload не должен одновременно объявляться как `picture` и `sound`.

## Control policy bits

- `fm_required` — bool
- `self_test_required` — bool
- `operator_override_allowed` — bool

## Связи с другими объектами

- Используется watermark backends и detector harness.
- Связан с `WatermarkState`, `WatermarkEvidence`, `DetectorReport`.

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
