# CPL

## Назначение

Каноническая нормализованная модель Composition Playlist как самостоятельного DCP-объекта.
В рамках `T01b` объект разбирается и валидируется без OV/VF resolving, supplemental merge и без проверки наличия asset-файлов на диске.

## Канонические поля

- `composition_id` — UUID строка в lower-case canonical form
- `content_title_text` — обязательная непустая строка
- `annotation_text` — optional строка
- `issuer` — обязательная непустая строка
- `creator` — optional строка
- `content_kind` — optional строка
- `issue_date_utc` — UTC ISO-8601 в форме `YYYY-MM-DDTHH:MM:SSZ`
- `edit_rate` — обязательная рациональная пара `{numerator, denominator}`
- `namespace_uri` — namespace URI root CPL
- `schema_flavor` — `interop | smpte | unknown`
- `reels` — непустой массив reel

## Нормализованная модель reel

Каждый reel содержит:

- `reel_id` — UUID строка в lower-case canonical form
- `picture` — optional asset reference
- `sound` — optional asset reference
- `subtitle` — optional asset reference

Asset reference внутри reel содержит:

- `asset_id` — UUID строка в lower-case canonical form
- `track_type` — `picture | sound | subtitle`
- `annotation_text` — optional строка
- `edit_rate` — обязательная рациональная пара `{numerator, denominator}`

`T01b` не добавляет path resolving, PKL/AssetMap membership, encryption semantics, timeline fields и playback execution semantics.

## Инварианты

- Root element должен быть `CompositionPlaylist`.
- `composition_id`, `content_title_text`, `issuer`, `issue_date_utc`, `edit_rate` и `reels` обязательны.
- `issue_date_utc` принимается только в форме `YYYY-MM-DDTHH:MM:SSZ`.
- `edit_rate` принимается только в форме двух положительных целых чисел через пробел.
- Список reel не пустой.
- Каждый reel обязан иметь `reel_id`.
- Каждый reel обязан содержать хотя бы один из track reference: `MainPicture`, `MainSound`, `MainSubtitle`.
- Для каждого track reference обязательны `Id` и `EditRate`.
- Duplicate `reel_id` внутри одного CPL запрещён.
- Duplicate track type внутри одного reel запрещён.
- Duplicate `asset_id` внутри одного reel запрещён.
- Unknown namespace не делает объект невалидным, но обязан поднимать детерминированный warning.

## Диагностика `T01b`

Формат диагностики совпадает с `T01a`:

- `code` — стабильный машинно-читаемый код
- `severity` — `error | warning`
- `path` — XPath-like путь
- `message` — стабильное platform-independent сообщение

Минимальный набор кодов для `T01b`:

- `cpl.xml_malformed`
- `cpl.unsupported_namespace`
- `cpl.missing_required_field`
- `cpl.invalid_uuid`
- `cpl.invalid_time`
- `cpl.invalid_edit_rate`
- `cpl.empty_reel_list`
- `cpl.duplicate_reel_id`
- `cpl.reel_missing_track`
- `cpl.duplicate_track_type`
- `cpl.duplicate_asset_reference`

## Связи с другими объектами

- Содержит reel с явными asset reference.
- Может быть связан с `PKL` и `AssetMap` на следующих ветках, но `T01b` не выполняет полноценный resolver.
- Может быть связан с `KDMEnvelope` по `composition_id`.

## Каноническая сериализация

- JSON snake_case.
- Рациональные значения хранятся как `{numerator, denominator}`.
- Optional поля сериализуются только при наличии значения.

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- UUID, timestamp или edit rate не проходят нормализацию;
- reel list пуст;
- в reel нет ни одного поддерживаемого track reference;
- reel/track references дублируются в конфликтующей форме;
- сериализация или диагностика недетерминированы.

## Замечания по эволюции

- Обратимо-совместимые расширения добавляются новыми полями.
- Добавление resolver semantics, supplemental merge и timeline execution требует отдельных companion-specs и отдельных веток.
- Ломающие изменения требуют явного обновления `docs/PLAN.md`, `docs/STATUS.md` и связанных task-specific `AGENTS.md`.
