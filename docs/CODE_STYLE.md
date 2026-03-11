# CODE_STYLE

## 1. Общий принцип

Код в `dcplayer` должен быть читаемым, детерминированным и безопасным по умолчанию.
Мы не оптимизируем раньше времени и не скрываем сложность за магией.

## 2. Языки и их роли

- **C++20** — основной язык real-time и domain logic.
- **C** — только если этого требует низкоуровневая интеграция.
- **Python 3** — tooling, simulator glue, TMS prototype tasks, тестовые harnesses.
- **Shell** — только для bootstrap/build/test helpers.

Нельзя дублировать бизнес-логику одновременно на C++ и Python.

## 3. Нейминг

### 3.1 Файлы и каталоги
- lower_snake_case
- имя файла должно совпадать с основным объектом внутри

Примеры:
- `composition_graph.hpp`
- `security_module_session.cpp`
- `watermark_payload.hpp`

### 3.2 Пространства имён
- lower_snake_case
- короткие и доменные

Пример:
- `dcplayer::dcp`
- `dcplayer::security`
- `dcplayer::watermark`

### 3.3 Типы
- `UpperCamelCase`

Пример:
- `CompositionGraph`
- `SecurityModuleSession`

### 3.4 Функции и переменные
- `lower_snake_case`

Пример:
- `resolve_supplemental_bindings()`
- `show_window_utc`

### 3.5 Поля классов
- `lower_snake_case_` с завершающим `_`

### 3.6 Константы
- `kUpperCamelCase` для compile-time constants
- `UPPER_SNAKE_CASE` только для macros/header guards

## 4. C++ правила

1. Никакого `using namespace` в header-файлах.
2. Никаких raw `new` / `delete` в application code.
3. Предпочитать:
   - `std::unique_ptr`
   - `std::shared_ptr` только при реальной shared ownership
   - `std::span`
   - `std::array`
   - `std::optional`
   - `std::string_view`
4. Предпочитать RAII для:
   - файлов;
   - TLS handles;
   - GPU buffers;
   - mutex/locks;
   - zeroization guards.
5. Исключения не должны пересекать real-time и межмодульные границы.
   На этих границах использовать явный `Result/Status` pattern.
6. Header-файлы не должны тянуть тяжёлые зависимости без крайней необходимости.
7. В header-файлах — только то, что нужно для контракта.

## 5. Include order

Порядок include-блоков:
1. собственный header;
2. проектные headers;
3. third-party headers;
4. standard library.

Между группами — пустая строка.

## 6. Ошибки и статусы

Ошибки делятся на:
- `ValidationError`
- `SecurityError`
- `IoError`
- `DecodeError`
- `SyncError`
- `ConfigurationError`

Требования:
- ошибка должна быть машинно-классифицируемой;
- сообщение должно содержать доменный контекст;
- security-related ошибки не должны раскрывать секреты.

## 7. Concurrency и real-time дисциплина

1. Никаких неочевидных глобальных mutable singletons.
2. Для shared state обязателен явный owner.
3. Lock order документируется.
4. RT path не должен делать:
   - блокирующий DNS;
   - файловые сканы;
   - динамическую загрузку конфигурации;
   - вывод огромных логов.
5. Каждая очередь должна иметь:
   - capacity;
   - policy при overflow;
   - metrics;
   - test на backpressure.

## 8. GPU code

- Backend interfaces в `src/video/*` и `src/watermark/*` обязаны быть стабильными.
- GPU-specific код не должен протекать в доменную модель.
- Любой device buffer должен иметь clear ownership.
- Никаких implicit sync без комментария и теста.
- Любой optimized kernel обязан иметь reference path.

## 9. Python style

- PEP 8.
- type hints обязательны.
- `pathlib` вместо строковых путей там, где возможно.
- никаких side effects при import.
- CLI-инструменты должны возвращать правильный exit code.

## 10. Логирование

1. Логи должны быть структурированными.
2. Секреты, plaintext essence, ключи, pin-коды и KDM body в лог не попадают.
3. Security events выделяются отдельно от operational logs.
4. Сообщения формулируются так, чтобы по ним можно было отлаживать, но нельзя было утечь чувствительные детали.

## 11. Тесты

Именование:
- `*_test.cpp`
- `test_*.py`

Для каждого нового модуля как минимум:
- happy path;
- malformed input;
- security/authorization failure;
- boundary conditions;
- determinism / repeatability.

## 12. Комментарии

Комментарии нужны там, где код неочевиден.
Нельзя писать комментарий, который просто пересказывает строку кода.

Хороший комментарий объясняет:
- почему так;
- на какой инвариант опирается код;
- где trust boundary;
- какой fallback ожидается.

## 13. Markdown и документация

- Русский язык для проектной документации.
- Термины API, имена файлов, классов и протоколов — на английском.
- Любое изменение объекта требует обновления соответствующей companion-спецификации.
- Любое изменение task scope требует обновления task-specific `AGENTS.md`.

## 14. Что считается style violation

- скрытые side effects;
- неявные ownership transitions;
- немые catch-all блоки;
- функции на сотни строк без декомпозиции;
- тип `bool` вместо явного enum/state;
- hardcoded paths и magic numbers без named constant;
- логирование секрета;
- бизнес-логика внутри UI слоя.
