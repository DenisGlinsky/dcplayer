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

## 13. T02c playback timeline baseline

Для `T02c` вводится отдельный dry-run builder в `src/playout/timeline`, который работает только поверх уже show-ready `CompositionGraph` и не лезет в decode/runtime/scheduler.

Вход `T02c`:
- один уже разрешённый `CompositionGraph`;
- никаких повторных OV/VF resolving, supplemental merge, KDM/security checks и runtime queue semantics.

Выход `T02c`:
- нормализованный `PlaybackTimeline`;
- детерминированные diagnostics формата `code + severity + path + message`;
- `validation_status`, вычисляемый по тому же правилу, что и в `T01a/T01b/T02a/T02b`.

Контракт `T02c`:
- timeline сохраняет `composition_id`, `origin_package_id`, `composition_kind`, `source_packages`;
- каждый `reel_entry` соответствует одному reel из `CompositionGraph.resolved_reels` и сохраняет `reel_id`;
- lane `picture`, `sound`, `subtitle` переносятся как optional machine-readable entries с provenance (`source_package_id`, `dependency_kind`, `resolved_path`, `asset_id`);
- `edit_rate` фиксируется на уровне `reel_entry` только при полном совпадении `edit_rate` всех присутствующих lane данного reel;
- если `CompositionGraph` содержит `missing_assets`, пустой `resolved_reels`, конфликт lane identity/edit_rate или нарушает собственные инварианты provenance, builder не публикует частичный timeline.

Диагностика `T02c`:
- `timeline.graph_not_show_ready` — graph содержит unresolved assets и потому не пригоден для dry-run timeline;
- `timeline.empty_reel_list` — graph не содержит ни одного resolved reel;
- `timeline.lane_type_mismatch` — `TrackFile.track_type` не совпадает с lane, в который он положен;
- `timeline.composition_kind_dependency_mismatch` — `composition_kind` противоречит фактическому lane provenance / `dependency_kind`;
- `timeline.invalid_edit_rate` — lane содержит неположительный `edit_rate`;
- `timeline.entry_edit_rate_mismatch` — в одном reel присутствуют lane с разным `edit_rate`;
- `timeline.unsupported_graph_shape` — нарушены остальные ожидаемые shape/provenance invariants (`origin_package_id`, `source_packages`, reel/lane identity, пустые обязательные поля).

Правило precedence `T02c`:
- если lane уже получил более специфичную ошибку `timeline.invalid_edit_rate`, builder не добавляет вторичный generic `timeline.unsupported_graph_shape` только потому, что reel не смог дать entry-level `edit_rate`;
- global `timeline.composition_kind_dependency_mismatch` не добавляется поверх уже достаточного первичного дефекта вроде `timeline.empty_reel_list` или single-lane `timeline.invalid_edit_rate`;
- `timeline.unsupported_graph_shape` остаётся только для остальных shape/provenance нарушений, не покрытых более специфичными кодами.

Сознательные ограничения `T02c`:
- timeline остаётся reel-level, а не frame/runtime-level;

## 14. T03a PKI baseline

Для `T03a` введён локальный baseline PKI/trust-store слой в `src/security_api/certs` без выхода в transport и без реальной crypto-интеграции с TPM/ZymKey.

Вход `T03a`:
- in-memory import metadata store (`store_id`, `store_version`, `updated_at_utc`);
- import trust anchors, intermediates и device certificates;
- optional import-only `private_key_material`, который обязан быть пустым;
- `revocation_sources`;
- `subject_fingerprint`, `validation_time_utc`, optional `checked_sources`, in-memory `revocation_entries` и `revocation_policy`.

Выход `T03a`:
- нормализованный `CertificateStore`;
- `TrustChain` result model с `decision`, `decision_reason`, `revocation_status` и `checked_sources`;
- детерминированные diagnostics того же формата `code + severity + path + message`;
- `validation_status`, вычисляемый тем же правилом, что и в предыдущих validation-ветках.

Правила `T03a`:
- `fingerprint_algorithm` фиксирован как `sha256`;
- fingerprint нормализуется к lower-case hex и валидируется как 64-char SHA-256;
- `roots` принимают только `role=root`, `intermediates` только `role=intermediate`, `device_certs` только `role=server|client|signer`;
- `issuer_role_hint` остаётся optional metadata-only полем и в `T03a` используется только для deterministic missing-issuer classification (`root|intermediate|unknown`);
- root certificate baseline должен быть self-issued;
- `revocation_sources` нормализуются в unique + lexicographically sorted order;
- rootless store может существовать как object model; для `T03a` `trust_chain.anchor_missing` гарантирован только для missing issuer после intermediate и для direct device certificate с `issuer_role_hint=root`, а self-issued non-root и остальные unresolved issuer links остаются `trust_chain.chain_broken`;
- chain evaluation в `T03a` является metadata-based: она не делает ASN.1 parsing, signature verification, OCSP/CRL network fetch и не использует secure clock.

Revocation semantics `T03a`:
- configured authority set задаётся через `CertificateStore.revocation_sources`;
- если request задаёт `checked_sources`, authoritative subset = `revocation_sources ∩ checked_sources`;
- revocation entry учитывается только по паре `(fingerprint, source)` из authoritative subset;
- source вне configured/checked authority set даёт `trust_chain.unknown_revocation_source` как наблюдаемый warning diagnostic и не считается валидным evidence;
- `require_good` — missing/unknown/not_checked revocation state приводит к `trust_chain.revocation_unavailable` c `severity=error` и `decision=rejected`;
- `allow_unknown` — тот же сценарий, включая non-authoritative revocation entries, приводит к `severity=warning` и `decision=warning_only`;
- `revoked` всегда fail-closed и даёт `trust_chain.revoked`;
- invalid revocation entry contract даёт `trust_chain.invalid_revocation_state`; при этом parser сначала собирает все наблюдаемые `unknown_revocation_source` diagnostics из request, а затем завершает evaluation fail-closed;
- malformed request `validation_time_utc` останавливает evaluation до trust decision с `trust_chain.invalid_validation_time`, `decision_reason=invalid_validation_time` и non-success `revocation_status`.

Детерминизм `T03a`:
- diagnostics сортируются правилом `error` before `warning`, затем `code`, затем `path`, затем `message`;
- valid fixtures проверяют канонический JSON для store/result;
- invalid fixtures проверяют канонический JSON diagnostics.
- duration не сериализуется, потому что текущий `CompositionGraph` ещё не содержит duration metadata;
- dry-run builder не занимается wall-clock scheduling, device output, real-time sync, subtitle rendering и audio routing runtime.
