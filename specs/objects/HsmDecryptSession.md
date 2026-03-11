# HsmDecryptSession

## Назначение

Состояние отдельной decrypt session между host и secure module.

## Канонические поля

- `session_id` — UUID строка
- `show_id` — строка
- `composition_id` — UUID строка
- `key_scope` — строка
- `state` — enum

## Инварианты

- Сессия привязана к одному show/composition.
- State machine fail-closed.

## Связи с другими объектами

- Используется в secure playout и GPU Contract.

## Каноническая сериализация

- JSON snake_case.
- State сериализуется строкой.

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
