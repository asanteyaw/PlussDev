#include <pluss/tils/env.h>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

void set_env(const char* key, const char* value) {
#if defined(_WIN32)
  _putenv_s(key, value);
#else
  setenv(key, value, 1);
#endif
}

void unset_env(const char* key) {
#if defined(_WIN32)
  _putenv_s(key, "");
#else
  unsetenv(key);
#endif
}

} // namespace

int main() {
  using namespace pluss::tils::env;
  std::cout << std::boolalpha;

  constexpr auto text = "PLUSS_DOC_ENV_TEXT";
  unset_env(text);
  std::cout << "get missing: " << get(text).has_value() << '\n';
  std::cout << "get empty key: " << get("").has_value() << '\n';
  std::cout << "get_or missing: " << get_or(text, "fallback") << '\n';
  set_env(text, "  authored value  ");
  std::cout << "get present: [" << *get(text) << "]\n";
  std::cout << "get_or present: [" << get_or(text, "fallback") << "]\n";
  set_env(text, "");
  std::cout << "get_or present empty size: " << get_or(text, "fallback").size() << '\n';

  constexpr auto flag = "PLUSS_DOC_ENV_FLAG";
  set_env(flag, " YES ");
  std::cout << "bool true token: " << get_bool(flag, false) << '\n';
  set_env(flag, "off");
  std::cout << "bool false token: " << get_bool(flag, true) << '\n';
  set_env(flag, "perhaps");
  std::cout << "bool invalid fallback: " << get_bool(flag, true) << '\n';

  constexpr auto number = "PLUSS_DOC_ENV_NUMBER";
  set_env(number, " -42 ");
  std::cout << "int valid: " << get_int(number, 7) << '\n';
  set_env(number, "42px");
  std::cout << "int partial fallback: " << get_int(number, 7) << '\n';
  set_env(number, "999999999999999999999999");
  std::cout << "int overflow fallback: " << get_int(number, 7) << '\n';
  set_env(number, "6.25e1");
  std::cout << "double valid: " << get_double(number, -1.0) << '\n';
  set_env(number, "nan");
  std::cout << "double nonfinite fallback: " << get_double(number, -1.0) << '\n';
  set_env(number, "1.5ms");
  std::cout << "double partial fallback: " << get_double(number, -1.0) << '\n';

  set_env(number, "99");
  std::cout << "int clamp high: " << get_int_clamped(number, 5, 0, 10) << '\n';
  std::cout << "int reversed bounds: " << get_int_clamped(number, 5, 10, 0) << '\n';
  unset_env(number);
  std::cout << "int clamped fallback: " << get_int_clamped(number, 15, 0, 10) << '\n';
  set_env(number, "-3.5");
  std::cout << "double clamp low: " << get_double_clamped(number, 1.0, -2.0, 2.0) << '\n';
  unset_env(number);
  std::cout << "double clamped fallback: " << get_double_clamped(number, 7.0, -2.0, 2.0) << '\n';
  try {
    (void)get_double_clamped(number, 0.0, -2.0, std::numeric_limits<double>::infinity());
  } catch (const std::invalid_argument&) {
    std::cout << "double invalid bounds: throws\n";
  }

  unset_env(text);
  unset_env(flag);
  unset_env(number);
}
