# dcplayer

Программный DCP player для кинотеатрального программно-аппаратного комплекса.

Корневой путь проекта в production-среде: `/Users/admin/Documents/dcplayer`

## Текущий статус

Это **корректный clean scaffold baseline** проекта:
- репозиторий очищен от build-артефактов и platform-specific мусора;
- branch-sized план и task-specific `AGENTS.md` синхронизированы;
- task names сокращены до формата `TXXx-одно-или-два-слова`;
- companion-specs приведены к каноническому набору;
- build-system готов к первым implementation-веткам без выхода за scope;
- CMake-scaffold конфигурируется, собирается и проходит минимальные unit/integration/smoke tests;
- реальные DCP parser, secure control plane, GPU decode и playout ещё не реализованы.

## Цель продукта

Собрать theater-grade DCP playback appliance с Linux-first MVP, поддержкой:
- Interop и SMPTE DCP;
- open и encrypted DCP;
- secure control plane через Raspberry Pi + ZymKey + RTC и Ubuntu + TPM 2.0;
- pipeline `local storage -> HSM decrypt service -> GPU decode -> GPU picture FM -> playout`;
- software forensic picture & sound watermark;
- supplemental / OV / VF;
- subtitles;
- internal PCM engine с последующим AES3 output;
- минимальным SMS/TMS.

## Secure show mode

```text
DCP ingest
  -> verify
  -> local storage
  -> show authorization
  -> HSM decrypt service (Raspberry Pi + ZymKey + RTC)
  -> GPU JPEG2000 decode
  -> GPU picture forensic watermark
  -> subtitle composite / output stage
  -> projector / protected output

audio:
  local storage
  -> secure authorization
  -> decrypt
  -> audio forensic watermark
  -> PCM engine
  -> later AES3 output
```

## Контрольный канал

```text
Ubuntu media server + TPM 2.0  <== mutual TLS ==>  Raspberry Pi + ZymKey
        client cert in TPM                            server cert in ZymKey
```

## Карта документов

- `AGENTS.md` — глобальные правила работы Codex.
- `docs/ARCHITECTURE.md` — архитектура системы, trust boundary и module map.
- `docs/CODE_STYLE.md` — стиль кода и инженерные соглашения.
- `docs/IMPLEMENT.md` — правила реализации, тестирования и handoff.
- `docs/LIBRARIES.md` — рекомендованные и допустимые зависимости.
- `docs/PLAN.md` — branch-sized план задач.
- `docs/STATUS.md` — фактический статус проекта и очередь следующего шага.
- `specs/objects/` — формальные companion-specs.
- `tasks/INDEX.md` — индекс задач и путей к task-specific `AGENTS.md`.

## Структура репозитория

```text
apps/        пользовательские приложения и сервисы
src/         ядро доменной логики, security, audio/video, playout
spb1/        secure module API / simulator / firmware / diagnostics
docs/        архитектурная и проектная документация
specs/       канонические object contracts
tasks/       task-specific инструкции для branch-sized веток
tests/       unit / integration / fixtures / compliance
scripts/     bootstrap, build, test, smoke, format и repo maintenance
```

## Команды первого запуска

```bash
./scripts/bootstrap.sh
./scripts/build.sh
./scripts/test.sh
./scripts/smoke.sh
```

## Presets

Доступны CMake presets:
- `dev-ninja`
- `dev-make`
- `ci`

Каждый preset использует свой `binaryDir`:
- `build/dev-ninja`
- `build/dev-make`
- `build/ci`

Скрипт `bootstrap.sh` сам выбирает `dev-ninja`, если доступен `ninja`, иначе использует `dev-make`.

## Репозиторные карты

Файлы `TREE.txt` и `manifest.json` считаются **генерируемыми**.
Обновлять их нужно так:

```bash
python3 scripts/update_repo_maps.py
```

## Что делать дальше

Смотреть:
- `docs/STATUS.md` — что реально уже сделано;
- `docs/PLAN.md` — в какой последовательности резать работу;
- `tasks/INDEX.md` — какой `AGENTS.md` соответствует следующей ветке.

На текущем scaffold baseline следующими готовыми к запуску ветками являются `T01a`, `T03a`, `T05a` и `T06a`.
