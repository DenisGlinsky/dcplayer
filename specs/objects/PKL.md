# PKL

## Назначение

Каноническая модель Packing List / `PKL` для состава пакета, control-plane verify и контрольных сумм. Это второй ключевой контракт для `T01a`.

## Канонические поля

- `pkl_id` — UUID строка
- `annotation_text` — строка или `null`
- `issuer` — строка
- `creator` — строка или `null`
- `issue_date_utc` — UTC ISO-8601
- `namespace_uri` — строка
- `schema_flavor` — enum
- `assets` — массив asset entries

### Asset entry

- `asset_id` — UUID строка
- `annotation_text` — строка или `null`
- `asset_type` — enum `packing_list | composition_playlist | track_file | text_xml | unknown`
- `type_value` — исходное значение XML field `Type`
- `original_filename` — строка или `null`
- `size_bytes` — целое > 0
- `hash` — digest entry

### Digest entry

- `algorithm` — enum `sha1 | sha256 | unknown`
- `value` — hex/base64 строка как в исходном XML
- `normalized_hex` — lower-case hex строка или `null`

## Инварианты

- Каждый `asset_id` в `assets` уникален.
- Каждая запись asset обязана содержать digest.
- `size_bytes` должен быть положительным.
- `type_value` сохраняется как есть, а `asset_type` нормализуется детерминированным mapping-ом.
- Для известных алгоритмов должен существовать нормализованный digest.
- Ссылки на отсутствующие assets из `AssetMap` дают validation error.

## Namespace и версия

- Парсер обязан фиксировать `namespace_uri` и `schema_flavor` (`interop` / `smpte` / `unknown`).
- Неизвестный namespace допускается только как диагностируемое состояние, а не как silent fallback.
- Для `T01a` ingest поддерживает UTF-8 XML и UTF-8 XML с BOM.

## XML normalisation

- XML text и attribute values нормализуются только после полного entity decoding.
- Поддерживаются named entities `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;`.
- Поддерживаются decimal numeric references (`&#47;`, `&#65;`) и hex numeric references (`&#x2F;`, `&#x41;`).
- Malformed entity не декодируется best-effort; parser завершает разбор детерминированной `xml_malformed` diagnostic.

## Политика диагностики

Каждая ошибка должна иметь форму:
- `code`
- `severity`
- `path`
- `message`

Минимальные коды для `T01a`:
- `pkl.xml_malformed`
- `pkl.missing_required_field`
- `pkl.invalid_uuid`
- `pkl.invalid_time`
- `pkl.invalid_integer`
- `pkl.duplicate_asset_id`
- `pkl.invalid_digest_algorithm`
- `pkl.invalid_digest_value`
- `pkl.asset_missing_in_assetmap`
- `pkl.unsupported_namespace`

Для `T01a` path формируется в XPath-like виде, например:
- `/PackingList/Id`
- `/PackingList/AssetList/Asset[1]/Hash/@Algorithm`
- Для malformed XML / malformed entity используется детерминированный path текущего XML context-а.

## Связи с другими объектами

- Сопоставляется с `AssetMap` и `CPL`.
- Используется `dcp_probe` и `ingest verify`.
- Входит в `CompositionGraph`.

## Каноническая сериализация

- JSON `snake_case`.
- `digest_algorithm` и `digest_value` сериализуются явно.
- `urn:uuid:` префикс на входе допускается, но в нормализованной модели не сохраняется.
- `issue_date_utc` в `T01a` принимается в форме `YYYY-MM-DDTHH:MM:SSZ`.
- XML entity decoding происходит до digest/type/path-related normalization и до остальных доменных invariant checks.
- Для известных алгоритмов parser принимает hex или base64 на входе и нормализует к lower-case hex.
- Диагностика сортируется детерминированно по `severity` (`error` before `warning`), затем по `code`, затем по `path`.

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- нарушен инвариант;
- формат времени, enum или идентификатора невалиден;
- объект противоречит связанным контрактам;
- сериализация недетерминирована.

## Замечания по эволюции

- Обратимо-совместимые расширения добавляются новыми полями.
- Ломающие изменения требуют явного обновления `docs/PLAN.md`, `docs/STATUS.md` и связанных task-specific `AGENTS.md`.
- При изменении security semantics нужно дополнительно проверить trust boundary и audit model.
