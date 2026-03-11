# ADR 0004 — Audio roadmap to AES3

Статус: accepted
Дата: 2026-03-09

## Решение

Аудиодорожка проекта развивается поэтапно:
1. internal PCM engine + label-driven routing;
2. AES3 output adapter.

## Последствия

- core audio model не должен зависеть от конкретного hardware adapter;
- routing profile задаётся доменным объектом, а не hardcoded позициями каналов;
- AES3 интеграция идёт отдельной задачей T08.
