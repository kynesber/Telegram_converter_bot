#include "../include/BotHandler.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

// Приводит строку к верхнему регистру. Возвращает новую строку.
std::string toUpper(std::string s) {
  // std::transform применяет функцию к каждому элементу диапазона.
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return s;
}

// Разбивает строку по пробелам и возвращает вектор токенов.
std::vector<std::string> split(const std::string& text) {
  std::istringstream iss(text);
  std::vector<std::string> tokens;
  std::string token;
  while (iss >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

}  // namespace

BotHandler::BotHandler(const std::string& token,
                       std::unordered_set<std::int64_t> allowedUsers)
    : bot_(token), converter_(), allowedUsers_(std::move(allowedUsers)) {
  // В теле конструктора регистрируем все обработчики команд.
  registerHandlers();
}

bool BotHandler::isAllowed(std::int64_t userId) const {
  if (allowedUsers_.empty()) {
    return true;
  }
  // count() возвращает 0 или 1 для множества — удобная проверка вхождения.
  return allowedUsers_.count(userId) > 0;
}

// Регистрация обработчиков
void BotHandler::registerHandlers() {
  auto guarded = [this](auto&& handler) {
    // Возвращаем новую лямбду, которую и отдадим библиотеке как колбэк.
    return [this, handler](TgBot::Message::Ptr message) {
      // message->from может быть nullptr для некоторых системных сообщений —
      // проверяем, чтобы не упасть с сегфолтом.
      if (!message->from || !isAllowed(message->from->id)) {
        reply(message->chat->id, "Извините, у вас нет доступа к этому боту.");
        return;
      }
      handler(message);
    };
  };

  bot_.getEvents().onCommand(
      "start", guarded([this](TgBot::Message::Ptr m) { onStart(m); }));
  bot_.getEvents().onCommand(
      "help", guarded([this](TgBot::Message::Ptr m) { onHelp(m); }));
  bot_.getEvents().onCommand(
      "convert", guarded([this](TgBot::Message::Ptr m) { onConvert(m); }));
  bot_.getEvents().onCommand(
      "rates", guarded([this](TgBot::Message::Ptr m) { onRates(m); }));
  bot_.getEvents().onCommand(
      "currencies",
      guarded([this](TgBot::Message::Ptr m) { onCurrencies(m); }));

  // onAnyMessage вызывается для любого сообщения. Покажем подсказку, если
  // пришёл неопознанный текст.
  bot_.getEvents().onAnyMessage([this](TgBot::Message::Ptr message) {
    if (StringTools::startsWith(message->text, "/")) {
      return;
    }
    reply(message->chat->id,
          "Не понял команду. Наберите /help, чтобы увидеть список команд.");
  });
}

// Обработчики команд

void BotHandler::onStart(TgBot::Message::Ptr message) {
  const std::string text =
      "Привет! Я бот для конвертации валют.\n"
      "Используйте /help, чтобы увидеть доступные команды.";
  reply(message->chat->id, text);
}

void BotHandler::onHelp(TgBot::Message::Ptr message) {
  const std::string text =
      "Доступные команды:\n"
      "/start — приветствие\n"
      "/help — это сообщение\n"
      "/convert <сумма> <из> <в> — например: /convert 100 USD EUR\n"
      "/rates <валюта> — курсы относительно валюты, например: /rates USD\n"
      "/currencies — список поддерживаемых валют";
  reply(message->chat->id, text);
}

void BotHandler::onConvert(TgBot::Message::Ptr message) {
  // Разбиваем сообщение на токены.
  // Ожидаем формат: "/convert 100 USD EUR" — четыре токена.
  const auto tokens = split(message->text);

  if (tokens.size() != 4) {
    reply(message->chat->id, "Неверный формат. Пример: /convert 100 USD EUR");
    return;
  }

  // Пытаемся распарсить сумму. std::stod кидает исключение при ошибке.
  double amount = 0.0;
  try {
    amount = std::stod(tokens[1]);
  } catch (const std::exception&) {
    reply(message->chat->id,
          "Сумма должна быть числом. Пример: /convert 100 USD EUR");
    return;
  }

  if (amount <= 0) {
    reply(message->chat->id, "Сумма должна быть положительной.");
    return;
  }

  const std::string from = toUpper(tokens[2]);
  const std::string to = toUpper(tokens[3]);

  try {
    const double result = converter_.convert(amount, from, to);

    // Форматируем результат с двумя знаками после запятой.
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(2);
    out << amount << " " << from << " = " << result << " " << to;

    reply(message->chat->id, out.str());
  } catch (const std::exception& e) {
    // В случае сетевой ошибки или проблемы с API — сообщаем пользователю.
    reply(message->chat->id, std::string("Ошибка конвертации: ") + e.what());
  }
}

void BotHandler::onRates(TgBot::Message::Ptr message) {
  const auto tokens = split(message->text);
  if (tokens.size() != 2) {
    reply(message->chat->id, "Использование: /rates USD");
    return;
  }

  const std::string base = toUpper(tokens[1]);
  try {
    const std::string info = converter_.getRatesInfo(base);
    reply(message->chat->id, info);
  } catch (const std::exception& e) {
    reply(message->chat->id, std::string("Ошибка: ") + e.what());
  }
}

void BotHandler::onCurrencies(TgBot::Message::Ptr message) {
  try {
    const auto list = converter_.getAvailableCurrencies();

    std::ostringstream out;
    out << "Поддерживаемые валюты (" << list.size() << "):\n";
    for (const auto& c : list) {
      out << c << "\n";
    }
    reply(message->chat->id, out.str());
  } catch (const std::exception& e) {
    reply(message->chat->id, std::string("Ошибка: ") + e.what());
  }
}

// Отправка сообщения

void BotHandler::reply(std::int64_t chatId, const std::string& text) {
  try {
    // bot_.getApi() даёт доступ к низкоуровневым методам Telegram Bot API.
    // sendMessage отправляет текст в чат.
    bot_.getApi().sendMessage(chatId, text);
  } catch (const TgBot::TgException& e) {
    // Логируем ошибку в stderr — ошибка отправки не должна ронять бота.
    std::cerr << "Ошибка отправки сообщения: " << e.what() << std::endl;
  }
}

// Главный цикл

void BotHandler::run() {
  try {
    // getMe() проверяет, что токен валиден, и возвращает сведения о боте.
    std::cout << "Бот запущен как: " << bot_.getApi().getMe()->username
              << std::endl;

    bot_.getApi().deleteWebhook();

    TgBot::TgLongPoll longPoll(bot_);
    while (true) {
      std::cout << "Опрашиваем Telegram..." << std::endl;
      longPoll.start();
    }
  } catch (const TgBot::TgException& e) {
    std::cerr << "Ошибка Telegram API: " << e.what() << std::endl;
    throw;
  }
}