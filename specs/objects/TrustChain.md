# TrustChain

## Назначение

Результат проверки цепочки доверия и revocation status. Контракт нужен для `T03a`, `T03b` и KDM/security gating.

## Канонические поля

- `subject_fingerprint` — hex строка
- `issuer_chain` — массив fingerprint-ов
- `validation_time_utc` — UTC ISO-8601
- `revocation_status` — enum
- `decision` — enum
- `decision_reason` — enum
- `checked_sources` — массив строк

## Enum baseline

### `revocation_status`
- `good`
- `revoked`
- `unknown`
- `not_checked`

### `decision`
- `trusted`
- `rejected`
- `warning_only`

### `decision_reason`
- `ok`
- `anchor_missing`
- `expired`
- `not_yet_valid`
- `revoked`
- `chain_broken`
- `algorithm_forbidden`
- `revocation_unavailable`

## Инварианты

- Цепочка начинается с subject и заканчивается доверенным root либо explicit failure.
- Время проверки обязательно.
- `decision=trusted` недопустим при `revocation_status=revoked`.
- `warning_only` не разрешён для fail-closed show authorization path.

## Error taxonomy

Минимальные коды для `T03a`:
- `trust_chain.anchor_missing`
- `trust_chain.chain_broken`
- `trust_chain.expired`
- `trust_chain.not_yet_valid`
- `trust_chain.revoked`
- `trust_chain.revocation_unavailable`
- `trust_chain.algorithm_forbidden`

## Связи с другими объектами

- Потребляется mutual-TLS contract и KDM validation.
- Использует `CertificateStore` как source of trust anchors.

## Каноническая сериализация

- JSON `snake_case`.
- Статусы сериализуются строковыми enum.

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
