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

## 11. T02a OV/VF resolver baseline

Для `T02a` вводится отдельный resolver-слой в `src/dcp/ov_vf_resolver`, который работает только на уже разобранных и валидированных моделях `AssetMap`, `PKL` и `CPL`.

Вход resolver-а:
- набор пакетов, где каждый пакет состоит из нормализованных `AssetMap`, `PKL` и нуля или более `CPL`;
- `composition_id` целевой композиции.

Выход resolver-а:
- `CompositionGraph` с нормализованными `Reel` и `TrackFile`;
- детерминированный набор диагностик того же формата `code + severity + path + message`;
- `validation_status`, вычисляемый тем же правилом, что и в `T01a/T01b`.

Поведение результата `T02a`:
- при `validation_status = ok | warning` resolver возвращает `CompositionGraph`;
- при `validation_status = error` resolver возвращает только диагностику и не публикует частичный graph как show-ready результат.

Правила разрешения `T02a`:
- владелец целевого `CPL` определяется только по backed owner candidate: в пакете должен существовать parsed `CPL` с нужным `composition_id`, `PKL` asset типа `composition_playlist` с тем же id и корректный `AssetMap` backing этого asset;
- stray/unbacked parsed `CPL` без такого `PKL`/`AssetMap` backing не участвует в owner selection и не создаёт ложный `ov_vf.conflicting_composition_owner`;
- для каждого `CPL` asset reference resolver сначала пытается разрешить asset в owning package;
- если owning package не содержит такой `track_file`, resolver ищет backing asset во внешних пакетах и таким образом фиксирует OV/VF dependency;
- resolver не делает supplemental merge, playback timeline execution, disk existence checks и security/KDM semantics;
- итоговый `composition_kind` равен `vf`, если хотя бы один track разрешён из внешнего пакета; иначе `ov`.

Минимальные диагностические коды `T02a`:
- `ov_vf.target_composition_not_found`
- `ov_vf.conflicting_composition_owner`
- `ov_vf.missing_asset_id`
- `ov_vf.broken_reference`
- `ov_vf.conflicting_asset_resolution`
- `ov_vf.broken_dependency`

Смысл кодов `T02a`:
- `missing_asset_id` — для asset reference, у которого нет ни одной backing `PKL` entry ни в одном пакете;
- `broken_reference` — `PKL` entry существует, но не приводит к валидному `AssetMap` backing asset в ожидаемом пакете разрешения;
- `conflicting_asset_resolution` — один и тот же внешний asset reference имеет более одного валидного кандидата;
- `broken_dependency` — хотя бы одна внешняя OV/VF dependency не была разрешена до пригодного к показу `TrackFile`.

Детерминизм `T02a`:
- diagnostics сортируются тем же правилом: `error` before `warning`, затем `code`, затем `path`;
- `source_packages` и `missing_assets` в `CompositionGraph` стабильно сортируются лексикографически;
- `resolved_reels` сохраняют порядок исходного `CPL`.

## 12. T02b supplemental merge baseline

Для `T02b` вводится отдельный слой `src/dcp/supplemental`, который работает поверх уже show-ready `CompositionGraph` из `T02a` и не меняет код runtime/timeline.

Вход слоя `T02b`:
- `CompositionGraph`, уже собранный `ov_vf_resolver`-ом;
- набор пакетов из тех же нормализованных `AssetMap`, `PKL`, `CPL`;
- один или более `SupplementalMergePolicy`.

Выход слоя `T02b`:
- нормализованный `CompositionGraph` после применения допустимых supplemental override;
- детерминированный набор диагностик формата `code + severity + path + message`;
- `validation_status`, вычисляемый тем же правилом, что и в `T01a/T01b/T02a`.

Правила применения `T02b`:
- base graph не перестраивается заново и не исполняется; merge идёт только поверх уже разрешённых `resolved_reels`;
- supplemental composition выбирается только по backed owner candidate: parsed `CPL` + `PKL` asset типа `composition_playlist` + валидный `AssetMap` backing;
- policy сортируются стабильно по `(base_composition_id, supplemental_composition_id, policy_id)`, чтобы multi-policy merge не зависел от порядка входного массива;
- supplemental reel сопоставляется с base graph по `reel_id`;
- если supplemental lane отсутствует, lane наследуется из base graph без изменений;
- если supplemental reference имеет валидный local `track_file` backing и lane разрешён в `allowed_overrides`, base lane заменяется этим `TrackFile`;
- если local backing отсутствует, reference считается допустимой base dependency только при точном совпадении `asset_id`, lane identity и `edit_rate` с уже существующим lane того же reel в base graph;
- supplemental не может добавлять новые reel или новые lane вне base graph;
- при ошибке merge не публикует частичный graph как show-ready результат.

Минимальные диагностические коды `T02b`:
- `supplemental.base_composition_mismatch`
- `supplemental.unsupported_merge_mode`
- `supplemental.target_composition_not_found`
- `supplemental.conflicting_composition_owner`
- `supplemental.invalid_allowed_override`
- `supplemental.duplicate_allowed_override`
- `supplemental.override_not_allowed`
- `supplemental.asset_without_valid_backing`
- `supplemental.base_edit_rate_mismatch`
- `supplemental.broken_base_dependency`
- `supplemental.conflicting_override`

Смысл кодов `T02b`:
- `base_composition_mismatch` — policy ссылается не на тот base graph, который подан на вход merge;
- `unsupported_merge_mode` — `merge_mode` не входит в поддерживаемый реализацией поднабор;
- `target_composition_not_found` — supplemental composition не найден среди backed owner candidates;
- `conflicting_composition_owner` — один и тот же supplemental composition backed более чем одним пакетом;
- `invalid_allowed_override` — `allowed_overrides` содержит невалидное enum-значение lane;
- `duplicate_allowed_override` — `allowed_overrides` содержит повтор одного и того же lane;
- `override_not_allowed` — policy запрещает override данного lane;
- `asset_without_valid_backing` — supplemental reference локально указывает на невалидный `PKL/AssetMap` backing;
- `base_edit_rate_mismatch` — explicit base dependency совпадает по `asset_id`, но расходится по `edit_rate` с base graph;
- `broken_base_dependency` — supplemental reference не совпадает с допустимым lane в base graph или ссылается на reel/lane, которого нет в base graph;
- `conflicting_override` — разные policy дают разные resolved tracks для одного и того же `(reel_id, lane)`.

Детерминизм `T02b`:
- diagnostics сортируются тем же правилом: `error` before `warning`, затем `code`, затем `path`;
- итоговый `source_packages` пересчитывается по фактически используемым `TrackFile`;
- итоговый `composition_kind` пересчитывается от финальных `TrackFile` относительно `origin_package_id`.
