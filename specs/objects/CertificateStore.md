# CertificateStore

## Назначение

Локальная in-memory модель trust store для security control plane. Контракт нужен для `T03a`, не должен хранить приватные ключи и не описывает ASN.1 / PEM / TLS transport.

## Канонические поля

- `store_id` — строка
- `store_version` — строка
- `fingerprint_algorithm` — enum `sha256`
- `roots` — массив `certificate_record`
- `intermediates` — массив `certificate_record`
- `device_certs` — массив `certificate_record`
- `revocation_sources` — массив URL/URI
- `updated_at_utc` — UTC ISO-8601

### `certificate_record`

- `fingerprint` — hex строка
- `subject_dn` — строка
- `issuer_dn` — строка
- `role` — enum `root | intermediate | server | client | signer | unknown`
- `issuer_role_hint` — optional enum `root | intermediate | unknown`
- `not_before_utc` — UTC ISO-8601
- `not_after_utc` — UTC ISO-8601
- `serial_number` — строка

### Import contract

Минимальный import path для `T03a` принимает:
- metadata store (`store_id`, `store_version`, `updated_at_utc`);
- trust anchors;
- intermediate chain entries;
- device certificates;
- `revocation_sources`;
- optional import-only поле `private_key_material`, которое обязано быть пустым и не попадает в `CertificateStore`.

`revocation_sources` в baseline `T03a` являются configured authority set для revocation evidence. Status из источника вне этого списка не может считаться авторитетным при chain evaluation.

## Инварианты

- Store не должен содержать приватные ключи.
- Один и тот же fingerprint не должен дублироваться в разных списках с противоречивой ролью.
- `fingerprint_algorithm` фиксирован и одинаков для всех записей store.
- Каждый `fingerprint` нормализуется к lower-case hex и должен соответствовать SHA-256 длиной 64 hex-символа.
- `not_before_utc` и `not_after_utc` обязаны быть в форме `YYYY-MM-DDTHH:MM:SSZ`.
- `roots` содержат только `role=root`.
- `intermediates` содержат только `role=intermediate`.
- `device_certs` содержат только `role=server | client | signer`.
- `issuer_role_hint`, если задан, может быть только `root | intermediate`.
- Корневой сертификат baseline `T03a` должен быть self-issued (`subject_dn == issuer_dn`).
- `revocation_sources` нормализуются в unique + lexicographically sorted order.

## Trust anchor rules

- `roots` являются trust anchors.
- `intermediates` и `device_certs` не дают доверия сами по себе без якоря в `roots`.
- Rootless store допустим как объект.
- Для `T03a` `issuer_role_hint` допускается как metadata-only подсказка для deterministic missing-issuer classification без ASN.1/signature logic.
- Direct device certificate с `issuer_role_hint=root` даёт `trust_chain.anchor_missing`, даже если в store есть unrelated roots.

## Политика диагностики

Минимальные коды для `T03a`:
- `cert_store.duplicate_fingerprint`
- `cert_store.invalid_fingerprint`
- `cert_store.invalid_role`
- `cert_store.missing_trust_anchor`
- `cert_store.invalid_time_window`
- `cert_store.private_key_material_forbidden`
- `cert_store.invalid_revocation_source`

Диагностика обязана быть:
- machine-readable;
- детерминированно отсортированной (`error` before `warning`, затем `code`, затем `path`, затем `message`);
- без platform-dependent текста.

## Связи с другими объектами

- Используется в `TrustChain` и `SecureChannelContract`.
- Обновляется задачей `T03a`.

## Каноническая сериализация

- JSON `snake_case`.
- Сертификаты описываются metadata и fingerprint-ами, а не бинарными blob.
- Списки `revocation_sources` сериализуются в уже нормализованном порядке.

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
