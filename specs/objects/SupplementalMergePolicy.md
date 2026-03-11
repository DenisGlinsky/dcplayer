# SupplementalMergePolicy

## Назначение

Политика merge/override для supplemental, OV и VF.

## Канонические поля

- `policy_id` — строка
- `base_composition_id` — UUID строка
- `supplemental_composition_id` — UUID строка
- `merge_mode` — enum
- `allowed_overrides` — массив lane names

## Инварианты

- Нельзя одновременно разрешать конфликтующие override-правила.
- Base composition обязателен.

## Связи с другими объектами

- Используется T02a/T02b.
- Влияет на CompositionGraph и PlaybackTimeline.

## Каноническая сериализация

- JSON snake_case.
- merge_mode — строковый enum.

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
