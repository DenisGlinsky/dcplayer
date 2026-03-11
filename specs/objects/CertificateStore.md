# CertificateStore

## Назначение

Локальная модель trust store для secure control plane. Контракт нужен для `T03a` и не должен хранить приватные ключи.

## Канонические поля

- `store_id` — строка
- `store_version` — строка
- `fingerprint_algorithm` — enum `sha256`
- `roots` — массив cert refs
- `intermediates` — массив cert refs
- `device_certs` — массив cert refs
- `revocation_sources` — массив URL/URI
- `updated_at_utc` — UTC ISO-8601

### Cert ref

- `fingerprint` — hex строка
- `subject_dn` — строка
- `issuer_dn` — строка
- `role` — enum `root | intermediate | server | client | signer | unknown`
- `not_before_utc` — UTC ISO-8601
- `not_after_utc` — UTC ISO-8601
- `serial_number` — строка

## Инварианты

- Store не должен содержать приватные ключи.
- Один и тот же fingerprint не должен дублироваться в разных списках с противоречивой ролью.
- `fingerprint_algorithm` фиксирован и одинаков для всех записей store.
- Корневые сертификаты не могут зависеть от отсутствующего issuer внутри store.
- Revocation sources должны быть детерминированно упорядочены.

## Trust anchor rules

- `roots` являются trust anchors.
- `intermediates` и `device_certs` не дают доверия сами по себе без якоря в `roots`.
- Состояние “anchor missing” — отдельный decision class, а не generic parse error.

## Политика диагностики

Минимальные коды для `T03a`:
- `cert_store.duplicate_fingerprint`
- `cert_store.invalid_role`
- `cert_store.missing_trust_anchor`
- `cert_store.invalid_time_window`
- `cert_store.private_key_material_forbidden`
- `cert_store.invalid_revocation_source`

## Связи с другими объектами

- Используется в `TrustChain` и `SecureChannelContract`.
- Обновляется задачей `T03a`.

## Каноническая сериализация

- JSON `snake_case`.
- Сертификаты описываются fingerprint-ами и metadata, а не бинарными blob.

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- нарушен инвариант;
- формат времени, enum или идентификатора невалиден;
- объект противоречит связанным контрактам;
- сериализация недетерминирована.

## Замечания по эволюции

- Обратимо-совместимые расширения добавляются новыми полями.
- Ломающие изменения требуют явного обновления `docs/PLAN.md`, `docs/STATUS.md` и связанных task-specific `AGENTS.md`.
- При изменении security semantics нужно дополнительно проверить trust boundary и audit model.
