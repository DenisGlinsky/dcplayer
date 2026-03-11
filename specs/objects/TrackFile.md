# TrackFile

## Назначение

Нормализованное описание essence track file.

## Канонические поля

- `asset_id` — UUID строка
- `track_type` — enum: picture/sound/subtitle/aux
- `path_hint` — относительный путь
- `encrypted` — bool
- `duration_edit_units` — целое >= 0
- `edit_rate` — рациональная пара

## Инварианты

- track_type обязателен.
- duration_edit_units не отрицателен.
- Encrypted track требует отдельного security path при playout.

## Связи с другими объектами

- Используется в Reel и CompositionGraph.
- В secure path участвует в HsmDecryptSession.

## Каноническая сериализация

- JSON snake_case.
- track_type сериализуется строковым enum.

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
