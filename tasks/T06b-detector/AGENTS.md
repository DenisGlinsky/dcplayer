# AGENTS.md — T06b Detector

Этот файл ограничивает одну ветку Codex одной инженерной целью и одной поверхностью приёмки. Перед первым изменением прочитай только Required reading set. Не расширяй scope за пределы этой задачи.

## 1. Цель ветки

Сделать offline harness для проверки извлекаемости watermark после контролируемых деградаций.

## 2. Контекстный бюджет

- Плановый класс ветки: **M**.
- Целевая зона: **60k–120k**.
- Жёлтая зона: **120k–180k**.
- Красная зона: **180k–220k**. Если видишь, что ветка туда уходит, режь scope и оставляй handoff.
- Никогда не планируй ветку вплотную к техническому потолку **258k**.

## 3. Что вне scope

- Без picture FM insertion.
- без secure playout orchestration.
- без operator UI.

## 4. Required reading set

Прочитай только эти файлы до первого изменения:

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/WatermarkPayload.md`
- `specs/objects/WatermarkEvidence.md`
- `specs/objects/DetectorReport.md`
- `docs/STATUS.md`

Зависимости по плану:
- `T06a`

## 5. Разрешённая поверхность изменений

- `src/watermark/detector_harness/**`
- `tests/integration/watermark/detector/**`
- `specs/objects/DetectorReport.md`
- `specs/objects/WatermarkEvidence.md`
- `docs/IMPLEMENT.md`
- `docs/STATUS.md`

## 6. Ожидаемые deliverables

- offline detector harness
- attack corpus runner
- scoring/report format

## 7. Definition of Done

Есть воспроизводимый прогон по attack matrix и единый формат отчёта по детекции.

## 8. Верификация

Минимальный набор команд:

```bash
./scripts/build.sh
./scripts/test.sh -R 'detector|watermark'
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

## 12. Правило для watermark

- Не обещай '100% идентификацию' вне утверждённой attack matrix.
- Не смешивай picture FM и sound FM в одной ветке, если это не указано явно в цели.
- Любое изменение payload/state/evidence сначала отражай в companion-spec.
