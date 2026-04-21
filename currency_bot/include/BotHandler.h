#pragma once

#include <tgbot/tgbot.h>

#include <cstdint>
#include <string>
#include <unordered_set>

#include "CurrencyConverter.h"

// Класс BotHandler инкапсулирует всю работу с Telegram Bot API.
class BotHandler {
 public:
  // Конструктор принимает токен бота и (опционально) множество разрешённых
  // Telegram user ID. Если множество пустое — боту может писать кто угодно.
  // explicit — запрещает неявное преобразование строки в BotHandler.
  explicit BotHandler(const std::string& token,
                      std::unordered_set<std::int64_t> allowedUsers = {});

  // Запускает основной цикл бота (long-polling). Блокирующий вызов.
  void run();

 private:
  // Регистрирует все команды и обработчики у bot_.
  void registerHandlers();

  // Обработчики отдельных команд. Message::Ptr — это
  // std::shared_ptr<TgBot::Message>.
  void onStart(TgBot::Message::Ptr message);
  void onHelp(TgBot::Message::Ptr message);
  void onConvert(TgBot::Message::Ptr message);
  void onRates(TgBot::Message::Ptr message);
  void onCurrencies(TgBot::Message::Ptr message);

  // Утилита: безопасно отправляет сообщение в чат, логируя ошибки.
  void reply(std::int64_t chatId, const std::string& text);

  // Проверка белого списка пользователей. Возвращает true, если пользователю
  // разрешено.
  bool isAllowed(std::int64_t userId) const;

  // bot_ — это объект TgBot::Bot, который занимается сетью.
  TgBot::Bot bot_;
  // converter_ — наш конвертер валют.
  CurrencyConverter converter_;
  // Белый список пользователей. Пустой — значит «все разрешены».
  std::unordered_set<std::int64_t> allowedUsers_;
};