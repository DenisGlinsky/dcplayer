# ARCHITECTURE

## 1. Назначение

Документ фиксирует архитектурный baseline проекта `dcplayer` на этапе scaffold.
Он описывает:
- продуктовые контуры;
- trust boundary;
- эталонный secure show path;
- модульную декомпозицию;
- ограничения, которые нельзя тихо менять в реализации.

## 2. Системная форма

Система состоит из двух контуров.

### 2.1 Host-plane
Функции:
- ingest и verify;
- local storage;
- разбор ASSETMAP / PKL / CPL;
- OV / VF / supplemental resolution;
- timeline;
- open playback path;
- operator tooling;
- SMS / TMS;
- observability и diagnostics.

### 2.2 Secure-plane
Функции:
- identity;
- certificate handling;
- secure clock / RTC;
- KDM gating;
- decrypt authorization;
- tamper / recovery / escrow semantics;
- signed / protected audit;
- security-sensitive show control.

На текущем baseline secure-plane представлен архитектурно и через симуляторный контракт, а не через готовую production-реализацию.

## 3. Эталонный secure show path

```text
local storage
  -> HSM decrypt service (Raspberry Pi + ZymKey + RTC)
  -> GPU decode
  -> GPU picture FM
  -> playout
```

Для аудио:

```text
local storage
  -> secure authorization
  -> decrypt
  -> audio FM
  -> PCM engine
  -> later AES3
```

Open/debug path допустим только как отдельный режим и не может молча подменять secure path.

## 4. Secure control plane profile

Базовый профиль для security control plane:
- Raspberry Pi + ZymKey работает как TLS server;
- Ubuntu media server + TPM 2.0 работает как TLS client;
- серверный приватный ключ остаётся внутри ZymKey;
- клиентский приватный ключ остаётся внутри TPM;
- mutual-TLS обязателен;
- revocation и identity checks обязательны;
- ACL и audit обязательны;
- tamper, recovery, escrow и secure time являются частью baseline, а не “дополнительной опцией”.

## 5. Декомпозиция репозитория

### 5.1 Приложения
- `apps/dcp_probe/` — CLI для анализа состава DCP и composition graph.
- `apps/playout_service/` — future orchestrator/service для показа.
- `apps/sms_ui/` — local auditorium control.
- `apps/tms_web/` — minimal TMS.

### 5.2 Исходные подсистемы
- `src/core/` — общие базовые компоненты и scaffold utilities.
- `src/dcp/` — parsing и composition logic.
- `src/playout/` — timeline, sync, frame queue, orchestrators.
- `src/video/` — CPU/GPU decode stack и secure decode adapters.
- `src/audio/` — PCM engine, routing, AES3.
- `src/subtitles/` — Interop и SMPTE Text.
- `src/security_api/` — host-side contracts к secure-plane.
- `src/watermark/` — picture/audio FM и detector harness.
- `src/platform/` — платформенные адаптеры.

### 5.3 Secure-plane subtree
- `spb1/api/` — формальный контракт secure module.
- `spb1/simulator/` — simulator-first реализация.
- `spb1/firmware/` — будущий firmware/embedded code.
- `spb1/diag/` — diagnostics и service tooling.

## 6. Pluggable forensic watermark

Подсистема forensic watermark считается pluggable только в одном смысле:
можно менять backend внедрения/детекции, но нельзя ломать:
- payload contract;
- control semantics;
- state machine;
- operator-visible policy;
- detector report format.

Это позволяет иметь:
- CPU reference path;
- GPU production path;
- lab/research path;
- detector harness
без расползания watermark-логики по остальным подсистемам.

## 7. Архитектурные запреты

Нельзя без ADR:
- менять trust boundary;
- переносить secrets в host-plane;
- убирать fallback path у decode/watermark backend-а;
- объявлять secure path “эквивалентным” open path;
- вводить неподконтрольные бинарные watermark plugins;
- логировать plaintext essence, ключи или KDM body.

## 8. Состояние scaffold baseline

На этом этапе реализованы:
- структура репозитория;
- branch-sized task framing;
- companion-specs;
- CMake-scaffold;
- stub-приложения и минимальные тесты.

На этом этапе не реализованы:
- реальный DCP parsing;
- secure module;
- KDM logic;
- GPU decode;
- watermark insertion;
- playout orchestration.
