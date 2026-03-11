# dcplayer — `docs/STATUS.md`

## 1. Назначение

Этот документ — единственный оперативный источник правды о **фактическом** состоянии проекта.

- `docs/PLAN.md` отвечает на вопрос: **что и в каком порядке делать**;
- `docs/STATUS.md` отвечает на вопрос: **что реально сделано и что готово к следующей ветке**.

Рабочий корень проекта: `/Users/admin/Documents/dcplayer`.

## 2. Текущее project snapshot

Сейчас проект находится в состоянии **scaffold baseline + T01a/T01b готовы**.

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
- зафиксирован детерминированный формат диагностик `code + severity + path + message`;
- XML layer декодирует named/numeric entities до доменной validation и принимает UTF-8 BOM;
- добавлены valid/invalid DCP fixtures и unit tests для `assetmap`/`pkl`/`cpl`;
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
- strict XML entity decoding + deterministic malformed-entity diagnostics;
- UTF-8 BOM support for `AssetMap`/`PKL`/`CPL` ingest;
- DCP fixtures для positive/negative parser cases;
- unit tests `assetmap_parser_test`, `pkl_parser_test` и `cpl_parser_test`;
- канонические project docs;
- 34 companion-specs;
- 39 task-specific `AGENTS.md`.

### Что ещё не реализовано
- OV/VF resolver и composition graph;
- secure module;
- KDM validation;
- GPU decode;
- forensic watermark insertion;
- playout orchestrators;
- AES3 output;
- SMS/TMS logic.

## 4. Evidence baseline

Для веток `T01a` и `T01b` в этом handoff подтверждены следующие команды:

```bash
./scripts/bootstrap.sh
./scripts/build.sh
./scripts/test.sh
./scripts/test.sh -R 'assetmap|pkl'
./scripts/test.sh -R 'cpl'
```

Focused branch tests:
- `assetmap_parser_test`
- `pkl_parser_test`
- `cpl_parser_test`

## 5. Легенда статусов

- **DONE** — задача выполнена и подтверждена evidence.
- **READY** — все зависимости закрыты; можно открывать отдельную ветку.
- **BLOCKED** — задача определена, но ждёт завершения зависимостей.

## 6. Следующая очередь

Первыми готовыми к запуску ветками являются:
- `T01b` — CPL
- `T03a` — PKI
- `T05a` — J2K Backend
- `T06a` — Watermark Model

Рекомендуемый порядок старта:
1. `T01b`
2. `T03a`
3. `T05a`
4. `T06a`

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
| T03a | READY | Security control-plane baseline можно стартовать независимо от DCP parser. |
| T05a | READY | Decode abstraction можно проектировать на канонических specs и leaf-level CMake scaffold. |
| T06a | READY | Watermark contract можно фиксировать на текущих companion-specs. |
| T07a | BLOCKED | Ждёт `T02c`, потому что опирается на `PlaybackTimeline` в рабочем виде. |

## 9. Остальные ветки

Все остальные задачи из `docs/PLAN.md` считаются **BLOCKED** до закрытия своих зависимостей.
Полный список задач и их пути к task-specific `AGENTS.md` — в `tasks/INDEX.md`.

## 10. Правило обновления

Каждая ветка обязана обновить этот документ:
- изменить статус своей задачи;
- указать, что стало READY после закрытия зависимостей;
- кратко перечислить evidence;
- не поднимать статус соседних задач без отдельного подтверждения.
