# ADR 0003 — Watermark control model

Статус: accepted
Дата: 2026-03-09

## Решение

Forensic watermark оформляется как внутренний pluggable subsystem с единым control contract.
Менять можно backend внедрения/детекции, но нельзя менять семантику state machine,
security logs, payload model и operator-facing policy без отдельного решения.

## Последствия

- возможны CPU reference и GPU production backends;
- detector harness существует отдельно от inline playout backend;
- attack matrix фиксируется и версионируется.
