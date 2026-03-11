# SecureChannelContract

## Назначение

Формальный контракт secure channel между Ubuntu+TPM и Pi+ZymKey.

## Канонические поля

- `channel_id` — строка
- `server_role` — строка
- `client_role` — строка
- `tls_profile` — строка
- `revocation_policy` — строка
- `acl_scope` — массив allowed actions

## Инварианты

- Серверная роль фиксирована как Pi+ZymKey.
- Клиентская роль фиксирована как Ubuntu media server + TPM.
- Mutual-auth обязателен.

## Связи с другими объектами

- Опирается на CertificateStore и TrustChain.
- Ограничивает ProtectedApiEnvelope.

## Каноническая сериализация

- JSON snake_case.
- Policy поля детерминированы и сравнимы по diff.

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
