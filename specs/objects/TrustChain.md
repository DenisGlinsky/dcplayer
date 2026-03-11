# TrustChain

## Назначение

Результат baseline-проверки цепочки доверия и revocation status для security control plane. Контракт нужен для `T03a`, `T03b` и KDM/security gating.

## Канонические поля

- `subject_fingerprint` — hex строка
- `issuer_chain` — массив fingerprint-ов
- `validation_time_utc` — UTC ISO-8601
- `revocation_status` — enum
- `decision` — enum
- `decision_reason` — enum
- `checked_sources` — массив строк

### Evaluation request contract

Минимальный request для `T03a` содержит:
- `CertificateStore`;
- `subject_fingerprint`;
- `validation_time_utc`;
- optional `checked_sources`;
- in-memory `revocation_entries`;
- `revocation_policy`.

`T03a` intentionally не включает:
- TLS handshake;
- signature verification по реальным ASN.1 chain blobs;
- OCSP/CRL network fetch;
- secure clock discipline.

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
- `unsupported_certificate_role`
- `invalid_revocation_state`
- `invalid_validation_time`

### `revocation_policy`
- `require_good`
- `allow_unknown`

## Инварианты

- Цепочка начинается с subject и заканчивается доверенным root либо explicit failure.
- Время проверки обязательно.
- `decision=trusted` недопустим при `revocation_status=revoked`.
- `warning_only` не разрешён для fail-closed show authorization path.
- `issuer_chain` детерминированно отражает фактически разрешённые issuer fingerprint-ы вплоть до trust anchor.
- `checked_sources` сериализуются в детерминированном порядке.
- revocation evidence считается авторитетным только если его `source` входит в `CertificateStore.revocation_sources`, а при непустом request `checked_sources` ещё и в этот поднабор.

## Error taxonomy

Минимальные коды для `T03a`:
- `trust_chain.anchor_missing`
- `trust_chain.chain_broken`
- `trust_chain.expired`
- `trust_chain.not_yet_valid`
- `trust_chain.revoked`
- `trust_chain.revocation_unavailable`
- `trust_chain.algorithm_forbidden`
- `trust_chain.unsupported_certificate_role`
- `trust_chain.invalid_revocation_state`
- `trust_chain.unknown_revocation_source`
- `trust_chain.invalid_validation_time`

Семантика `T03a`:
- `anchor_missing` — issuer path дошёл до границы доверия, но trust anchor отсутствует в `CertificateStore.roots`;
- `chain_broken` — missing/ambiguous/cyclic issuer link до достижения границы доверия;
- `revocation_unavailable` — configured revocation policy не смогла получить достаточное состояние для всей цепочки;
- `invalid_revocation_state` — revocation entry противоречит baseline enum/contract.
- `unknown_revocation_source` — revocation entry ссылается на источник вне configured authority set или вне request `checked_sources`; такая entry не считается authoritative evidence, не должна сама по себе удовлетворять `require_good` и должна оставаться наблюдаемой в diagnostics даже при успешном trust decision по другому authoritative evidence.
- `invalid_validation_time` — request `validation_time_utc` не соответствует строгому `YYYY-MM-DDTHH:MM:SSZ`.

Классификация missing issuer в `T03a` остаётся узкой и детерминированной:
- `anchor_missing` используется для missing issuer после intermediate и для direct device certificate с `issuer_role_hint=root`;
- self-issued non-root certificate не считается missing trust anchor и возвращает `chain_broken`, даже если `issuer_role_hint=root`;
- остальные missing issuer cases, которые нельзя надёжно отнести к absent trust anchor по текущему baseline contract, возвращают `chain_broken`.

Поведение revocation parsing в `T03a`:
- malformed/contradicting revocation entry даёт `invalid_revocation_state` и завершает evaluation fail-closed;
- non-authoritative entries при этом всё равно остаются наблюдаемыми через `unknown_revocation_source`, если они присутствуют в request.

## Связи с другими объектами

- Потребляется mutual-TLS contract и KDM validation.
- Использует `CertificateStore` как source of trust anchors.

## Каноническая сериализация

- JSON `snake_case`.
- Статусы сериализуются строковыми enum.
- Диагностика evaluation serializes separately и использует тот же детерминированный порядок, что и другие validation-layer ветки.

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
