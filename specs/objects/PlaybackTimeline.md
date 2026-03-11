# PlaybackTimeline

## Назначение

Dry-run и runtime timeline показа.

## Канонические поля

- `timeline_id` — UUID строка
- `composition_id` — UUID строка
- `segments` — массив timeline segments
- `total_duration_edit_units` — целое >= 0
- `lane_summary` — объект по picture/audio/text

## Инварианты

- Сегменты упорядочены по времени.
- Общая длительность согласована с последним сегментом.

## Связи с другими объектами

- Строится из CompositionGraph.
- Используется audio sync, subtitles и playout orchestrator.

## Каноническая сериализация

- JSON snake_case.
- Сегменты содержат start_edit_unit и duration_edit_units.

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
