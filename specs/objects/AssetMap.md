# AssetMap

## Назначение

Каноническая модель `ASSETMAP` как входной таблицы активов DCP. Это первый объектный контракт для ingest/verify и для `T01a`.

## Канонические поля

- `asset_map_id` — UUID строка
- `issuer` — строка
- `issue_date_utc` — UTC ISO-8601
- `creator` — строка или `null`
- `namespace_uri` — строка
- `schema_flavor` — enum `interop | smpte | unknown`
- `volume_count` — целое >= 1
- `assets` — массив записей asset

### Asset entry

- `asset_id` — UUID строка
- `chunk_list` — массив chunk entries
- `packing_list_hint` — bool, нормализуется из optional `PackingList` XML field
- `is_text_xml` — bool, нормализуется по расширению первого chunk path (`*.xml`)

### Chunk entry

- `path` — относительный POSIX-style путь
- `offset` — целое >= 0
- `length` — целое > 0
- `volume_index` — целое >= 1

## Инварианты

- `asset_map_id` уникален в пределах пакета.
- Каждый `asset_id` уникален в пределах `assets`.
- Каждый asset имеет как минимум один chunk.
- Все `path` нормализованы, относительны и не выходят выше корня ingest (`..` запрещён).
- `.` и пустые path-segments при нормализации удаляются; backslash и absolute path запрещены.
- Внутри одного asset сочетание `volume_index + path + offset` не должно дублироваться.
- `volume_index` каждого chunk не может превышать `volume_count`.
- `chunk_list` сериализуется в исходном порядке носителей; сортировка “для красоты” запрещена.

## Namespace и версия

- Парсер обязан фиксировать `namespace_uri` и `schema_flavor` (`interop` / `smpte` / `unknown`).
- Неизвестный namespace не запрещает ingest автоматически, но даёт детерминированный warning-class diagnostic.
- Для `T01a` ingest поддерживает UTF-8 XML и UTF-8 XML с BOM.

## XML normalisation

- XML text и attribute values нормализуются только после полного entity decoding.
- Поддерживаются named entities `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;`.
- Поддерживаются decimal numeric references (`&#47;`, `&#65;`) и hex numeric references (`&#x2F;`, `&#x41;`).
- Malformed entity не декодируется best-effort; parser завершает разбор детерминированной `xml_malformed` diagnostic.
- `path` validation применяется к уже декодированному значению, поэтому `..&#47;...` и `..&#x2F;...` отклоняются тем же инвариантом, что и `../...`.

## Политика диагностики

Каждая ошибка должна иметь форму:
- `code` — стабильный строковый код
- `severity` — `error` / `warning`
- `path` — JSON pointer или XPath-like location
- `message` — человекочитаемое описание

Минимальные коды для `T01a`:
- `assetmap.xml_malformed`
- `assetmap.missing_required_field`
- `assetmap.invalid_uuid`
- `assetmap.invalid_time`
- `assetmap.invalid_integer`
- `assetmap.duplicate_asset_id`
- `assetmap.duplicate_chunk_entry`
- `assetmap.invalid_relative_path`
- `assetmap.empty_chunk_list`
- `assetmap.unsupported_namespace`

Для `T01a` path формируется в XPath-like виде, например:
- `/AssetMap/Id`
- `/AssetMap/AssetList/Asset[2]/ChunkList/Chunk[1]/Path`
- Для malformed XML / malformed entity используется детерминированный path текущего XML context-а, например `/AssetMap/AssetList/Asset/ChunkList/Chunk/Path`.

## Связи с другими объектами

- Используется парсером `T01a`.
- Сопоставляется с `PKL` и `CPL`.
- Формирует вход в `CompositionGraph`.

## Каноническая сериализация

- JSON `snake_case`.
- UUID всегда lower-case canonical form.
- `urn:uuid:` префикс на входе допускается, но в нормализованной модели не сохраняется.
- Пути сериализуются как относительные POSIX-style строки без завершающего `/`.
- `issue_date_utc` в `T01a` принимается в форме `YYYY-MM-DDTHH:MM:SSZ`.
- XML entity decoding происходит до path normalization и до остальных доменных invariant checks.
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
