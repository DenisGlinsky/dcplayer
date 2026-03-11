# AGENTS.md — T08d Show Rehearsal

Этот файл ограничивает одну ветку Codex одной инженерной целью и одной поверхностью приёмки. Перед первым изменением прочитай только Required reading set. Не расширяй scope за пределы этой задачи.

## 1. Цель ветки

Провести полную репетицию encrypted playback с журналом событий, acceptance checklist и evidence pack.

## 2. Контекстный бюджет

- Плановый класс ветки: **M**.
- Целевая зона: **60k–120k**.
- Жёлтая зона: **120k–180k**.
- Красная зона: **180k–220k**. Если видишь, что ветка туда уходит, режь scope и оставляй handoff.
- Никогда не планируй ветку вплотную к техническому потолку **258k**.

## 3. Что вне scope

- Без feature-creep в ядро.
- без новых vendor backends.
- без redesign TMS/SMS.

## 4. Required reading set

Прочитай только эти файлы до первого изменения:

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/PlaybackSession.md`
- `specs/objects/SecurityEvent.md`
- `specs/objects/OperatorAlert.md`
- `docs/STATUS.md`

Зависимости по плану:
- `T08b`
- `T08c`

## 5. Разрешённая поверхность изменений

- `tests/compliance/encrypted_show/**`
- `apps/playout_service/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`
- `docs/ARCHITECTURE.md`

## 6. Ожидаемые deliverables

- encrypted show rehearsal runbook
- evidence pack
- acceptance checklist
- operator report

## 7. Definition of Done

Есть воспроизводимая репетиция encrypted show и комплект доказательств прохождения критериев.

## 8. Верификация

Минимальный набор команд:

```bash
./scripts/build.sh
./scripts/test.sh -R 'encrypted_show|rehearsal'
```

Если часть команд ещё не существует на этой фазе, создай ближайший эквивалентный smoke/integration check и зафиксируй это в handoff.

## 9. Обязательные обновления при handoff

- Обнови `docs/STATUS.md`: что сделано, чем проверено, что не покрыто, какие риски остались.
- Обнови `docs/IMPLEMENT.md`, если изменились модульные границы, flow команд, форматы данных или детали интеграции.
- Обнови `docs/ARCHITECTURE.md` только если изменилось архитектурное решение, boundary или ответственность подсистем.
- Обнови `docs/PLAN.md` только если изменились зависимости, порядок фаз, оценка ветки или состав работ.

## 10. Stop conditions

- Остановись, если для завершения задачи нужно притянуть соседнюю ветку из `docs/PLAN.md`.
- Остановись, если оценка ветки вышла в красную зону и задача не была заранее расщеплена.
- Остановись, если меняется объектный контракт, а companion-spec ещё не обновлён.
- Не делай opportunistic refactor вне разрешённой поверхности файлов.
- Не меняй публичные интерфейсы смежных модулей без явного описания в handoff и спецификациях.

## 11. Формат отчёта по ветке

- Что сделано.
- Какие файлы изменены.
- Как проверено.
- Что сознательно не покрыто.
- Какие риски и follow-up остались.

## 12. Security-профиль, который нельзя ослаблять

- mutual-TLS обязателен;
- серверный ключ остаётся в ZymKey;
- клиентский ключ остаётся в TPM;
- identity checks и ACL обязательны;
- аудит, tamper, recovery и time semantics нельзя выкидывать как 'потом';
- acceptance tests по security-профилю входят в Definition of Done.
