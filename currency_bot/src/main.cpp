#include <cstdlib>
#include <iostream>
#include <string>

#include "../include/BotHandler.h"

int main() {
  // std::getenv возвращает значение переменной окружения или nullptr, если её
  // нет.
  const char* tokenEnv = std::getenv("BOT_TOKEN");
  if (tokenEnv == nullptr || std::string(tokenEnv).empty()) {
    std::cerr << "Ошибка: переменная окружения BOT_TOKEN не задана.\n"
              << "Получите токен у @BotFather в Telegram и запустите:\n"
              << "  export BOT_TOKEN=ваш_токен\n"
              << "  ./currency_bot\n";
    return 1;
  }

  const std::string token{tokenEnv};

  try {
    BotHandler bot(token);
    bot.run();  // блокирующий вызов; вернётся только при серьёзной ошибке
  } catch (const std::exception& e) {
    std::cerr << "Критическая ошибка: " << e.what() << std::endl;
    return 2;
  }

  return 0;
}