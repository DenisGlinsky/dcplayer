# PlaybackSession

## Назначение

Состояние текущей playback session независимо от open/secure mode.

## Канонические поля

- `session_id` — UUID строка
- `composition_id` — UUID строка
- `mode` — enum open/secure
- `authorization_state` — enum
- `start_time_utc` — UTC ISO-8601 или null
- `stop_time_utc` — UTC ISO-8601 или null
- `outcome` — enum или null

## Инварианты

- Mode обязателен.
- Для secure mode authorization_state не может молча деградировать в open path.

## Связи с другими объектами

- Используется playout orchestrator, SMS/TMS и logs.

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
