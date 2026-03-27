#include "pluss/tils/env.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace pluss::tils::env {
namespace {

[[nodiscard]] std::string trim_ascii(std::string_view value) {
  std::size_t first = 0;
  while (first < value.size() &&
         std::isspace(static_cast<unsigned char>(value[first])) != 0) {
    ++first;
  }
  std::size_t last = value.size();
  while (last > first &&
         std::isspace(static_cast<unsigned char>(value[last - 1])) != 0) {
    --last;
  }
  return std::string(value.substr(first, last - first));
}

[[nodiscard]] std::string lowercase_ascii(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return text;
}

[[nodiscard]] std::optional<std::string> raw_env_value(std::string_view key) {
  if (key.empty()) return std::nullopt;
  std::string local_key(key);
  if (const char* raw = std::getenv(local_key.c_str())) {
    return std::string(raw);
  }
  return std::nullopt;
}

} // namespace

std::optional<std::string> get(std::string_view key) {
  return raw_env_value(key);
}

std::string get_or(std::string_view key, std::string fallback) {
  if (auto value = raw_env_value(key)) return std::move(*value);
  return fallback;
}

bool get_bool(std::string_view key, bool fallback) {
  const auto raw = raw_env_value(key);
  if (!raw) return fallback;

  const std::string token = lowercase_ascii(trim_ascii(*raw));
  if (token.empty()) return fallback;
  if (token == "1" || token == "true" || token == "yes" || token == "on") return true;
  if (token == "0" || token == "false" || token == "no" || token == "off") return false;
  return fallback;
}

int get_int(std::string_view key, int fallback) {
  const auto raw = raw_env_value(key);
  if (!raw) return fallback;

  const std::string token = trim_ascii(*raw);
  if (token.empty()) return fallback;
  int parsed = fallback;
  const char* begin = token.data();
  const char* end = token.data() + token.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end) return fallback;
  return parsed;
}

double get_double(std::string_view key, double fallback) {
  const auto raw = raw_env_value(key);
  if (!raw) return fallback;

  const std::string token = trim_ascii(*raw);
  if (token.empty()) return fallback;

  char* parse_end = nullptr;
  const double parsed = std::strtod(token.c_str(), &parse_end);
  if (parse_end == token.c_str()) return fallback;
  if (parse_end == nullptr || *parse_end != '\0') return fallback;
  if (!std::isfinite(parsed)) return fallback;
  return parsed;
}

int get_int_clamped(std::string_view key, int fallback, int min_value, int max_value) {
  if (min_value > max_value) std::swap(min_value, max_value);
  const int parsed = get_int(key, fallback);
  return std::clamp(parsed, min_value, max_value);
}

double get_double_clamped(
    std::string_view key, double fallback, double min_value, double max_value) {
  if (min_value > max_value) std::swap(min_value, max_value);
  const double parsed = get_double(key, fallback);
  return std::clamp(parsed, min_value, max_value);
}

} // namespace pluss::tils::env
