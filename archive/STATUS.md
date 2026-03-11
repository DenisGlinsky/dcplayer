# STATUS

_Дата фиксации: 2026-03-09_

## 1. Общее состояние

Проект находится на стадии **architecture + documentation freeze**.
Реального playback кода пока нет. Это нормально и запланировано.

## 2. Что уже готово

- создана структура репозитория;
- написан `AGENTS.md`;
- написан `README.md`;
- написаны:
  - `docs/ARCHITECTURE.md`
  - `docs/CODE_STYLE.md`
  - `docs/IMPLEMENT.md`
  - `docs/LIBRARIES.md`
  - `docs/PLAN.md`
  - `docs/STATUS.md`
- созданы task-specific `AGENTS.md` для всех задач;
- созданы companion-спецификации объектов;
- создан `.codex/config.toml`;
- добавлены минимальные bootstrap/build scripts;
- добавлены стартовые ADR и зафиксирована tree-структура репозитория.

## 3. Что зафиксировано архитектурно

### Frozen
- base flow: `ingest -> verify -> local storage -> playout`;
- secure show chain:
  `local storage -> HSM decrypt service (Raspberry Pi + ZymKey + RTC) -> GPU decode -> GPU picture FM -> playout`;
- mutual-TLS между Ubuntu/TPM и Pi/ZymKey;
- pluggable watermark contract;
- наличие secure clock, signed logs, ACL и tamper semantics;
- phased audio roadmap: PCM engine -> AES3.

### Provisional
- конкретная wire-схема secure API;
- финальный TMS web stack;
- финальный payload format watermark;
- точный hardware path для secure output;
- метод adjacent trusted compute, если Pi-only decrypt throughput не проходит.

## 4. Ближайшая следующая задача

**Рекомендуемая стартовая задача для Codex: `T00-security-boundary-bootstrap`**

Почему именно она:
- фиксирует contracts;
- создаёт simulator;
- минимизирует риск расползания архитектуры;
- готовит фундамент для T01/T03.

## 5. Основные открытые риски

1. Производительность decrypt stage на Raspberry Pi + ZymKey sidecar.
2. Robustness picture/audio watermark при утверждённой attack matrix.
3. Выбор production-grade GPU paths вне NVIDIA.
4. Интеграция AES3 и cinema processor layer.
5. Полнота fixture corpus для Interop/SMPTE/OV/VF/supplemental.

## 6. Следующее обновление этого файла

Обновить после:
- завершения T00;
- появления первых real code targets;
- изменения trust boundary;
- изменения task sequence.
