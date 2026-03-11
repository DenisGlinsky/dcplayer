# AGENTS.md — T01b CPL

Этот файл ограничивает одну ветку Codex одной инженерной целью и одной поверхностью приёмки. Перед первым изменением прочитай только Required reading set. Не расширяй scope за пределы этой задачи.

## 1. Цель ветки

Реализовать parse и validation для CPL, reels, track references, edit rate и composition metadata.

## 2. Контекстный бюджет

- Плановый класс ветки: **S**.
- Целевая зона: **60k–120k**.
- Жёлтая зона: **120k–180k**.
- Красная зона: **180k–220k**. Если видишь, что ветка туда уходит, режь scope и оставляй handoff.
- Никогда не планируй ветку вплотную к техническому потолку **258k**.

## 3. Что вне scope

- Без `dcp_probe` CLI.
- без OV/VF resolving.
- без playout timeline.

## 4. Required reading set

Прочитай только эти файлы до первого изменения:

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/CODE_STYLE.md`
- `specs/objects/CPL.md`
- `specs/objects/Reel.md`
- `specs/objects/TrackFile.md`

Зависимости по плану:
- `T01a`

## 5. Разрешённая поверхность изменений

- `src/dcp/cpl/**`
- `tests/unit/dcp/cpl/**`
- `tests/fixtures/dcp/**`
- `specs/objects/CPL.md`
- `specs/objects/Reel.md`
- `specs/objects/TrackFile.md`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 6. Ожидаемые deliverables

- parse/validation для CPL
- reels/track references
- negative fixtures
- unit tests

## 7. Definition of Done

Парсер выдаёт детерминированный AST/domain object; ссылки на reels и track files представлены явно.

## 8. Верификация

Минимальный набор команд:

```bash
./scripts/build.sh
./scripts/test.sh -R 'cpl|reel'
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
