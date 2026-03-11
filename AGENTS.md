# AGENTS.md

Репозиторий: `~/Documents/dcplayer`

## Language and communication

- Default language for all interaction with the project owner is Russian.
- All plans, progress updates, milestone closeout summaries, risk notes, and explanatory responses must be written in Russian.
- Do not switch to English just because referenced libraries, APIs, or standards are in English.
- Preserve the existing language of files you edit unless the task explicitly requires translation.
- Code identifiers, filenames, module names, API names, protocol field names, and test names should remain in the language already used by the codebase, typically English.
- When a response was accidentally produced in the wrong language, restate it in Russian in the next reply before continuing.

Проект ведётся по модели **архитектурный штаб + Codex-исполнитель**.
Архитекторы фиксируют архитектуру, trust boundary, companion-specs и критерии приёмки.
Codex делает только узкие branch-sized срезы и отчитывается по шаблону.

## Порядок чтения перед началом работы

1. Прочитай этот файл.
2. Прочитай `tasks/<TASK-ID>/AGENTS.md` для текущей ветки.
3. Прочитай **только** `Required reading set`, перечисленный в task-specific `AGENTS.md`.

Инструкция “прочитай весь репозиторий” запрещена.
Источник истины по readset — только task-specific `AGENTS.md`.

## Неподвижные архитектурные правила

1. Эталонный secure show path:
   `local storage -> HSM decrypt service (Raspberry Pi + ZymKey + RTC) -> GPU decode -> GPU picture FM -> playout`.
2. Для звука действует тот же secure show context:
   `local storage -> secure authorization -> decrypt -> audio FM -> PCM engine -> later AES3`.
3. Управляющий канал между media server и secure-plane строится как mutual-TLS:
   - Raspberry Pi + ZymKey = TLS server;
   - Ubuntu media server + TPM 2.0 = TLS client;
   - серверный приватный ключ остаётся внутри ZymKey;
   - клиентский приватный ключ остаётся внутри TPM;
   - проверка цепочки, revocation, ACL и аудит обязательны.
4. Любой performance path обязан иметь fallback path.
5. Приоритет в cinema-critical коде:
   `корректность -> отказоустойчивость -> производительность`.
6. Никаких заявлений “DCI compliant” без отдельной матрицы соответствия и подтверждённых испытаний.
7. В репозиторий нельзя коммитить реальные KDM, приватные ключи, сертификаты, escrow-материалы и plaintext essence.

## Что значит pluggable forensic watermark

В этом проекте **pluggable forensic watermark** — это стабильный внутренний контракт watermark-подсистемы.
Разрешено менять backend внедрения или детекции, но запрещено менять:
- payload model;
- state machine;
- security-log semantics;
- operator-facing control;
- attack-matrix semantics
без обновления companion-specs и handoff.

Минимальный baseline:
- единый интерфейс backend-а;
- CPU reference path;
- GPU production path;
- detector harness;
- единый self-test;
- единый attack-matrix и report format.

## Глобальные правила работы

- Имена task-директорий должны иметь формат `TXXx-слово` или `TXXx-слово-слово`; длинные многочастные имена запрещены.
- Одна ветка = одна инженерная цель, один основной subsystem, одна поверхность приёмки.
- Никаких тихих рефакторингов вне scope ветки.
- Любое изменение контракта или state machine требует обновления `specs/objects/*`.
- Любое изменение trust boundary требует ADR.
- Сначала simulator/fixtures, потом реальное железо.
- Длинные логи и отчёты — в файлы/артефакты, а не в чат.

## Формат отчёта

Используй шаблон `docs/templates/CODEX_REPORT.md`.

Отчёт обязан содержать:
- что сделано;
- какие файлы изменены;
- чем проверено;
- что не покрыто;
- какие риски остались;
- какие зависимости/лицензии изменились.

## Разрешённая область изменений

Допустимо менять:
- `apps/**`
- `src/**`
- `spb1/**`
- `docs/**`
- `specs/**`
- `tests/**`
- `scripts/**`
- `CMakeLists.txt`
- `CMakePresets.json`
- `README.md`
- `AGENTS.md`
- `.clang-format`
- `.clang-tidy`
- `.codex/config.toml`
- `manifest.json`
- `TREE.txt`

Запрещено коммитить:
- `*.pem`, `*.key`, `*.p12`, `*.pfx`, `*.csr`, `*.crt`
- реальные KDM/сертификаты
- plaintext essence
- core dumps
- логи с чувствительными данными

## Базовые команды

```bash
./scripts/bootstrap.sh
./scripts/build.sh
./scripts/test.sh
./scripts/format.sh
./scripts/smoke.sh
python3 scripts/update_repo_maps.py
```

## Перед сдачей задачи

1. Перечитай task-specific `AGENTS.md`.
2. Проверь, что изменения не выходят за scope.
3. Обнови связанные companion-specs.
4. Обнови `docs/STATUS.md`.
5. Если изменились границы работ — обнови `docs/PLAN.md`.
6. Подготовь отчёт строго по шаблону.
