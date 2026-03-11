# PlaybackTimeline

## Назначение

Нормализованная machine-readable модель dry-run playback timeline, построенная из уже show-ready `CompositionGraph` без decode, scheduler и device output.
В рамках `T02c` объект фиксирует reel-level sequence и lane provenance для picture / sound / subtitle и не вводит runtime/pll/sync semantics.

## Канонические поля

- `composition_id` — UUID строка
- `origin_package_id` — UUID строки пакета-владельца целевой composition
- `composition_kind` — enum `ov | vf`
- `source_packages` — уникальный стабильно отсортированный массив UUID строк пакетов, реально участвующих в итоговом timeline
- `reel_entries` — непустой массив reel-level timeline entries в порядке `CompositionGraph.resolved_reels`
- `lane_summary` — объект со счётчиками reel entries по `picture`, `sound`, `subtitle`

Каждый `reel_entry` содержит:

- `entry_index` — zero-based индекс reel entry в timeline
- `reel_id` — UUID строка
- `edit_rate` — рациональная пара `{numerator, denominator}`, единая для всех присутствующих lane внутри данного entry
- `picture` — optional lane entry или `null`
- `sound` — optional lane entry или `null`
- `subtitle` — optional lane entry или `null`

Каждый lane entry содержит:

- `asset_id` — UUID строки разрешённого `TrackFile`
- `track_type` — enum `picture | sound | subtitle`
- `edit_rate` — рациональная пара `{numerator, denominator}`
- `resolved_path` — нормализованный относительный path из `TrackFile`
- `source_package_id` — UUID строки пакета, из которого взят asset
- `dependency_kind` — enum `local | external`

## Инварианты

- Timeline строится только из show-ready `CompositionGraph`: `missing_assets` пуст, `resolved_reels` не пуст, `origin_package_id` присутствует в `source_packages`.
- `reel_entries` сохраняют порядок `CompositionGraph.resolved_reels`; перестановка reels запрещена.
- Каждый `reel_entry` обязан содержать хотя бы один из lane `picture`, `sound`, `subtitle`.
- Если lane присутствует, его `track_type` обязан совпадать с именем lane.
- Если lane присутствует, его `source_package_id` обязан входить в `source_packages`.
- `dependency_kind = local` допустим только если `source_package_id == origin_package_id`.
- `dependency_kind = external` допустим только если `source_package_id != origin_package_id`.
- `composition_kind = ov` допустим только если все lane timeline provenance имеют `dependency_kind = local`.
- `composition_kind = vf` допустим только если хотя бы один lane timeline provenance имеет `dependency_kind = external`.
- Все присутствующие lane внутри одного `reel_entry` обязаны иметь одинаковый положительный `edit_rate`; при расхождении dry-run builder обязан вернуть детерминированную ошибку.
- `lane_summary` содержит количество reel entries, где соответствующий lane присутствует.
- Если для lane уже зафиксирован более специфичный дефект `invalid_edit_rate`, builder не должен добавлять вторичный generic fallback `timeline.unsupported_graph_shape` только из-за отсутствия пригодного entry-level `edit_rate`.
- Global `timeline.composition_kind_dependency_mismatch` добавляется только если для graph уже нет более раннего достаточного structural/semantic дефекта, который сам по себе делает dry-run невозможным.

`T02c` намеренно не включает effective duration и wall-clock scheduling:
текущий `CompositionGraph` не несёт duration metadata, поэтому dry-run timeline на этом этапе фиксирует только reel sequence, lane presence, provenance и entry-level `edit_rate`.

## Связи с другими объектами

- Строится из уже show-ready `CompositionGraph` после `T02a` resolver и, при наличии, `T02b` supplemental merge.
- Используется как вход для будущих веток sync/subtitles/playout orchestration, но сам не выполняет runtime execution.

## Каноническая сериализация

- JSON snake_case.
- `source_packages` сериализуется в стабильном порядке.
- `reel_entries` сериализуется в порядке reel timeline.
- absent lane сериализуются как `null`.
- рациональные значения сериализуются как `{numerator, denominator}`.

## Причины отклонения / ошибки

- `timeline.graph_not_show_ready`
- `timeline.empty_reel_list`
- `timeline.lane_type_mismatch`
- `timeline.composition_kind_dependency_mismatch`
- `timeline.invalid_edit_rate`
- `timeline.entry_edit_rate_mismatch`
- `timeline.unsupported_graph_shape`

Формат диагностики совпадает с `T02a/T02b`: `code + severity + path + message`, сортировка детерминирована как `error before warning`, затем `code`, затем `path`.

## Замечания по эволюции

- Обратимо-совместимые расширения добавляются новыми полями.
- Добавление duration, runtime scheduling, sync state, decode queue и device-output semantics требует отдельных веток и обновления связанных companion-specs.
- Ломающие изменения требуют явного обновления docs/PLAN.md, docs/STATUS.md и связанных task-specific AGENTS.md.
- При изменении security semantics нужно дополнительно проверить trust boundary и audit model.
