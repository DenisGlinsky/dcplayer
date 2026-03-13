# dcplayer — `docs/STATUS.md`

## 1. Назначение

Этот документ — единственный оперативный источник правды о **фактическом** состоянии проекта.

- `docs/PLAN.md` отвечает на вопрос: **что и в каком порядке делать**;
- `docs/STATUS.md` отвечает на вопрос: **что реально сделано и что готово к следующей ветке**.

Рабочий корень проекта: `/Users/admin/Documents/dcplayer`.

## 2. Текущее project snapshot

Сейчас проект находится в состоянии **scaffold baseline + T01a/T01b/T02a/T02b/T02c/T03b/T03c/T03d/T03e готовы; T03a desync в текущем worktree**.

Подтверждено:
- репозиторий очищен от `build/`, `__MACOSX`, `.DS_Store` и подобных артефактов;
- docs, tasks и specs синхронизированы по именам и ссылкам;
- task names сокращены до branch-sized формата `TXXx-одно-или-два-слова`;
- канонический набор companion-specs существует и перечислен в `docs/PLAN.md`;
- task-specific `AGENTS.md` существуют для всех 39 branch-sized задач;
- build-system включает subsystem-level CMake scaffold и leaf-level CMake в разрешённых task-поверхностях;
- CMake-scaffold конфигурируется;
- stub-приложения собираются;
- CTest содержит реальные unit и integration smoke tests и проходит;
- реализованы `AssetMap` и `PKL` parser/validator с нормализованными моделями;
- реализован `CPL` parser/validator с нормализованной моделью composition/reel/track references;
- реализован OV/VF resolver с нормализованным `CompositionGraph`;
- реализован supplemental merge/override поверх нормализованного `CompositionGraph`;
- реализован dry-run `PlaybackTimeline` builder поверх show-ready `CompositionGraph`;
- owner selection для OV/VF resolver учитывает только PKL-backed CPL candidates;
- resolver различает local/external dependencies на уровне итогового graph;
- deterministic diagnostics покрывают missing asset id, broken reference, conflicting resolution и broken dependency;
- deterministic supplemental diagnostics покрывают target not found, unsupported merge mode, base edit-rate mismatch, broken base dependency, conflicting override и asset without valid backing;
- deterministic timeline diagnostics покрывают not show-ready graph, empty reel list, lane type mismatch, composition_kind/dependency mismatch, invalid edit rate, entry edit-rate mismatch и unsupported graph shape с корректным precedence для global mismatch;
- зафиксирован детерминированный формат диагностик `code + severity + path + message`;
- XML layer декодирует named/numeric entities до доменной validation и принимает UTF-8 BOM;
- зафиксирован spec-level PKI result model для `CertificateStore` / `TrustChain`, который используется как trust input для secure control plane;
- реализован `secure_channel` contract/model слой без real transport;
- реализованы machine-readable `SecureChannelContract`, request/response `ProtectedApiEnvelope` и `SecurityModuleContract`;
- реализованы metadata-level identity checks, trust binding и ACL baseline для `pi_zymkey <-> ubuntu_tpm`;
- реализованы strict ACL/API semantics поверх baseline envelope-а: API-specific request/response body schemas для `sign`, `unwrap`, `decrypt`, `health`, `identity`;
- response validation теперь покрывает request/response family matching, status/body combinations и request-id consistency без transport/runtime side effects;
- response parse/validate split зафиксирован: parse stage принимает syntactically valid response envelope с корректной response family/body schema, а status/body reject остаётся в semantic validation;
- response validation соблюдает precedence: `invalid_status_body_combination` остаётся первичным кодом и не получает лишний `request_response_api_mismatch` поверх того же дефекта;
- deterministic diagnostics покрывают invalid server/client role, role mismatch, invalid peer identity, missing trust binding, untrusted peer, unauthorized API, invalid request/response body, missing required field, unexpected field, request/response api mismatch, invalid status/body combination и response request-id mismatch;
- реализованы `SecurityEvent` и `AuditExport` object models с deterministic canonical JSON serialization;
- реализованы baseline event families `auth/trust/acl/api/tamper/recovery/export` и model-level tamper/recovery/export semantics;
- deterministic diagnostics покрывают invalid event_type, invalid severity, malformed event_time/export_time, duplicate event_id, export ordering mismatch, invalid event/export linkage, invalid tamper transition и missing required audit field;
- corrective patch усилил direct object-level validate path: `validate_audit_export()` теперь сохраняет error status и не обходит nested event-level invariants;
- corrective patch устранил parse/direct diagnostic drift для invalid-present nested audit fields: `build_security_event()` больше не наслаивает ложный `audit.missing_required_field` поверх специфичного nested diagnostic;
- реализованы `SecureClockPolicy` и `SecureTimeStatus` с deterministic canonical JSON serialization, strict model-level gating semantics, `\uXXXX` round-trip в parser и стабильной parse boundary для required time fields;
- реализованы baseline source kinds `rtc_secure | rtc_untrusted | imported_secure_time | operator_set_time_placeholder | unknown`;
- deterministic diagnostics покрывают invalid time format/source, missing required time field, untrusted source, stale time, skew exceeded, non-monotonic time, policy-disallowed source и secure time unavailable;
- host-side baseline `spb1` contract теперь публикует secure clock policy и baseline список secure-time-gated APIs;
- `docs/IMPLEMENT.md` синхронизирован с публичным `T03b/T03c` diagnostic surface, включая ACL/API body validation и baseline invalid ref diagnostics;
- `docs/IMPLEMENT.md` синхронизирован с публичным `T03d` audit/tamper diagnostic surface;
- `docs/IMPLEMENT.md` синхронизирован с публичным `T03e` secure-time diagnostic surface и gating semantics;
- добавлены valid/invalid secure channel fixtures для baseline APIs и branch-focused unit tests;
- добавлены valid/invalid audit fixtures и branch-focused unit tests для `SecurityEvent`/`AuditExport`;
- добавлены valid/invalid secure time fixtures и branch-focused unit tests для `SecureClockPolicy`/`SecureTimeStatus`;
- добавлены valid/invalid DCP fixtures и unit tests для `assetmap`/`pkl`/`cpl`;
- добавлены valid/invalid OV/VF fixtures и unit/integration tests для resolver-а;
- добавлены valid/invalid supplemental fixtures и unit/integration tests для merge layer;
- добавлены valid/invalid playback timeline fixtures и unit/integration tests для dry-run builder-а;
- `TREE.txt` и `manifest.json` регенерируются скриптом.

