# IMPLEMENT

## 1. Назначение

Этот документ фиксирует правила реализации и handoff для branch-sized режима.

## 2. Основной workflow

Для каждой ветки:

1. Прочитать `AGENTS.md`.
2. Прочитать `tasks/<TASK-ID>/AGENTS.md`.
3. Прочитать только `Required reading set`.
4. Сделать изменения только в разрешённой области.
5. Прогнать verification-команды.
6. Обновить `docs/STATUS.md`.
7. Обновить companion-specs, если менялись контракты.
8. Подготовить отчёт по `docs/templates/CODEX_REPORT.md`.

## 3. Durable memory

Проектная память не живёт в истории чата.
Она живёт в:
- `docs/PLAN.md`
- `docs/STATUS.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/*.md`
- task-specific `AGENTS.md`

## 4. Scope discipline

Запрещено:
- в одной ветке закрывать несколько несвязанных задач;
- тянуть новые зависимости без отражения в `docs/LIBRARIES.md`;
- менять trust boundary без ADR;
- смешивать picture FM и sound FM задачи без явного указания в task-specific AGENTS.

## 5. Verification policy

Минимально для scaffold и большинства задач:
- `./scripts/bootstrap.sh`
- `./scripts/build.sh`
- `./scripts/test.sh`

Если задача меняет smoke path, дополнительно:
- `./scripts/smoke.sh`

Если меняется repo layout:
- `python3 scripts/update_repo_maps.py`

## 6. Handoff policy

После завершения ветки обязательно обновить:
- `docs/STATUS.md`
- связанные companion-specs
- при необходимости `docs/PLAN.md`
- артефакты проверки в файловом виде

## 7. Stop conditions

Нужно остановиться и поднять вопрос архитекторам, если:
- требуется новая crypto/vendor dependency;
- меняется trust boundary;
- нужен новый subsystem за пределами scope;
- изменение инвариантов тянет много соседних модулей;
- secure path начинает тихо деградировать в open path.

## 8. Scaffold baseline

Текущий baseline уже должен:
- конфигурироваться и собираться;
- иметь хотя бы один реальный CTest;
- иметь stub CLI для smoke-проверок;
- иметь синхронизированные docs, specs и tasks.

Это не product implementation, а корректная стартовая площадка.

## 9. T01a diagnostics baseline

Для `T01a` зафиксирован минимальный детерминированный формат диагностик парсеров `AssetMap` и `PKL`:
- `code` — стабильный машинно-читаемый код;
- `severity` — `error` или `warning`;
- `path` — XPath-like путь до проблемного поля;
- `message` — стабильное человекочитаемое описание без platform-specific деталей.

Правила нормализации `T01a`:
- UUID принимает optional `urn:uuid:` prefix на входе и хранится в lower-case canonical form;
- `issue_date_utc` принимается в форме `YYYY-MM-DDTHH:MM:SSZ`;
- XML parser для `T01a` принимает UTF-8 и UTF-8 BOM;
- XML text/attribute normalization происходит только после полного entity decoding;
- поддерживаются named entities и numeric character references (`&#...;`, `&#x...;`);
- malformed XML entity не декодируется best-effort и поднимает детерминированный `xml_malformed` с path текущего XML context-а;
- path validation в `AssetMap` применяется к уже декодированному значению;
- digest в `PKL` принимает hex/base64 и нормализуется к lower-case hex;
- diagnostics сортируются детерминированно: `error` before `warning`, затем `code`, затем `path`.
