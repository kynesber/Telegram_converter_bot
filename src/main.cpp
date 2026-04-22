#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

#include "../include/BotHandler.h"

static std::unordered_set<std::int64_t> parseAllowedUsers(const char* raw) {
  std::unordered_set<std::int64_t> result;
  if (raw == nullptr || *raw == '\0') {
    return result;
  }

  std::stringstream ss(raw);
  std::string item;
  // getline(..., ',') читает токены, разделённые запятой.
  while (std::getline(ss, item, ',')) {
    while (!item.empty() &&
           std::isspace(static_cast<unsigned char>(item.front()))) {
      item.erase(item.begin());
    }
    while (!item.empty() &&
           std::isspace(static_cast<unsigned char>(item.back()))) {
      item.pop_back();
    }
    if (item.empty()) {
      continue;
    }
    try {
      result.insert(static_cast<std::int64_t>(std::stoll(item)));
    } catch (const std::exception&) {
      std::cerr << "Пропускаем некорректный ID: " << item << std::endl;
    }
  }
  return result;
}

int main() {
  const char* tokenEnv = std::getenv("BOT_TOKEN");
  if (tokenEnv == nullptr || std::string(tokenEnv).empty()) {
    std::cerr << "Ошибка: переменная окружения BOT_TOKEN не задана.\n"
              << "Получите токен у @BotFather в Telegram и запустите:\n"
              << "  export BOT_TOKEN=ваш_токен\n"
              << "  ./currency_bot\n";
    return 1;
  }

  // Опциональная переменная ALLOWED_USERS — через запятую, без пробелов или с
  // ними. Пример: ALLOWED_USERS="123456,789012"
  auto allowedUsers = parseAllowedUsers(std::getenv("ALLOWED_USERS"));
  if (allowedUsers.empty()) {
    std::cout << "ALLOWED_USERS не задан — бот открыт для всех.\n";
  } else {
    std::cout << "Доступ разрешён " << allowedUsers.size()
              << " пользователю(ям).\n";
  }

  const std::string token{tokenEnv};

  try {
    BotHandler bot(token, std::move(allowedUsers));
    bot.run();
  } catch (const std::exception& e) {
    std::cerr << "Критическая ошибка: " << e.what() << std::endl;
    return 2;
  }

  return 0;
}