## 3. Честный baseline

### Что реально реализовано
- CMake scaffold;
- CMake presets с раздельными `binaryDir`;
- bootstrap / build / test / smoke / format / repo-map scripts;
- библиотека `dcplayer_core`;
- subsystem-level interface libraries и leaf-level CMake scaffold;
- stub-исполняемые файлы:
  - `dcp_probe`
  - `playout_service`
  - `sms_ui`
  - `tms_web`
- минимальные unit/integration smoke tests;
- `AssetMap` parser + validation;
- `PKL` parser + validation;
- `CPL` parser + validation;
- cross-validation `PKL -> AssetMap`;
- `OV/VF` resolver + `CompositionGraph`;
- `supplemental` merge layer + `SupplementalMergePolicy`;
- `PlaybackTimeline` dry-run builder + canonical JSON serialization;
- spec-level `CertificateStore` / `TrustChain` PKI result-model baseline;
- `secure_channel` object model, canonical JSON serialization, strict ACL matrix и authority checks;
- `audit` object model, canonical JSON serialization, export validation и tamper/recovery state validation;
- `secure_time` object model, canonical JSON serialization с корректным string escaping, parser support для `\uXXXX`, reject raw control chars в JSON string, direct policy/status validation и fail-closed/fail-open gate semantics без ослабления `secure_time_status != trusted`;
- `ProtectedApiEnvelope` request/response validation с API-specific body schemas и family matching;
- `SecurityModuleContract` baseline для `pi_zymkey`;
- baseline host contract factory для `spb1`, включая secure clock policy и secure-time-gated API list;
- regression coverage для `secure_channel.duplicate_revocation_status` и `secure_channel.duplicate_supported_api_name`;
- regression coverage для `secure_channel.missing_required_field`, `secure_channel.unexpected_field`, `secure_channel.invalid_request_body`, `secure_channel.invalid_response_body`, `secure_channel.request_response_api_mismatch` и `secure_channel.invalid_status_body_combination`;
- regression coverage для `audit.invalid_event_type`, `audit.invalid_severity`, `audit.invalid_event_time`, `audit.duplicate_event_id`, `audit.export_ordering_mismatch`, `audit.invalid_event_export_linkage`, `audit.invalid_tamper_transition` и `audit.missing_required_field`;
- regression coverage для direct `validate_audit_export()` теперь отдельно проверяет missing `request_id`, missing `export_id_ref`, malformed `event_time_utc`, invalid enum values и стабильный non-ok status на error-level diagnostics;
- regression coverage теперь отдельно сравнивает parse-path и direct validation для invalid nested `actor_identity`, `decision_summary` и `result_summary`;
- backed owner selection для целевого `CPL`;
- deterministic OV/VF diagnostics и dependency classification;
- deterministic supplemental diagnostics, policy validation и multi-policy conflict handling;
- deterministic timeline diagnostics, precedence for invalid edit-rate fallback/global mismatch и machine-readable timeline dump;
- strict XML entity decoding + deterministic malformed-entity diagnostics;
- UTF-8 BOM support for `AssetMap`/`PKL`/`CPL` ingest;
- DCP fixtures для positive/negative parser cases;
- playback timeline fixtures для positive/negative dry-run cases;
- unit tests `assetmap_parser_test`, `pkl_parser_test` и `cpl_parser_test`;
- unit test `ov_vf_resolver_unit_test`;
- integration test `ov_vf_resolver_integration_test`;
- unit test `supplemental_merge_unit_test`;
- integration test `supplemental_merge_integration_test`;
- unit test `playback_timeline_unit_test`;
- integration test `playback_timeline_integration_test`;
- unit tests `secure_channel_contract_unit_test` и `spb1_protected_api_unit_test`;
- unit tests `secure_clock_policy_unit_test` и `secure_time_gate_unit_test`;
- канонические project docs;
- 34 companion-specs;
- 39 task-specific `AGENTS.md`.

