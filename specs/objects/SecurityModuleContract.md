# SecurityModuleContract

## Назначение

Host-side контракт обращения к secure module `pi_zymkey` без привязки к реальному transport.

На этапе `T03b` контракт фиксирует:
- module identity;
- ссылку на secure channel contract;
- baseline protected APIs;
- envelope references;
- server identity model.

## Канонические поля

- `module_id` — строка
- `module_role` — enum `pi_zymkey`
- `secure_channel_id` — строка
- `request_envelope_ref` — строка
- `response_envelope_ref` — строка
- `supported_api_names` — массив enum `sign | unwrap | decrypt | health | identity`
- `server_identity` — `PeerIdentity`

## Инварианты

- `module_role` фиксирован как `pi_zymkey`.
- `server_identity.role == module_role`.
- в текущем `T03b` model scope `secure_channel_id` должен быть baseline ref `spb1.control.v1`.
- в текущем `T03b` model scope `request_envelope_ref == protected_request.v1`.
- в текущем `T03b` model scope `response_envelope_ref == protected_response.v1`.
- `request_envelope_ref` и `response_envelope_ref` обязаны ссылаться на один baseline protected-envelope family и одну версию.
- `supported_api_names` сериализуются детерминированно, не раскрывают секреты реализации и не допускают повторов.

`T03b` сознательно не вводит registry/transport binding, поэтому ref-поля валидируются по документированному baseline family, а не по runtime lookup.

## Baseline behavior

- `sign` — contract-level placeholder для будущего sign path;
- `unwrap` — contract-level placeholder для unwrap path;
- `decrypt` — contract-level placeholder для decrypt path;
- `health` — identity/availability probe без runtime transport semantics;
- `identity` — возврат module identity summary без key material.

## Связи с другими объектами

- Опирается на `SecureChannelContract`.
- Использует `ProtectedApiEnvelope`.
- Дальнейшие runtime integrations (`T03c/T04+`) должны потреблять этот контракт, а не вводить новый скрытый API.

## Каноническая сериализация

- JSON `snake_case`
- строковые enum
- массивы `supported_api_names` детерминированно отсортированы

## Минимальная диагностика

- `secure_channel.invalid_server_role`
- `secure_channel.role_mismatch`
- `secure_channel.invalid_api_name`
- `secure_channel.duplicate_supported_api_name`
- `secure_channel.invalid_secure_channel_ref`
- `secure_channel.invalid_request_envelope_ref`
- `secure_channel.invalid_response_envelope_ref`
- `secure_channel.missing_field`
- `secure_channel.invalid_type`

## Причины отклонения / ошибки

- отсутствует обязательное поле;
- server identity не совпадает с module role;
- ссылка на secure channel/envelope family отсутствует;
- сериализация недетерминирована.

## Замечания по эволюции

- Реальный transport и device I/O добавляются только поверх этого host-side контракта.
- Любое изменение baseline API surface требует обновления:
  - `SecureChannelContract.md`
  - `ProtectedApiEnvelope.md`
  - `docs/IMPLEMENT.md`
  - `docs/STATUS.md`
