# dcplayer — `docs/PLAN.md`

## 1. Назначение документа

Этот документ — рабочий branch-sized план проекта `dcplayer` для Codex. Его задача — резать работу на такие ветки, которые сохраняют инженерный контекст, проходят приёмку в одной ветке и не упираются в контекстный лимит.

Рабочий корень проекта: `/Users/admin/Documents/dcplayer`.

## 2. Неподвижные архитектурные ограничения

### 2.1. Форма продукта

Система состоит из двух контуров:

- **host-plane**: ingest, verify, local storage, asset graph, CPL/OV/VF/supplemental resolution, timeline, open playback path, SMS/TMS, observability, tooling;
- **secure-plane**: HSM/RTC service, identity, certificate handling, KDM gating, secure clock, signed audit, tamper/recovery, secure encrypted-show control path.

### 2.2. Эталонная secure playback chain

Для encrypted show фиксируем следующую цепочку:

`local storage -> HSM decrypt service -> GPU decode -> GPU picture FM -> playout`

Эта цепочка — эталонный production path. Open/debug path допускается только как отдельный режим и не может тихо подменять secure path.

### 2.3. Secure control-plane profile, который обязателен к встраиванию

В проект встроены требования из note про Raspberry Pi + ZymKey + TPM2:

- Raspberry Pi + ZymKey выступает TLS-сервером; серверный приватный ключ остаётся внутри ZymKey;
- Ubuntu media server использует клиентский приватный ключ в TPM2 и проходит client-certificate authentication;
- канал — mutual-TLS с проверкой цепочки сертификатов и revocation;
- доступ режется ACL и identity checks;
- операции пишутся в локальный и защищённый аудит;
- tamper, escrow, recovery, time discipline, jump-host, MFA и RBAC входят в операционный baseline;
- обязательны acceptance tests на handshake, sign/verify, ACL deny, tamper и recovery.

### 2.4. Дорожная карта по аудио

- **Stage A**: internal PCM engine + label-driven routing + sync с timeline;
- **Stage B**: 24-bit AES3 output adapter как отдельный subsystem.

### 2.5. Дорожная карта по forensic watermark

- picture FM входит в secure show path;
- sound FM ведётся отдельной веткой и не смешивается с picture FM-задачами;
- критерий “100% идентификация” допускается только относительно утверждённой attack matrix и detector harness.

## 3. Политика размера веток

Технический потолок, наблюдаемый пользователем для одной ветки Codex: **258000 токенов**.

Для проекта вводится более жёсткая внутренняя дисциплина:

- **целевая зелёная зона**: 60k–120k;
- **жёлтая зона**: 120k–180k;
- **красная зона**: 180k–220k;
- **операционный стоп**: после 220k задачу нужно резать или закрывать ветку handoff-ом;
- **жёсткий технический потолок**: 258k; не планировать ветки вплотную к нему.

Правило: одна ветка = одна инженерная цель, один основной subsystem, одна поверхность приёмки.

## 4. Правила durable memory

### 4.1. Источники долговременной памяти проекта

