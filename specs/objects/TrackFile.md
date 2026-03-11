# TrackFile

## Назначение

Нормализованное описание разрешённого essence track file внутри `CompositionGraph`.
Объект фиксирует, какой asset из какого пакета реально участвует в показе после resolver-этапа OV/VF и, при наличии, после supplemental override.

## Канонические поля

- `asset_id` — UUID строка
- `track_type` — enum `picture | sound | subtitle`
- `edit_rate` — рациональная пара `{numerator, denominator}` из `CPL` asset reference
- `resolved_path` — относительный POSIX-style путь из первого `AssetMap` chunk разрешённого asset
- `source_package_id` — UUID строки `PKL.pkl_id` пакета, из которого взят asset
- `dependency_kind` — enum `local | external`

## Инварианты

- track_type обязателен.
- `edit_rate` обязателен и участвует в проверке explicit base dependency при supplemental merge.
- `resolved_path` обязателен, относителен и уже нормализован по правилам `AssetMap`.
- `dependency_kind = local` допустим только если `source_package_id` совпадает с `CompositionGraph.origin_package_id`.
- `dependency_kind = external` допустим только если `source_package_id` отличается от `CompositionGraph.origin_package_id`.

## Связи с другими объектами

- Используется в `Reel` и `CompositionGraph`.
- Формируется resolver-ом или supplemental merge-слоем из связки `CPL asset reference + PKL asset + AssetMap asset`.
- Runtime/security-поля (`encrypted`, decrypt session и т.п.) остаются вне scope `T02a/T02b`.

## Каноническая сериализация

- JSON snake_case.
- track_type сериализуется строковым enum.
- dependency_kind сериализуется строковым enum.

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- нарушен инвариант;
- формат времени, enum или идентификатора невалиден;
- объект противоречит связанным контрактам;
- сериализация недетерминирована.

## Замечания по эволюции

- Обратимо-совместимые расширения добавляются новыми полями.
- Добавление runtime-specific полей требует отдельной ветки и синхронизации с `PlaybackTimeline`/security specs.
- Ломающие изменения требуют явного обновления docs/PLAN.md, docs/STATUS.md и связанных task-specific AGENTS.md.
- При изменении security semantics нужно дополнительно проверить trust boundary и audit model.
