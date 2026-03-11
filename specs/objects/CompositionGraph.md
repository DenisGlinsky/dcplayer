# CompositionGraph

## Назначение

Итоговый граф composition после разрешения связей `CPL -> PKL -> AssetMap` и зависимостей OV/VF.
Объект описывает уже нормализованный состав показа без timeline execution и без supplemental merge semantics.

## Канонические поля

- `composition_id` — UUID строка целевой composition
- `content_title_text` — строка из выбранного `CPL`
- `origin_package_id` — UUID строки `PKL.pkl_id` пакета, которому принадлежит выбранный `CPL`
- `composition_kind` — enum `ov | vf`
- `source_packages` — массив UUID строк `PKL.pkl_id`, которые реально участвуют в итоговом графе; список уникален и стабильно отсортирован
- `resolved_reels` — массив `Reel` в исходном порядке `CPL.ReelList`
- `missing_assets` — массив UUID строк asset references, для которых не найден ни один backing `PKL` asset; список уникален и стабильно отсортирован

## Инварианты

- Граф должен быть детерминирован для одного и того же набора нормализованных `AssetMap` / `PKL` / `CPL`.
- `origin_package_id` обязан входить в `source_packages`.
- `composition_kind = ov` допустим только если все разрешённые `TrackFile` в `resolved_reels` имеют `dependency_kind = local`.
- `composition_kind = vf` фиксируется, если хотя бы один разрешённый `TrackFile` получен из внешнего по отношению к `origin_package_id` пакета.
- `resolved_reels` сохраняет порядок reel из исходного `CPL`; сортировка reels запрещена.
- `missing_assets` не содержит дубликатов и не перечисляет asset ids, которые уже присутствуют в разрешённых `TrackFile`.

## Связи с другими объектами

- Строится resolver-ом OV/VF на основе `CPL`, `PKL` и `AssetMap`.
- Использует объект `Reel`, внутри которого лежат разрешённые `TrackFile`.
- Кормит будущий `PlaybackTimeline`, но сам не исполняет timeline semantics.

## Каноническая сериализация

- JSON snake_case.
- `source_packages` и `missing_assets` сериализуются в стабильном лексикографическом порядке.
- `resolved_reels` сериализуется в порядке исходного `CPL`.

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- нарушен инвариант;
- формат времени, enum или идентификатора невалиден;
- объект противоречит связанным контрактам;
- один и тот же asset reference разрешается неоднозначно;
- сериализация недетерминирована.

## Замечания по эволюции

- Обратимо-совместимые расширения добавляются новыми полями.
- Добавление supplemental merge policy, timeline execution или runtime security semantics требует отдельных companion-specs и отдельных веток.
- Ломающие изменения требуют явного обновления docs/PLAN.md, docs/STATUS.md и связанных task-specific AGENTS.md.
- При изменении security semantics нужно дополнительно проверить trust boundary и audit model.
