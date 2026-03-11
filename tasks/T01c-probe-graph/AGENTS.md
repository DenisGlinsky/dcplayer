# AGENTS.md — T01c Probe/Graph

Этот файл ограничивает одну ветку Codex одной инженерной целью и одной поверхностью приёмки. Перед первым изменением прочитай только Required reading set. Не расширяй scope за пределы этой задачи.

## 1. Цель ветки

Сделать CLI, который читает DCP directory и печатает нормализованный composition graph в JSON.

## 2. Контекстный бюджет

- Плановый класс ветки: **S**.
- Целевая зона: **60k–120k**.
- Жёлтая зона: **120k–180k**.
- Красная зона: **180k–220k**. Если видишь, что ветка туда уходит, режь scope и оставляй handoff.
- Никогда не планируй ветку вплотную к техническому потолку **258k**.

## 3. Что вне scope

- Без decode/playback.
- без OV/VF merge semantics.
- без security logic.

## 4. Required reading set

Прочитай только эти файлы до первого изменения:

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/CompositionGraph.md`
- `specs/objects/AssetMap.md`
- `specs/objects/PKL.md`
- `specs/objects/CPL.md`

Зависимости по плану:
- `T01a`
- `T01b`

## 5. Разрешённая поверхность изменений

- `apps/dcp_probe/**`
- `src/dcp/**`
- `tests/integration/dcp_probe/**`
- `tests/fixtures/dcp/**`
- `specs/objects/CompositionGraph.md`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 6. Ожидаемые deliverables

- apps/dcp_probe
- детерминированный JSON composition graph
- integration tests для Interop/SMPTE fixtures

## 7. Definition of Done

`dcp_probe <path>` выдаёт стабильный JSON и корректно кодирует граф композиции.

## 8. Верификация

Минимальный набор команд:

```bash
./scripts/build.sh
./scripts/test.sh -R 'dcp_probe|composition'
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
