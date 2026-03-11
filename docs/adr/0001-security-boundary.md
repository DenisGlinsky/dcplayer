# ADR 0001 — Security boundary

Статус: accepted
Дата: 2026-03-09

## Решение

Secure show path фиксируется как:
`local storage -> HSM decrypt service (Raspberry Pi + ZymKey + RTC) -> GPU decode -> GPU picture FM -> playout`.

Управляющий канал между media server и secure module строится на mutual-TLS,
где клиентский приватный ключ находится в TPM 2.0 media server,
а серверный приватный ключ находится в ZymKey на Raspberry Pi.

## Последствия

- secure mode обязан быть fail-closed;
- object contracts и logs не должны раскрывать секреты;
- любые изменения trust boundary оформляются новым ADR.
