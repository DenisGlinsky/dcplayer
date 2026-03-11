# FrameSurface

## Назначение

Описание поверхности кадра после decode/marking. Контракт используется на границе decode, watermark и playout.

## Канонические поля

- `surface_id` — строка
- `width` — целое > 0
- `height` — целое > 0
- `pixel_format` — строка
- `color_space` — строка
- `bit_depth` — целое > 0
- `memory_domain` — enum
- `row_stride_bytes` — целое > 0
- `plane_count` — целое >= 1
- `owner` — enum

## Enum baseline

### `memory_domain`
- `host_ram`
- `cuda_device`
- `opencl_device`
- `rocm_device`
- `metal_device`
- `unknown`

### `owner`
- `decoder`
- `watermark`
- `playout`
- `external_adapter`

## Инварианты

- Размеры положительны.
- `memory_domain` совместим с backend contract.
- Передача владения должна быть явной; implicit copy semantics запрещены.
- `row_stride_bytes` должен быть совместим с `pixel_format` и шириной.

## Lifetime semantics

- `owner` определяет, кто обязан освободить surface.
- Если `memory_domain != host_ram`, host-side доступ разрешён только через explicit map/unmap semantics.

## Связи с другими объектами

- Используется decode и watermark stack.
- Совместим с `J2KBackendContract`.

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
