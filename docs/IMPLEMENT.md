# IMPLEMENT

## 1. Назначение

Этот документ фиксирует правила реализации и handoff для branch-sized режима.

## 2. Основной workflow

Для каждой ветки:

1. Прочитать `AGENTS.md`.
2. Прочитать `tasks/<TASK-ID>/AGENTS.md`.
3. Прочитать только `Required reading set`.
4. Сделать изменения только в разрешённой области.
5. Прогнать verification-команды.
6. Обновить `docs/STATUS.md`.
7. Обновить companion-specs, если менялись контракты.
8. Подготовить отчёт по `docs/templates/CODEX_REPORT.md`.

## 3. Durable memory

Проектная память не живёт в истории чата.
Она живёт в:
- `docs/PLAN.md`
- `docs/STATUS.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/*.md`
- task-specific `AGENTS.md`

## 4. Scope discipline

Запрещено:
- в одной ветке закрывать несколько несвязанных задач;
- тянуть новые зависимости без отражения в `docs/LIBRARIES.md`;
- менять trust boundary без ADR;
- смешивать picture FM и sound FM задачи без явного указания в task-specific AGENTS.

## 5. Verification policy

Минимально для scaffold и большинства задач:
- `./scripts/bootstrap.sh`
- `./scripts/build.sh`
- `./scripts/test.sh`

Если задача меняет smoke path, дополнительно:
- `./scripts/smoke.sh`

Если меняется repo layout:
- `python3 scripts/update_repo_maps.py`

## 6. Handoff policy

После завершения ветки обязательно обновить:
- `docs/STATUS.md`
- связанные companion-specs
- при необходимости `docs/PLAN.md`
- артефакты проверки в файловом виде

## 7. Stop conditions

Нужно остановиться и поднять вопрос архитекторам, если:
- требуется новая crypto/vendor dependency;
- меняется trust boundary;
- нужен новый subsystem за пределами scope;
- изменение инвариантов тянет много соседних модулей;
- secure path начинает тихо деградировать в open path.

## 8. Scaffold baseline

Текущий baseline уже должен:
- конфигурироваться и собираться;
- иметь хотя бы один реальный CTest;
- иметь stub CLI для smoke-проверок;
- иметь синхронизированные docs, specs и tasks.

Это не product implementation, а корректная стартовая площадка.

## 9. T01a diagnostics baseline

Для `T01a` зафиксирован минимальный детерминированный формат диагностик парсеров `AssetMap` и `PKL`:
- `code` — стабильный машинно-читаемый код;
- `severity` — `error` или `warning`;
- `path` — XPath-like путь до проблемного поля;
- `message` — стабильное человекочитаемое описание без platform-specific деталей.

Правила нормализации `T01a`:
- UUID принимает optional `urn:uuid:` prefix на входе и хранится в lower-case canonical form;
- `issue_date_utc` принимается в форме `YYYY-MM-DDTHH:MM:SSZ`;
- XML parser для `T01a` принимает UTF-8 и UTF-8 BOM;
- XML text/attribute normalization происходит только после полного entity decoding;
- поддерживаются named entities и numeric character references (`&#...;`, `&#x...;`);
- malformed XML entity не декодируется best-effort и поднимает детерминированный `xml_malformed` с path текущего XML context-а;
- path validation в `AssetMap` применяется к уже декодированному значению;
- digest в `PKL` принимает hex/base64 и нормализуется к lower-case hex;
- diagnostics сортируются детерминированно: `error` before `warning`, затем `code`, затем `path`.

## 10. T01b CPL baseline

Для `T01b` добавлен самостоятельный `CPL` parser/validator в `src/dcp/cpl` без перехода к resolver/timeline слоям.

Нормализованная модель `CPL` на этом этапе включает:
- composition-level metadata: `composition_id`, `content_title_text`, `annotation_text`, `issuer`, `creator`, `content_kind`, `issue_date_utc`, `edit_rate`;
- `namespace_uri` и `schema_flavor`;
- список `reels`;
- внутри каждого reel явные optional lane `picture`, `sound`, `subtitle`;
- внутри каждого asset reference: `asset_id`, `track_type`, optional `annotation_text`, `edit_rate`.

Детерминированные правила `T01b`:
- root parser принимает UTF-8 и UTF-8 BOM через тот же XML layer, что и `T01a`;
- UUID нормализуется так же, как в `AssetMap/PKL`: optional `urn:uuid:` prefix и lower-case canonical form;
- `IssueDate` нормализуется только в форме `YYYY-MM-DDTHH:MM:SSZ`;
- `EditRate` принимается только как две положительные integer-компоненты через whitespace;
- diagnostics используют тот же формат `code + severity + path + message`;
- diagnostics сортируются тем же правилом, что и в `T01a`;
- XML parse errors остаются отделены от domain validation errors: malformed XML получает `cpl.xml_malformed`, structural/domain defects получают отдельные `cpl.*` коды.

Минимальный набор инвариантов `T01b`:
- `CompositionPlaylist` root обязателен;
- `Id`, `ContentTitleText`, `Issuer`, `IssueDate`, `EditRate`, `ReelList` обязательны;
- `ReelList` не может быть пустым;
- каждый reel обязан иметь `Id` и хотя бы один поддерживаемый track reference;
- duplicate `reel_id`, duplicate track type внутри reel и duplicate `asset_id` внутри reel диагностируются явно;
- `T01b` не валидирует существование asset ID в `AssetMap/PKL`, не делает OV/VF resolving и не строит playback timeline.
