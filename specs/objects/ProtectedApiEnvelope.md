# ProtectedApiEnvelope

## Назначение

Единый envelope для security-sensitive API вызовов.

## Канонические поля

- `request_id` — UUID строка
- `caller_identity` — строка
- `action` — строковый enum
- `policy_tags` — массив строк
- `audit_context` — объект

## Инварианты

- Envelope не содержит plaintext essence и приватные ключи.
- action и policy_tags достаточны для ACL decision.

## Связи с другими объектами

- Используется secure API /sign, /unwrap, /decrypt.
- Пишется в SecurityEvent.

## Каноническая сериализация

- JSON snake_case.
- Payload бизнес-данных хранится отдельно от envelope metadata.

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
