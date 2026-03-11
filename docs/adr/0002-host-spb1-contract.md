# ADR 0002 — Host/SPB1 contract

Статус: accepted
Дата: 2026-03-09

## Решение

Host-plane отвечает за ingest, verify, storage, graph resolution, orchestration, SMS/TMS и open playback.
SPB1/secure module отвечает за device identity, secure time, authorization, key unwrap/decrypt service и security logs.

## Последствия

- host не хранит приватные ключи secure plane;
- simulator-first обязателен;
- security-sensitive transitions логируются независимо от business logs.
