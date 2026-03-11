# WatermarkEvidence

## Назначение

Нормализованное свидетельство извлечения watermark.

## Канонические поля

- `evidence_id` — UUID строка
- `payload_id` — UUID строка
- `detector_version` — строка
- `confidence` — число 0..1
- `attack_tags` — массив строк
- `sample_span_seconds` — число > 0
- `retention_class` — enum

## Инварианты

- Confidence ограничен диапазоном 0..1.
- Evidence ссылается на конкретный payload.
- `sample_span_seconds` должен быть положительным.

## Retention semantics

### `retention_class`
- `ephemeral`
- `operator_visible`
- `forensic_archive`

## Связи с другими объектами

- Строится detector harness.
- Входит в `DetectorReport`.

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
