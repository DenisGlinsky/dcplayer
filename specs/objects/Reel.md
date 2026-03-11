# Reel

## Назначение

Каноническая модель reel внутри `CompositionGraph` после resolver-этапа OV/VF и, при наличии, после supplemental merge.
Reel хранит только уже разрешённые picture/sound/subtitle lanes без timeline execution.

## Канонические поля

- `reel_id` — UUID строка
- `picture_track` — TrackFile или null
- `sound_track` — TrackFile или null
- `subtitle_track` — TrackFile или null

## Инварианты

- Хотя бы один из `picture_track`, `sound_track`, `subtitle_track` обязан быть заполнен.
- В одном reel каждый поддерживаемый lane встречается не более одного раза.
- `picture_track.track_type`, `sound_track.track_type` и `subtitle_track.track_type`, если поле заполнено, должны совпадать с именем lane.

## Связи с другими объектами

- Формируется из `CPL.Reel` после разрешения asset references.
- При supplemental merge сопоставляется по `reel_id` с reel из supplemental `CPL`; новые reel вне base graph не добавляются.
- Входит в `CompositionGraph`.
- Является входом для будущего `PlaybackTimeline`.

## Каноническая сериализация

- JSON snake_case.
- Незаполненные lane сериализуются как null.

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- нарушен инвариант;
- формат времени, enum или идентификатора невалиден;
- объект противоречит связанным контрактам;
- сериализация недетерминирована.

## Замечания по эволюции

- Обратимо-совместимые расширения добавляются новыми полями.
- Добавление supplemental-only lanes или timeline-specific metadata требует отдельного обновления companion-specs.
- Ломающие изменения требуют явного обновления docs/PLAN.md, docs/STATUS.md и связанных task-specific AGENTS.md.
- При изменении security semantics нужно дополнительно проверить trust boundary и audit model.
