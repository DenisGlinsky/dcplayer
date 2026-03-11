# LIBRARIES

Документ фиксирует рекомендуемые и допустимые библиотеки.
Любая новая зависимость должна сначала появиться здесь.

## 1. Политика отбора

Предпочтение получают зависимости:
- с понятной лицензией;
- с активным сопровождением;
- с понятной ролью в архитектуре;
- с fallback strategy;
- без скрытого runtime magic.

## 2. Текущее состояние scaffold

На scaffold baseline фактически используются только:
- CMake;
- C++ standard library;
- Python 3 standard library;
- shell toolchain.

Ни одна product dependency пока не интегрирована в код.

## 3. Планируемые основные библиотеки

| Библиотека | Роль | Статус | Лицензия / режим | Комментарий |
|---|---|---|---|---|
| asdcplib | DCP/MXF low-level I/O | planned-preferred | permissive | Базовый кандидат для ASSETMAP/PKL/CPL/MXF layer. |
| OpenJPEG | CPU JPEG2000 fallback | planned-approved | BSD-2-Clause | Обязательный reference/fallback backend. |
| nvJPEG2000 | NVIDIA GPU JPEG2000 | planned-approved-candidate | vendor SDK terms | Не вендорить в репозиторий. |
| OpenSSL 3 | TLS, X.509, crypto primitives | planned-approved | Apache-2.0 | Базовый TLS/crypto стек. |
| tpm2-openssl | TPM integration for OpenSSL 3 | planned-approved | BSD-3-Clause | Предпочтительный путь для Ubuntu/OpenSSL 3. |
| tpm2-pkcs11 | PKCS#11 bridge to TPM 2.0 | planned-fallback | BSD-2-Clause | Compatibility path. |
| Zymbit SDK / ZymKey APIs | ZymKey integration | planned-required-on-pi | vendor terms | Источник sign / hardware integration на Pi. |
| zkpkcs11 | PKCS#11 integration for ZymKey | planned-candidate | vendor terms | Рассматривать при необходимости упростить TLS workflow. |
| pugixml | XML parsing | planned-approved | MIT | Лёгкий XML parser. |
| xmlsec1 + libxml2 | XMLDSig / XML security | planned-approved-candidate | MIT-family | Для security-sensitive XML операций. |
| nlohmann/json | JSON serialization | planned-approved | MIT | Для CLI reports и diagnostics. |
| spdlog | structured logging | planned-approved | MIT | Operational logs. |
| Catch2 | C++ tests | planned-approved | BSL-1.0 | Возможный вход для unit/integration tests. |

## 4. Reference-only и исключения

| Компонент | Статус | Причина |
|---|---|---|
| libdcp | reference only | Полезна как oracle/tooling, но не в product core. |
| DCP-o-matic | behavioral reference | Внешняя точка сравнения, не embedded dependency. |
| произвольные watermark SDK без фиксированного контракта | excluded | Ломают стабильность object model и task isolation. |

## 5. Правила подключения

1. Сначала описать роль библиотеки в архитектуре.
2. Сначала оценить лицензию.
3. Сначала определить fallback или exit strategy.
4. Не тянуть зависимость “на всякий случай”.
5. Любая vendor SDK изолируется adapter-слоем.

## 6. Security plane

### Ubuntu media server
Предпочтительно:
- OpenSSL 3 + `tpm2-openssl`;
- fallback: `tpm2-pkcs11`.

### Raspberry Pi + ZymKey
Допустимо:
- прямой Zymbit SDK;
- ZymKey OpenSSL integration;
- `zkpkcs11`, если это реально упрощает workflow.