### Что ещё не реализовано
- secure module;
- `src/security_api/certs/**` и `tests/unit/security/pki/**` остаются scaffold-only в текущем дереве;
- real mutual-TLS transport;
- TPM/ZymKey device integration;
- runtime secure clock / RTC device access / live time synchronization;
- runtime security logs, log transport и encrypted audit storage;
- KDM validation;
- GPU decode;
- forensic watermark insertion;
- playout orchestrators;
- AES3 output;
- SMS/TMS logic.

## 4. Evidence baseline

Для веток `T01a`, `T01b`, `T02a`, `T02b`, `T02c`, `T03b`, `T03c`, `T03d` и `T03e` в этом handoff подтверждены следующие команды:

```bash
./scripts/bootstrap.sh
./scripts/build.sh
./scripts/test.sh
./scripts/test.sh -R 'assetmap|pkl'
./scripts/test.sh -R 'cpl'
./scripts/test.sh -R 'ov|vf'
./scripts/test.sh -R 'supplemental'
./scripts/test.sh -R 'timeline'
./scripts/test.sh -R 'secure|channel|spb1'
./scripts/test.sh -R 'audit|tamper|event|export'
./scripts/test.sh -R 'time|clock|secure_time'
```

Focused branch tests:
- `assetmap_parser_test`
- `pkl_parser_test`
- `cpl_parser_test`
- `ov_vf_resolver_unit_test`
- `ov_vf_resolver_integration_test`
- `supplemental_merge_unit_test`
- `supplemental_merge_integration_test`
- `playback_timeline_unit_test`
- `playback_timeline_integration_test`
- `secure_channel_contract_unit_test`
- `spb1_protected_api_unit_test`
- `audit_event_unit_test`
- `audit_export_tamper_unit_test`
- `secure_clock_policy_unit_test`
- `secure_time_gate_unit_test`

## 5. Легенда статусов

