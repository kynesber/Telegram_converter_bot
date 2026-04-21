# Converter Bot — Telegram-бот для конвертации валют на C++

Небольшой Telegram-бот на C++17, который умеет конвертировать валюты, показывать курсы и список поддерживаемых валют. Данные берёт у бесплатного открытого сервиса [Frankfurter](https://www.frankfurter.app) (на базе курсов ЕЦБ).

> **Важно про список валют.** Frankfurter использует референсные курсы Европейского Центрального Банка. ЕЦБ публикует около 30 мировых валют: USD, EUR, GBP, JPY, CHF, CNY, AUD, CAD, NOK, SEK, PLN, CZK, HUF, TRY и т. д. **RUB, BYN, UAH, KZT в этом списке отсутствуют** — такие запросы вернут ошибку. Полный актуальный список валют можно получить командой `/currencies` у самого бота или прямо из браузера: <https://api.frankfurter.app/currencies>.
## Используемые технологии

- **C++17** — язык программирования.
- **CMake 3.14+** — система сборки.
- **[tgbot-cpp](https://github.com/reo7sp/tgbot-cpp)** — C++-обёртка над Telegram Bot API.
- **libcurl** — HTTP-клиент для запросов к API курсов.
- **[nlohmann/json](https://github.com/nlohmann/json)** — header-only JSON-парсер.
- **OpenSSL** — TLS/SSL для HTTPS.
- **Boost (system)** — зависимость tgbot-cpp.
- **Frankfurter API** — источник курсов валют, без ключа.

## Структура проекта

```
currency_bot/
├── CMakeLists.txt          # конфигурация сборки CMake
├── Dockerfile              # многостадийный образ (builder + runtime)
├── docker-compose.yml      # запуск в один контейнер с автоперезапуском
├── .dockerignore           # что не включать в Docker-контекст
├── .env.example            # шаблон переменных окружения
├── .gitignore              # защищает .env и build/ от коммитов
├── README.md
├── include/
│   ├── BotHandler.h        # заголовок класса обработчика Telegram-команд
│   └── CurrencyConverter.h # заголовок класса конвертера валют
├── src/
│   ├── main.cpp            # точка входа, чтение BOT_TOKEN / ALLOWED_USERS
│   ├── BotHandler.cpp      # регистрация команд, обработка сообщений
│   └── CurrencyConverter.cpp # HTTP-запросы к API курсов, парсинг JSON
└── .github/workflows/
    └── ci.yml              # CI/CD через GitHub Actions
```

## Команды бота

| Команда | Описание | Пример |
|--------|----------|--------|
| `/start` | Приветствие | `/start` |
| `/help` | Список команд | `/help` |
| `/convert <сумма> <из> <в>` | Конвертация | `/convert 100 USD EUR` |
| `/rates <валюта>` | Курсы относительно валюты | `/rates USD` |
| `/currencies` | Поддерживаемые валюты | `/currencies` |

## Установка зависимостей

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install -y build-essential cmake git \
    libboost-system-dev libssl-dev libcurl4-openssl-dev \
    nlohmann-json3-dev zlib1g-dev

# Устанавливаем tgbot-cpp из исходников
git clone https://github.com/reo7sp/tgbot-cpp.git
cd tgbot-cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build
cd ..
```

### macOS (Homebrew)

```bash
brew install cmake boost openssl curl nlohmann-json
git clone https://github.com/reo7sp/tgbot-cpp.git
cd tgbot-cpp
cmake -S . -B build -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
cmake --build build -j
sudo cmake --install build
```

### Windows

Рекомендуется использовать [vcpkg](https://github.com/microsoft/vcpkg):

```powershell
vcpkg install boost-system openssl curl nlohmann-json tgbot-cpp
```

## Получение токена бота

1. Откройте Telegram и напишите боту **[@BotFather](https://t.me/BotFather)**.
2. Отправьте команду `/newbot` и следуйте инструкциям.
3. Получите токен вида `1234567890:AAH...`.
4. Сохраните его — это секрет, не публикуйте в коде.

## Сборка проекта

Из корня проекта:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

После успешной сборки появится исполняемый файл `build/currency_bot`.

## Запуск

Передаём токен через переменную окружения, а не аргумент командной строки — так безопаснее.

```bash
export BOT_TOKEN="ваш_токен_от_BotFather"
./build/currency_bot
```

На Windows (PowerShell):

```powershell
$env:BOT_TOKEN = "ваш_токен_от_BotFather"
.\build\Release\currency_bot.exe
```

При успехе в консоли появится сообщение:

```
Бот запущен как: <имя_вашего_бота>
Опрашиваем Telegram...
```

Откройте Telegram, найдите своего бота по имени и напишите ему `/start`.

## Архитектура

```
┌─────────────────┐  long-poll   ┌──────────────┐
│  Telegram API   │◀────────────▶│  BotHandler  │
└─────────────────┘              │  (tgbot-cpp) │
                                 └──────┬───────┘
                                        │ вызывает
                                        ▼
                                 ┌────────────────────┐    HTTPS   ┌──────────────────┐
                                 │ CurrencyConverter  │◀──────────▶│ Frankfurter API  │
                                 │ (libcurl + json)   │            └──────────────────┘
                                 └────────────────────┘
```

Разделение ответственности:
- `BotHandler` знает только про Telegram: парсит команды, отвечает пользователю.
- `CurrencyConverter` знает только про API курсов: ходит в сеть, парсит JSON.
- `main.cpp` — связывает их.

## Запуск в Docker

Docker-образ собирается в две стадии (builder + runtime) — итоговый образ маленький, без компилятора.

### Локально через docker compose

```bash
# 1. Скопируйте .env.example в .env и впишите свой токен
cp .env.example .env
nano .env           # BOT_TOKEN=...   ALLOWED_USERS=...(опционально)

# 2. Соберите и запустите
docker compose up -d --build

# 3. Посмотреть логи
docker compose logs -f

# 4. Остановить
docker compose down
```

## Лицензия

MIT (свободное использование).