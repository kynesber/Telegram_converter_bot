# Минимальная версия CMake. 3.14+ даёт нам FetchContent и современный таргет-ориентированный синтаксис.
cmake_minimum_required(VERSION 3.14)

# Объявляем проект
project(currency_bot VERSION 1.0 LANGUAGES CXX)

# Устанавливаем стандарт C++17 и делаем его обязательным
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # выключаем GNU-расширения — пишем портируемый код

# Включаем предупреждения компилятора
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Поиск зависимостей

# Threads — pthread на Linux. Нужна для внутренней работы tgbot-cpp.
find_package(Threads REQUIRED)

# OpenSSL — для HTTPS-соединений с серверами Telegram.
find_package(OpenSSL REQUIRED)

# Boost — tgbot-cpp требует компоненты system.
find_package(Boost REQUIRED COMPONENTS system)

# CURL — для HTTP-запросов к API курсов валют.
find_package(CURL REQUIRED)

# TgBot — основная библиотека для Telegram Bot API.
# Обычно ставится в систему, и мы ищем её как обычный пакет.
find_package(TgBot REQUIRED)

# nlohmann_json — парсер JSON. Header-only.
find_package(nlohmann_json 3.2.0 REQUIRED)

# Описание исполняемого файла

# Создаём таргет currency_bot, в который входят три .cpp-файла.
add_executable(currency_bot
    src/main.cpp
    src/CurrencyConverter.cpp
    src/BotHandler.cpp
)

# Добавляем путь к заголовочным файлам проекта (папка include).
target_include_directories(currency_bot PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Линкуем все нужные библиотеки.
# PRIVATE — значит, что эти зависимости нужны только для сборки данного таргета и не пробрасываются дальше.
target_link_libraries(currency_bot PRIVATE
    TgBot  # библиотека Telegram-бота
    CURL::libcurl  # libcurl для HTTP
    OpenSSL::SSL OpenSSL::Crypto  # TLS/SSL
    Boost::system  # Boost.System
    Threads::Threads  # потоки
    nlohmann_json::nlohmann_json  # JSON-парсер
)