# KDMEnvelope

## Назначение

Нормализованное описание импортированного KDM.

## Канонические поля

- `kdm_id` — UUID строка
- `composition_id` — UUID строка
- `recipient_cert_fingerprint` — hex строка
- `valid_from_utc` — UTC ISO-8601
- `valid_until_utc` — UTC ISO-8601

## Инварианты

- valid_from_utc <= valid_until_utc.
- Привязка к composition обязательна.

## Связи с другими объектами

- Проверяется через TrustChain и SecureClockPolicy.

## Каноническая сериализация

- JSON snake_case.

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- нарушен инвариант;
- формат времени, enum или идентификатора невалиден;
- объект противоречит связанным контрактам;
- сериализация недетерминирована.

## Замечания по эволюции

- Обратимо-совместимые расширения добавляются новыми полями.
- Ломающие изменения требуют явного обновления docs/PLAN.md, docs/STATUS.md и связанных task-specific AGENTS.md.
- При изменении security semantics нужно дополнительно проверить trust boundary и audit model.
