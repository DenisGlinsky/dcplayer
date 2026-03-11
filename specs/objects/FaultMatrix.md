# FaultMatrix

## Назначение

Явная таблица реакций системы на fault cases.

## Канонические поля

- `matrix_id` — строка
- `fault_cases` — массив case entries
- `default_fail_action` — строка
- `operator_actions` — массив строк

## Инварианты

- Каждый fault case имеет machine-readable reaction.
- Security-critical faults не могут иметь silent ignore action.

## Связи с другими объектами

- Используется T04c, T08c, T08d.

## Каноническая сериализация

- JSON snake_case.

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- нарушен инвариант;
- формат времени, enum или идентификатора невалиден;
- объект противоречит связанным контрактам;
- сериализация недетерминирована.

## Замечания по эволюции

- Обратимо-совместимые расширения добавляются новыми полями.
- Ломающие изменения требуют явного обновления docs/PLAN.md, docs/STATUS.md и связанных task-specific AGENTS.md.
- При изменении security semantics нужно дополнительно проверить trust boundary и audit model.
