# SupplementalMergePolicy

## Назначение

Политика merge/override для применения одного supplemental composition к уже собранному `CompositionGraph` из этапа OV/VF resolution.
Объект не строит base graph заново и не исполняет timeline; он только описывает, какой supplemental composition можно наложить на какой base graph и какие lane разрешено override-ить.

## Канонические поля

- `policy_id` — строка
- `base_composition_id` — UUID строка
- `supplemental_composition_id` — UUID строка
- `merge_mode` — enum `replace_lane`
- `allowed_overrides` — массив lane names `picture | sound | subtitle`

## Инварианты

- `base_composition_id` обязателен и должен совпадать с `CompositionGraph.composition_id`, к которому применяется policy.
- `supplemental_composition_id` обязателен.
- `merge_mode` обязан быть одним из поддерживаемых enum-значений реализации; в `T02b` поддерживается только `replace_lane`.
- `merge_mode = replace_lane` означает только замену уже существующего lane в base graph; добавление новых reel или новых lane вне base graph запрещено.
- `allowed_overrides` не содержит дубликатов.
- `allowed_overrides` содержит только поддерживаемые lane values `picture | sound | subtitle`.
- Две разные policy могут применяться к одному base graph, но если они дают разные resolved track для одного и того же `(reel_id, lane)`, это считается конфликтом и show-ready результат не публикуется.

## Связи с другими объектами

- Используется поверх нормализованных моделей `AssetMap`, `PKL`, `CPL` и `CompositionGraph`.
- Влияет на итоговый `CompositionGraph`.
- Не меняет и не исполняет `PlaybackTimeline`.

## Каноническая сериализация

- JSON snake_case.
- merge_mode — строковый enum.
- `allowed_overrides` сериализуется как массив строковых enum в стабильном порядке `picture`, `sound`, `subtitle`.

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- нарушен инвариант;
- формат времени, enum или идентификатора невалиден;
- объект противоречит связанным контрактам;
- `base_composition_id` не совпадает с входным `CompositionGraph`;
- `merge_mode` не поддерживается данной реализацией;
- supplemental composition не найден среди backed owner candidates;
- supplemental composition принадлежит нескольким backed packages;
- `allowed_overrides` содержит невалидный или дублирующийся lane value;
- supplemental пытается override-ить lane вне `allowed_overrides`;
- supplemental reference не имеет валидного local backing и не совпадает с допустимой base dependency;
- explicit base dependency совпадает по `asset_id`, но расходится по `edit_rate`;
- две policy дают конфликтующий override для одного `(reel_id, lane)`;
- сериализация недетерминирована.

## Замечания по эволюции

- Обратимо-совместимые расширения добавляются новыми полями.
- Ломающие изменения требуют явного обновления docs/PLAN.md, docs/STATUS.md и связанных task-specific AGENTS.md.
- При изменении security semantics нужно дополнительно проверить trust boundary и audit model.

## Семантика применения

Policy применяется к уже show-ready `CompositionGraph` по следующим правилам:

- supplemental composition выбирается тем же backed-owner правилом, что и в `T02a`: нужен parsed `CPL`, `PKL` asset типа `composition_playlist` с тем же id и валидный `AssetMap` backing этого `CPL`;
- supplemental reel сопоставляется с base graph по `reel_id`;
- если supplemental lane отсутствует, base lane наследуется без изменений;
- если supplemental lane ссылается на локальный `track_file` с валидным backing и lane разрешён policy, итоговый `TrackFile` заменяется на supplemental-local track;
- если локального supplemental backing нет, reference считается допустимой base dependency только тогда, когда одновременно совпадают `asset_id`, lane identity и `edit_rate` уже существующего `TrackFile` того же lane в base graph;
- при любой диагностике уровня `error` итоговый `CompositionGraph` не публикуется как show-ready результат.