Codex должен считать durable memory следующими файлами:

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/IMPLEMENT.md`
- `docs/CODE_STYLE.md`
- `docs/STATUS.md`
- `specs/objects/*.md`

### 4.2. Правило узкого readset

Каждый task-specific `AGENTS.md` указывает только те 5–7 файлов, которые нужно прочитать до первого изменения. Инструкция “прочитай весь репозиторий” запрещена.

### 4.3. Правило handoff

Каждая завершённая ветка обязана обновить:

- `docs/STATUS.md`
- `docs/IMPLEMENT.md` или `docs/ARCHITECTURE.md`, если менялись детали реализации или архитектура
- `docs/PLAN.md`, только если менялась последовательность/границы работ
- task-local artifacts, logs, reports и acceptance evidence в файловом виде

### 4.4. Spec-first rule

Если меняется объектный контракт, validation invariant, state machine или внешний API, сначала обновляется companion-spec, и только затем код.

### 4.5. Правило длинных логов

Длинные логи, тестовые дампы, evidence-pack и perf-отчёты складываются в файлы и артефакты. В чат попадает только краткое резюме и путь к артефакту.

## 5. Baseline companion-specs

- `specs/objects/AssetMap.md`
- `specs/objects/PKL.md`
- `specs/objects/CPL.md`
- `specs/objects/Reel.md`
- `specs/objects/TrackFile.md`
- `specs/objects/CompositionGraph.md`
- `specs/objects/PlaybackTimeline.md`
- `specs/objects/SupplementalMergePolicy.md`
- `specs/objects/CertificateStore.md`
- `specs/objects/TrustChain.md`
- `specs/objects/SecureChannelContract.md`
- `specs/objects/ProtectedApiEnvelope.md`
- `specs/objects/SecurityModuleContract.md`
- `specs/objects/SecureClockPolicy.md`
- `specs/objects/HsmDecryptSession.md`
- `specs/objects/KDMEnvelope.md`
- `specs/objects/KDMValidationResult.md`
- `specs/objects/SecurityEvent.md`
- `specs/objects/AuditExport.md`
- `specs/objects/J2KBackendContract.md`
- `specs/objects/FrameSurface.md`
- `specs/objects/WatermarkPayload.md`
- `specs/objects/WatermarkState.md`
- `specs/objects/WatermarkEvidence.md`
- `specs/objects/DetectorReport.md`
- `specs/objects/ChannelRouteMap.md`
- `specs/objects/AES3DeviceProfile.md`
- `specs/objects/SubtitleCue.md`
- `specs/objects/TimedTextDocument.md`
- `specs/objects/PlaybackSession.md`
- `specs/objects/FaultMatrix.md`
- `specs/objects/OperatorAlert.md`
- `specs/objects/ScheduleEntry.md`
- `specs/objects/ShowControlCommand.md`

## 6. Порядок выполнения

Приоритет старта для первых веток:

- `T00a`
- `T00b`
- `T00c`
- `T01a`
- `T01b`
- `T01c`
- `T02a`
- `T03a`
- `T03b`

Дальше работа идёт по зависимостям задач ниже. Не начинать новую ветку, пока предыдущая не оставила воспроизводимый handoff.

## Phase A — Основание проекта

### T00a — Repository bootstrap

**Цель**  
Поднять каркас репозитория, CMake, presets, scripts, пустые targets и минимальные handoff-шаблоны.

**Класс ветки**  
XS

**Зависимости**  
нет

**Основные выходы**
- Собираемый каркас репозитория
- build presets
- bootstrap/smoke scripts
- пустые targets и каталоги

**Definition of Done**  
Проект конфигурируется и собирается; smoke-проверки выполняются; каркас готов для следующих веток.

### T00b — Formal companion specs freeze

**Цель**  
Зафиксировать baseline companion-спецификаций объектов и контрактов, которые будут источником истины для последующих веток.

**Класс ветки**  
S

**Зависимости**  
`T00a`

**Основные выходы**
- Комплект specs/objects/*.md
- единый стиль полей, инвариантов и validation rules
- versioning notes

**Definition of Done**  
Все baseline-specs существуют; в коде не появляются скрытые контракты, не отражённые в спецификациях.

### T00c — Project memory discipline

**Цель**  
Зафиксировать правила durable memory, handoff, артефактов и readset для branch-sized режима.

**Класс ветки**  
XS

**Зависимости**  
`T00a`

**Основные выходы**
- Правила обновления STATUS/PLAN/IMPLEMENT
- формат handoff
- правила для task-specific AGENTS.md

**Definition of Done**  
Любая следующая ветка может завершиться воспроизводимым handoff без зависимости от истории чата.

---

## Phase B — DCP domain model

### T01a — ASSETMAP + PKL parser

**Цель**  
Реализовать parse и базовую validation для ASSETMAP и PKL с детерминированной диагностикой.

**Класс ветки**  
S

**Зависимости**  
`T00a`, `T00b`

**Основные выходы**
- Нормализованные модели ASSETMAP/PKL
- valid/invalid fixtures
- unit tests

**Definition of Done**  
Валидные fixtures разбираются стабильно; невалидные — падают с предсказуемой диагностикой.

### T01b — CPL parser

**Цель**  
Реализовать parse и validation для CPL, reels, track references, edit rate и composition metadata.

**Класс ветки**  
S

**Зависимости**  
`T01a`

**Основные выходы**
- Нормализованный объект CPL
- negative fixtures
- валидационные ошибки

**Definition of Done**  
Парсер выдаёт детерминированный AST/domain object; ссылки на reels и track files представлены явно.

### T01c — dcp_probe + composition graph

**Цель**  
Сделать CLI, который читает DCP directory и печатает нормализованный composition graph в JSON.

**Класс ветки**  
S

**Зависимости**  
`T01a`, `T01b`

**Основные выходы**
- apps/dcp_probe
- детерминированный JSON
- integration tests на Interop/SMPTE fixtures

**Definition of Done**  
`dcp_probe <path>` выдаёт стабильный JSON и корректно кодирует граф композиции.

---

## Phase C — Resolver и timeline

### T02a — OV/VF resolver

**Цель**  
Реализовать разрешение зависимостей OV/VF и диагностику missing/conflicting assets.

**Класс ветки**  
S

**Зависимости**  
`T01c`

**Основные выходы**
- Resolver зависимостей OV/VF
- диагностика конфликтов
- unit/integration tests

**Definition of Done**  
Из набора OV/VF строится итоговый composition graph, пригодный для dry-run и показа.

### T02b — Supplemental semantics

**Цель**  
Зафиксировать и реализовать правила merge/override для supplemental packages.

**Класс ветки**  
S

**Зависимости**  
`T02a`

**Основные выходы**
- Правила merge/override
- fixtures OV+VF+supplemental
- понятные diagnostics

**Definition of Done**  
Supplemental merge работает предсказуемо, а конфликтные случаи объясняются явной диагностикой.

### T02c — Dry-run playback timeline

**Цель**  
Построить playback timeline по reel'ам без decode и playout.

**Класс ветки**  
S

**Зависимости**  
`T02a`, `T02b`

**Основные выходы**
- Machine-readable timeline dump
- effective durations
- track-level timeline

**Definition of Done**  
Timeline собирается без фактического воспроизведения и годится для dry-run верификации показа.

---

## Phase D — Security control plane

### T03a — PKI and trust store baseline

**Цель**  
Поднять объектную модель trust store, chain validation и revocation policy.

**Класс ветки**  
S

**Зависимости**  
`T00b`

**Основные выходы**
- Certificate store model
- trust chain validation
- CRL/OCSP policy

**Definition of Done**  
Импорт цепочек и листовых сертификатов валидируется детерминированно; модель revocation задокументирована.

### T03b — Pi/ZymKey ↔ Ubuntu/TPM mutual-TLS contract

**Цель**  
Формализовать защищённый канал между Pi+ZymKey и Ubuntu+TPM: server key в ZymKey, client key в TPM, mutual-TLS, identity checks и acceptance criteria.

**Класс ветки**  
M

**Зависимости**  
`T03a`

**Основные выходы**
- SecureChannelContract
- sequence diagrams
- acceptance criteria на mutual-TLS

**Definition of Done**  
Контракт покрывает handshake, client-cert auth, chain validation, CRL/OCSP, CN/SAN/device identity checks и отказ неавторизованным клиентам.

### T03c — ACL + protected API envelope

**Цель**  
Зафиксировать общий envelope и ACL для защищённых вызовов `/sign`, будущих `/unwrap` и `/decrypt`.

**Класс ветки**  
S

**Зависимости**  
`T03b`

**Основные выходы**
- ProtectedApiEnvelope
- ACL model
- error codes и idempotency rules

**Definition of Done**  
Есть единый контракт запроса/ответа, audit-поля и поведение deny/allow без двусмысленностей.

### T03d — Audit, tamper, escrow, recovery

**Цель**  
Описать и реализовать модель security-аудита, tamper-состояний, escrow и recovery.

**Класс ветки**  
M

**Зависимости**  
`T03b`, `T03c`

**Основные выходы**
- SecurityEvent/AuditExport model
- tamper/recovery state transitions
- acceptance tests

**Definition of Done**  
События безопасности, tamper-реакции и recovery-процедуры формализованы и проверяемы.

### T03e — Secure time semantics

**Цель**  
Зафиксировать semantics защищённого времени: UTC source, skew policy, show-window dependency и логирование времени.

**Класс ветки**  
S

**Зависимости**  
`T03b`

**Основные выходы**
- SecureClockPolicy
- API semantics для GetTimeUTC
- negative tests на drift/skew/window

**Definition of Done**  
Поведение времени однозначно; отклонения времени и неверные окна показа отрабатываются предсказуемо.

---

## Phase E — KDM и security module

### T04a — ISecurityModule + simulator

**Цель**  
Собрать host-side API secure module и simulator с тем же контрактом.

**Класс ветки**  
M

**Зависимости**  
`T03c`, `T03e`

**Основные выходы**
- ISecurityModule
- spb1 simulator
- integration tests host↔simulator

**Definition of Done**  
Host-код работает через контракт, а simulator покрывает основной happy path и ключевые отказы.

### T04b — KDM ingest and binding

**Цель**  
Импортировать KDM, привязать его к composition/CPL и валидировать show-window.

**Класс ветки**  
S

**Зависимости**  
`T04a`

**Основные выходы**
- KDM ingest
- binding к CPL
- validation results и tests

**Definition of Done**  
Valid / expired / future / mismatched KDM покрыты тестами и выдают корректные решения allow/deny.

### T04c — Fail-closed playback state machine

**Цель**  
Формализовать fail-closed state machine для security-critical показа.

**Класс ветки**  
M

**Зависимости**  
`T04a`, `T04b`

**Основные выходы**
- PlaybackSession/FaultMatrix state machine
- negative tests
- запрет silent fallback

**Definition of Done**  
Нет неаудируемых состояний и нет незаметной подмены secure path на debug/open path.

### T04d — Security event export

**Цель**  
Сделать формат и policy экспорта security events на host/operator layer без утечки секретов.

**Класс ветки**  
S

**Зависимости**  
`T03d`, `T04a`

**Основные выходы**
- AuditExport format
- operator summaries
- tests на redaction/selection

**Definition of Done**  
Оператор получает только допустимую operational информацию, а секреты не покидают secure boundary.

---

## Phase F — Decode backend stack

### T05a — J2K backend abstraction

**Цель**  
Ввести общий интерфейс J2K backend, frame surfaces, error model и perf counters.

**Класс ветки**  
S

**Зависимости**  
`T00b`

**Основные выходы**
- J2KBackendContract
- FrameSurface model
- backend abstraction in code

**Definition of Done**  
CPU и GPU backend можно подключать без переписывания orchestration слоя.

### T05b — CPU reference decoder

**Цель**  
Собрать CPU reference decoder как baseline/oracle и fallback.

**Класс ветки**  
M

**Зависимости**  
`T05a`

**Основные выходы**
- CPU decode path
- reference tests
- baseline perf/logging hooks

**Definition of Done**  
CPU path декодирует корректно и служит oracle для сравнения с GPU backend.

### T05c — GPU decode contract

**Цель**  
Формализовать handoff между HSM decrypt service, GPU decode и frame queues.

**Класс ветки**  
S

**Зависимости**  
`T05a`, `T03c`

**Основные выходы**
- HsmDecryptSession + GPU handoff contract
- queue/sync rules
- error propagation rules

**Definition of Done**  
Переход `decrypt -> GPU decode` описан как явный контракт с очередями, ошибками и границами ответственности.

### T05d — NVIDIA backend

**Цель**  
Реализовать production-grade GPU decode backend для NVIDIA.

**Класс ветки**  
M

**Зависимости**  
`T05a`, `T05c`

**Основные выходы**
- NVIDIA J2K backend
- integration/smoke tests
- perf counters

**Definition of Done**  
На эталонных samples backend стабильно декодирует и выдаёт измеримый производственный path.

---

## Phase G — Forensic watermark

### T06a — Watermark object model

**Цель**  
Зафиксировать payload model, state machine, evidence model и attack matrix для FM.

**Класс ветки**  
S

**Зависимости**  
`T00b`

**Основные выходы**
- WatermarkPayload/State/Evidence/DetectorReport specs
- control flags
- attack matrix baseline

**Definition of Done**  
И inserter, и detector опираются на один и тот же формальный объектный контракт.

### T06b — Detector harness

**Цель**  
Сделать offline harness для проверки извлекаемости watermark после контролируемых деградаций.

**Класс ветки**  
M

**Зависимости**  
`T06a`

**Основные выходы**
- detector harness
- attack corpus runner
- scoring/report format

**Definition of Done**  
Есть воспроизводимый прогон по attack matrix и единый формат отчёта по детекции.

### T06c — GPU picture FM prototype

**Цель**  
Сделать picture FM insertion в GPU frame pipeline.

**Класс ветки**  
M

**Зависимости**  
`T06a`, `T05d`

**Основные выходы**
- GPU picture FM path
- логирование FM state
- tests на pipeline integrity

**Definition of Done**  
Вставка picture FM происходит в GPU path без разрыва основного playback pipeline.

### T06d — Secure playback integration

**Цель**  
Интегрировать chain `HSM decrypt -> GPU decode -> GPU picture FM -> playout` как единый secure show path.

**Класс ветки**  
M

**Зависимости**  
`T05c`, `T06c`, `T04c`

**Основные выходы**
- end-to-end secure playback chain
- state transitions
- fault handling

**Definition of Done**  
Secure chain собирается как единый рабочий путь и не заменяется тихо на open/debug path.

### T06e — Sound FM reference path

**Цель**  
Сделать sound FM как отдельную ветку, независимую от picture FM.

**Класс ветки**  
M

**Зависимости**  
`T06a`

**Основные выходы**
- reference sound FM path
- detector integration
- attack-matrix tests для аудио

**Definition of Done**  
Audio FM развивается отдельно и не смешивается с задачами picture FM.

---

## Phase H — Audio и subtitles

### T07a — PCM engine + channel routing

**Цель**  
Реализовать multichannel PCM engine и label-driven routing.

**Класс ветки**  
M

**Зависимости**  
`T02c`

**Основные выходы**
- PCM engine
- ChannelRouteMap
- tests на routing

**Definition of Done**  
Routing задаётся метками и картой, а не неявными позициями каналов.

### T07b — Audio clock and sync stabilization

**Цель**  
Стабилизировать sync аудио с playback timeline и drift handling.

**Класс ветки**  
M

**Зависимости**  
`T07a`, `T03e`

**Основные выходы**
- audio clock discipline
- sync logic
- load/drift tests

**Definition of Done**  
Аудио держит sync с timeline в допустимых пределах и корректно обрабатывает drift.

### T07c — AES3 adapter

**Цель**  
Сделать 24-bit AES3 output adapter как отдельный subsystem.

**Класс ветки**  
S

**Зависимости**  
`T07a`

**Основные выходы**
- AES3 device abstraction
- adapter implementation
- loopback/integration tests

**Definition of Done**  
Адаптер отделён от PCM engine и имеет явный контракт устройства/выхода.

### T07d — Interop subtitles

**Цель**  
Реализовать корректный path для Interop subtitles/subpicture.

**Класс ветки**  
S

**Зависимости**  
`T02c`

**Основные выходы**
- Interop subtitle parser/render integration
- timing tests

**Definition of Done**  
Interop subtitles корректно парсятся, таймятся и проходят через render integration.

### T07e — SMPTE timed text

**Цель**  
Реализовать SMPTE timed text path, включая fonts и timing.

**Класс ветки**  
S

**Зависимости**  
`T07d`

**Основные выходы**
- TimedText parser/render integration
- font handling
- timing tests

**Definition of Done**  
SMPTE timed text корректно обрабатывается вместе со шрифтами и временными событиями.

---

## Phase I — Playback orchestration

### T08a — Open playout orchestrator

**Цель**  
Собрать open-content orchestrator для локальной отладки и baseline playback.

**Класс ветки**  
M

**Зависимости**  
`T05b`, `T07a`, `T07d`

**Основные выходы**
- open playout service
- logs/metrics
- baseline playback tests

**Definition of Done**  
Open path воспроизводит контент стабильно и служит базой сравнения для secure path.

### T08b — Secure playout orchestrator

**Цель**  
Собрать полный secure path: `local storage -> HSM decrypt service -> GPU decode -> GPU picture FM -> playout`.

**Класс ветки**  
M

**Зависимости**  
`T04c`, `T05c`, `T06d`, `T07a`

**Основные выходы**
- secure playout service
- session orchestration
- integration tests

**Definition of Done**  
Полный secure show path работает как одна оркеструемая сессия с корректными переходами состояний.

### T08c — Fault matrix

**Цель**  
Сделать явную матрицу отказов для network loss, HSM deny, KDM invalidation, GPU failure и FM unavailable.

**Класс ветки**  
S

**Зависимости**  
`T08b`

**Основные выходы**
- FaultMatrix implementation
- operator alerts
- negative tests

**Definition of Done**  
Для каждого отказа определено корректное поведение и operator-visible outcome.

### T08d — Encrypted show rehearsal

**Цель**  
Провести полную репетицию encrypted playback с журналом событий, acceptance checklist и evidence pack.

**Класс ветки**  
M

**Зависимости**  
`T08b`, `T08c`

**Основные выходы**
- runbook
- evidence pack
- acceptance checklist
- операторский отчёт

**Definition of Done**  
Есть воспроизводимая репетиция encrypted show и комплект доказательств прохождения критериев.

---

## Phase J — SMS / TMS / Operations

### T09a — Local SMS API

**Цель**  
Сделать локальный API управления одним auditorium: inventory, play/pause/stop, state, alerts.

**Класс ветки**  
S

**Зависимости**  
`T08a`

**Основные выходы**
- local SMS API
- auth model
- integration tests

**Definition of Done**  
Один зал управляется локально через явный API с аутентификацией и журналом действий.

### T09b — Minimal TMS web

**Цель**  
Сделать минимальный TMS: inventory, CPL/KDM status, schedule, alerts, remote launch.

**Класс ветки**  
M

**Зависимости**  
`T09a`, `T04b`

**Основные выходы**
- minimal web UI
- schedule/status views
- alerts and remote launch

**Definition of Done**  
Оператор видит состав контента, статус окон показа/KDM и может инициировать показ удалённо.

### T09c — Monitoring and operator audit

**Цель**  
Свести monitoring, operator alerts и operator-side audit в единый operational layer.

**Класс ветки**  
S

**Зависимости**  
`T09b`, `T04d`

**Основные выходы**
- monitoring views
- operator audit trail
- incident export

**Definition of Done**  
Операторский слой даёт целостную картину состояния, алармов и действий персонала.

---

## 7. Общие stop-rules для всех веток

- Остановись, если для завершения задачи нужно притянуть соседнюю ветку из `docs/PLAN.md`.
- Остановись, если оценка ветки вышла в красную зону и задача не была заранее расщеплена.
- Остановись, если меняется объектный контракт, а companion-spec ещё не обновлён.
- Не делай opportunistic refactor вне разрешённой поверхности файлов.
- Не меняй публичные интерфейсы смежных модулей без явного описания в handoff и спецификациях.

## 8. Общий формат branch report

- Что сделано.
- Какие файлы изменены.
- Как проверено.
- Что сознательно не покрыто.
- Какие риски и follow-up остались.
