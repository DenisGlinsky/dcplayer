# AGENTS.md — T09c Operator Audit

Этот файл ограничивает одну ветку Codex одной инженерной целью и одной поверхностью приёмки. Перед первым изменением прочитай только Required reading set. Не расширяй scope за пределы этой задачи.

## 1. Цель ветки

Свести monitoring, operator alerts и operator-side audit в единый operational layer.

## 2. Контекстный бюджет

- Плановый класс ветки: **S**.
- Целевая зона: **60k–120k**.
- Жёлтая зона: **120k–180k**.
- Красная зона: **180k–220k**. Если видишь, что ветка туда уходит, режь scope и оставляй handoff.
- Никогда не планируй ветку вплотную к техническому потолку **258k**.

## 3. Что вне scope

- Без full BI/reporting suite.
- без feature-creep в playback core.
- без unrelated auth redesign.

## 4. Required reading set

Прочитай только эти файлы до первого изменения:

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/OperatorAlert.md`
- `specs/objects/SecurityEvent.md`
- `specs/objects/AuditExport.md`
- `docs/STATUS.md`

Зависимости по плану:
- `T09b`
- `T04d`

## 5. Разрешённая поверхность изменений

- `apps/tms_web/**`
- `apps/sms_ui/**`
- `src/core/**`
- `src/security_api/log_client/**`
- `tests/integration/ops_audit/**`
- `specs/objects/OperatorAlert.md`
- `specs/objects/AuditExport.md`
- `docs/IMPLEMENT.md`
- `docs/STATUS.md`

## 6. Ожидаемые deliverables

- monitoring views
- operator audit trail
- incident export
- integration tests

## 7. Definition of Done

Операторский слой даёт целостную картину состояния, алармов и действий персонала.

## 8. Верификация

Минимальный набор команд:

```bash
./scripts/build.sh
./scripts/test.sh -R 'monitoring|ops_audit|incident'
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
