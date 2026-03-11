# J2KBackendContract

## Назначение

Стабильный контракт backend-а JPEG2000 decode. Контракт нужен для `T05a` и отделяет playout core от конкретных CPU/GPU реализаций.

## Канонические поля

- `backend_id` — строка
- `backend_class` — enum
- `memory_domain` — enum
- `supported_profiles` — массив строк
- `supported_pixel_formats` — массив enum
- `async_submission` — bool
- `perf_counters` — массив строк
- `fallback_backend_id` — строка или `null`

## Enum baseline

### `backend_class`
- `cpu_reference`
- `gpu_open`
- `gpu_secure`
- `simulator`

### `memory_domain`
- `host_ram`
- `cuda_device`
- `opencl_device`
- `rocm_device`
- `metal_device`
- `unknown`

### `supported_pixel_formats`
- `xyz12_packed`
- `xyz16_planar`
- `rgba16`
- `unknown`

## Инварианты

- Каждый production backend имеет fallback path.
- Контракт не протекает в доменную модель.
- `gpu_secure` backend не может публиковать clear output за пределы trust boundary без явного secure adapter.
- Список perf counters детерминирован.

## Failure classes

- `unsupported_profile`
- `unsupported_surface`
- `device_unavailable`
- `queue_timeout`
- `corrupt_codestream`
- `internal_backend_error`

## Связи с другими объектами

- Используется `T05a–T05d` и secure playback integration.
- Совместим с `FrameSurface`.

## Каноническая сериализация

- JSON `snake_case`.

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
