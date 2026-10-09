#include <pluss/tils/env.h>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    ++failures;
  }
}

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

struct EnvCleanup {
  const char* key;
  ~EnvCleanup() { unset_env(key); }
};

void test_text() {
  constexpr auto key = "PLUSS_DOC_ENV_TEXT";
  EnvCleanup cleanup{key};
  set_env(key, "  value with spaces  ");

  const auto value = pluss::tils::env::get(key);
  expect(value.has_value(), "get returns a present value");
  expect(value == "  value with spaces  ", "get preserves spaces");
  expect(pluss::tils::env::get_or(key, "fallback") == "  value with spaces  ",
         "get_or preserves a present value");

  unset_env(key);
  expect(!pluss::tils::env::get(key).has_value(), "get reports an unset key");
  expect(pluss::tils::env::get_or(key, "fallback") == "fallback",
         "get_or returns its fallback");
  expect(!pluss::tils::env::get("").has_value(), "empty keys are absent");
}

void test_bool() {
  constexpr auto key = "PLUSS_DOC_ENV_BOOL";
  EnvCleanup cleanup{key};

  for (const auto* token : {"1", "true", "YES", " On "}) {
    set_env(key, token);
    expect(pluss::tils::env::get_bool(key, false), "recognized true token");
  }
  for (const auto* token : {"0", "false", "NO", " Off "}) {
    set_env(key, token);
    expect(!pluss::tils::env::get_bool(key, true), "recognized false token");
  }

  set_env(key, "perhaps");
  expect(pluss::tils::env::get_bool(key, true), "invalid bool uses true fallback");
  expect(!pluss::tils::env::get_bool(key, false), "invalid bool uses false fallback");
}

void test_numbers() {
  constexpr auto key = "PLUSS_DOC_ENV_NUM";
  EnvCleanup cleanup{key};

  set_env(key, "  -42  ");
  expect(pluss::tils::env::get_int(key, 7) == -42, "strict integer accepts spaces and sign");
  set_env(key, "42px");
  expect(pluss::tils::env::get_int(key, 7) == 7, "integer rejects trailing text");
  set_env(key, "999999999999999999999999");
  expect(pluss::tils::env::get_int(key, 7) == 7, "integer rejects overflow");

  set_env(key, " 6.25e1 ");
  expect(pluss::tils::env::get_double(key, -1.0) == 62.5, "double accepts exponent form");
  set_env(key, "nan");
  expect(pluss::tils::env::get_double(key, -1.0) == -1.0, "double rejects NaN");
  set_env(key, "1.5ms");
  expect(pluss::tils::env::get_double(key, -1.0) == -1.0, "double rejects trailing text");
}

void test_clamps() {
  constexpr auto key = "PLUSS_DOC_ENV_CLAMP";
  EnvCleanup cleanup{key};

  set_env(key, "99");
  expect(pluss::tils::env::get_int_clamped(key, 5, 0, 10) == 10,
         "integer clamps above the interval");
  expect(pluss::tils::env::get_int_clamped(key, 5, 10, 0) == 10,
         "integer normalizes reversed bounds");

  set_env(key, "-3.5");
  expect(pluss::tils::env::get_double_clamped(key, 1.0, -2.0, 2.0) == -2.0,
         "double clamps below the interval");
  unset_env(key);
  expect(pluss::tils::env::get_double_clamped(key, 7.0, -2.0, 2.0) == 2.0,
         "fallback is also clamped");

  bool threw = false;
  try {
    (void)pluss::tils::env::get_double_clamped(
        key, 0.0, -2.0, std::numeric_limits<double>::infinity());
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  expect(threw, "double clamp rejects non-finite bounds");
}

} // namespace

int main() {
  test_text();
  test_bool();
  test_numbers();
  test_clamps();
  if (failures == 0) {
    std::cout << "core environment tests passed\n";
  }
  return failures == 0 ? 0 : 1;
}