- **DONE** — задача выполнена и подтверждена evidence.
- **READY** — все зависимости закрыты; можно открывать отдельную ветку.
- **BLOCKED** — задача определена, но ждёт завершения зависимостей.
- **DESYNC** — companion-spec или docs существуют, но текущий worktree не подтверждает заявленную реализацию/evidence.

## 6. Следующая очередь

Первыми готовыми к запуску ветками являются:
- `T04a` — Security Module
- `T05a` — J2K Backend
- `T06a` — Watermark Model
- `T07a` — Audio Sync

Рекомендуемый порядок старта:
1. `T04a`
2. `T05a`
3. `T06a`
4. `T07a`

## 7. Фаза A — Foundation

| Task | Статус | Комментарий |
|---|---|---|
| T00a | DONE | Scaffold репозитория поднят, очищен и проверен. |
| T00b | DONE | Канонический baseline companion-specs зафиксирован и усилен для первых READY-веток. |
| T00c | DONE | Durable-memory, task naming discipline и branch-sized workflow синхронизированы. |

## 8. Готовые к старту ветки

| Task | Статус | Почему |
|---|---|---|
| T01a | DONE | `AssetMap`/`PKL` parse+validation, strict XML decoding/BOM support, fixtures и unit tests реализованы и проверены. |
| T01b | DONE | `CPL` parse+validation, deterministic diagnostics, fixtures и unit tests реализованы и проверены. |
| T02a | DONE | `OV/VF` resolver, `CompositionGraph`, deterministic diagnostics, valid/invalid fixtures и unit/integration tests реализованы и проверены. |
| T02b | DONE | Реализованы supplemental merge/override, детерминированные diagnostics, valid/invalid fixtures и branch-focused tests. |
| T02c | DONE | Реализованы dry-run `PlaybackTimeline`, machine-readable JSON dump, deterministic diagnostics, invariant-check для `composition_kind/dependency_kind`, precedence для `invalid_edit_rate` и global mismatch, valid/invalid fixtures и branch-focused unit/integration tests. |
| T03a | DESYNC | `CertificateStore`/`TrustChain` specs присутствуют и используются `T03b`, но `src/security_api/certs/**` и `tests/unit/security/pki/**` в текущем worktree остаются scaffold-only, поэтому `DONE` здесь не подтверждён. |
| T03b | DONE | Реализованы secure channel contract, protected request/response envelopes, metadata-level identity/trust checks, ACL baseline, fixtures и branch-focused unit tests. |
| T03c | DONE | Реализованы strict ACL/API semantics поверх `T03b`: baseline API body schemas, deterministic request/response family validation, invalid status/body handling, valid/invalid fixtures и branch-focused unit tests. |
| T03d | DONE | Реализованы `SecurityEvent`/`AuditExport`, deterministic diagnostics, tamper/recovery/export model semantics, valid/invalid fixtures и branch-focused unit tests без runtime logging/transport. |
| T03e | DONE | Реализованы `SecureClockPolicy`/`SecureTimeStatus`, deterministic time diagnostics, fail-closed/fail-open gating semantics, valid/invalid fixtures и branch-focused unit tests без RTC/sync/runtime integration. |
| T04a | READY | `SecureChannelContract`, `ProtectedApiEnvelope` и `SecureClockPolicy` теперь зафиксированы; можно строить host-side security module и simulator поверх этих contract-слоёв. |
| T05a | READY | Decode abstraction можно проектировать на канонических specs и leaf-level CMake scaffold. |
| T06a | READY | Watermark contract можно фиксировать на текущих companion-specs. |
| T07a | READY | `PlaybackTimeline` готов; можно стартовать sync-ветку поверх dry-run timeline контракта. |

## 9. Остальные ветки

Все остальные задачи из `docs/PLAN.md` считаются **BLOCKED** до закрытия своих зависимостей.
Полный список задач и их пути к task-specific `AGENTS.md` — в `tasks/INDEX.md`.

## 10. Правило обновления

Каждая ветка обязана обновить этот документ:
- изменить статус своей задачи;
- указать, что стало READY после закрытия зависимостей;
- кратко перечислить evidence;
- не поднимать статус соседних задач без отдельного подтверждения.
