# scripts

Здесь лежат минимальные локальные команды для Codex и разработчиков.

- `bootstrap.sh` — конфигурация CMake-сборки с выбором подходящего preset.
- `build.sh` — сборка активного preset-профиля.
- `test.sh` — запуск CTest для активного preset-профиля.
- `format.sh` — форматирование C/C++ исходников через `clang-format`.
- `smoke.sh` — конфигурация, сборка, тесты и запуск stub-приложений.
- `update_repo_maps.py` — регенерация `TREE.txt` и `manifest.json`.

Скрипты не требуют внешних зависимостей сверх стандартного toolchain и Python 3.
