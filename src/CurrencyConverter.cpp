#include "../include/CurrencyConverter.h"

#include <curl/curl.h>

#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

// Псевдоним типа: вместо nlohmann::json будем писать просто json.
using json = nlohmann::json;

// Конструктор и деструктор

// Конструктор инициализирует libcurl глобально.
// CURL_GLOBAL_DEFAULT включает все стандартные возможности, включая SSL.
CurrencyConverter::CurrencyConverter() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

// Деструктор освобождает глобальные ресурсы libcurl.
CurrencyConverter::~CurrencyConverter() { curl_global_cleanup(); }

// Callback для получения данных от libcurl

// libcurl по мере получения данных вызывает эту функцию порциями.
// size * nmemb — реальный размер порции в байтах.
// output — указатель на нашу строку, куда мы добавляем данные.
// Мы должны вернуть число обработанных байтов — если оно меньше реального,
// libcurl посчитает это ошибкой.
size_t CurrencyConverter::writeCallback(void* contents, size_t size,
                                        size_t nmemb, std::string* output) {
  const size_t totalSize = size * nmemb;

  // static_cast<char*> — безопасное приведение типов: void* -> char*.
  output->append(static_cast<char*>(contents), totalSize);

  return totalSize;
}

// Выполнение HTTP-запроса

std::string CurrencyConverter::makeHttpRequest(const std::string& url) {
  // Создаём handle — дескриптор CURL.
  CURL* curl = curl_easy_init();
  if (!curl) {
    // Если libcurl не смогла инициализироваться — бросаем исключение.
    throw std::runtime_error("Не удалось инициализировать libcurl");
  }

  std::string response;  // сюда накопится тело ответа

  // Настраиваем параметры запроса через curl_easy_setopt.
  // CURLOPT_URL — целевой URL (C-строка, поэтому .c_str()).
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  // CURLOPT_WRITEFUNCTION — колбэк записи данных.
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  // CURLOPT_WRITEDATA — указатель, который будет передан в колбэк четвёртым
  // параметром.
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  // Следуем за HTTP-редиректами (3xx).
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  // Таймаут в 10 секунд, чтобы не подвесить бота.
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  // Проверяем SSL-сертификат сервера (безопасность).
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
  // User-Agent: некоторые серверы обязательно ждут его наличия.
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "currency-bot/1.0");

  // Выполняем сам запрос (блокирующий вызов).
  const CURLcode res = curl_easy_perform(curl);

  // Обязательно освобождаем дескриптор, даже при ошибке.
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    // curl_easy_strerror переводит код ошибки в человекочитаемую строку.
    throw std::runtime_error(std::string("HTTP-запрос не удался: ") +
                             curl_easy_strerror(res));
  }

  return response;
}

// Публичный метод: конвертация

double CurrencyConverter::convert(double amount, const std::string& from,
                                  const std::string& to) {
  // Если валюты совпадают — просто возвращаем исходную сумму, не делая запрос.
  if (from == to) {
    return amount;
  }

  // Формируем URL запроса. Используем std::ostringstream — удобный способ
  // собирать строки.
  std::ostringstream url;
  url << "https://api.frankfurter.app/latest"
      << "?amount=" << amount << "&from=" << from << "&to=" << to;

  // Выполняем запрос и получаем JSON.
  const std::string body = makeHttpRequest(url.str());

  // Парсим JSON. Если он некорректный, json::parse бросит исключение.
  const json data = json::parse(body);

  // Проверяем наличие ожидаемых полей.
  // Если API вернул ошибку (например, неверный код валюты), поля "rates" может
  // не быть.
  if (!data.contains("rates") || !data["rates"].contains(to)) {
    throw std::runtime_error("API не вернул курс для валюты " + to +
                             ". Проверьте корректность кодов валют.");
  }

  // Извлекаем значение как double и возвращаем.
  return data["rates"][to].get<double>();
}

// Публичный метод: сводка курсов

std::string CurrencyConverter::getRatesInfo(const std::string& base) {
  // Получаем все курсы относительно базовой валюты.
  const std::string url = "https://api.frankfurter.app/latest?from=" + base;
  const std::string body = makeHttpRequest(url);
  const json data = json::parse(body);

  if (!data.contains("rates")) {
    throw std::runtime_error("Не удалось получить сводку курсов");
  }

  // Собираем красивый текст.
  std::ostringstream result;
  result << "Курсы относительно 1 " << base << ":\n";
  result << std::fixed << std::setprecision(4);  // 4 знака после запятой

  // Диапазонный for по парам ключ-значение объекта JSON.
  // items() даёт итератор по элементам, где .key() — код валюты, .value() —
  // курс.
  for (const auto& item : data["rates"].items()) {
    result << item.key() << ": " << item.value().get<double>() << "\n";
  }

  return result.str();
}

// Публичный метод: список валют

std::vector<std::string> CurrencyConverter::getAvailableCurrencies() {
  // Отдельный эндпоинт /currencies возвращает словарь код -> название.
  const std::string body =
      makeHttpRequest("https://api.frankfurter.app/currencies");
  const json data = json::parse(body);

  std::vector<std::string> currencies;
  currencies.reserve(
      data.size());  // резервируем память, чтобы избежать лишних реаллокаций

  for (const auto& item : data.items()) {
    // item.key() — это код валюты, item.value() — название.
    currencies.push_back(item.key() + " — " + item.value().get<std::string>());
  }

  return currencies;
}