#pragma once

#include <string>
#include <vector>

// Этот класс отвечает за обращение к внешнему API и математику конвертации.
class CurrencyConverter {
 public:
  // Конструктор по умолчанию. Инициализирует глобальное состояние libcurl.
  CurrencyConverter();

  // Деструктор. Освобождает глобальные ресурсы libcurl.
  ~CurrencyConverter();

  // Конвертирует `amount` валюты `from` в валюту `to`.
  // Функция возвращает значение типа double (вещественное число).
  double convert(double amount, const std::string& from, const std::string& to);

  // Возвращает красиво отформатированную строку с курсами, основанную на
  // базовой валюте.
  std::string getRatesInfo(const std::string& base);

  // Возвращает список доступных валют (коды ISO 4217: USD, EUR, RUB и т.д.).
  std::vector<std::string> getAvailableCurrencies();

 private:
  // Приватный хелпер: выполняет HTTP GET и возвращает тело ответа как строку.
  std::string makeHttpRequest(const std::string& url);

  // Callback для libcurl: вызывается по мере поступления данных.
  // Мы копим пришедшие байты в строку, указатель на которую передаётся как
  // user-data.
  static size_t writeCallback(void* contents, size_t size, size_t nmemb,
                              std::string* output);
};