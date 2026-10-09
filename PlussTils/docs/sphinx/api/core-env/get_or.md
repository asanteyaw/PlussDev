# `pluss::tils::env::get_or`

[Back to the Environment API Reference](../core/environment.md)

```cpp
std::string pluss::tils::env::get_or(
    std::string_view key,
    std::string fallback = {})
```

`get_or()` is the total text reader: it always returns an owning `std::string`.
When the key exists, the exact environment value wins. When the key is absent
or empty, the supplied fallback is returned. Omitting `fallback` therefore
turns absence into an empty string.

A present empty value is still present and wins over a nonempty fallback. This
is the most important distinction between `get_or()` and ad hoc code that tests
the resulting string for emptiness. Present text is not trimmed or normalized;
spaces and casing survive exactly as stored.

Because the fallback parameter is taken by value, a temporary or moved string
can become the result without another required ownership layer. A present value
still requires copying from the process environment into safe C++ storage.

## Complete Example

```cpp
#include <pluss/tils/env.h>

#include <cstdlib>
#include <iostream>

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

int main() {
  constexpr auto key = "PLUSS_DOC_ENV_TEXT";

  unset_env(key);
  std::cout << "get_or missing: "
            << pluss::tils::env::get_or(key, "fallback") << '\n';

  set_env(key, "  authored value  ");
  std::cout << "get_or present: ["
            << pluss::tils::env::get_or(key, "fallback") << "]\n";

  set_env(key, "");
  std::cout << "get_or present empty size: "
            << pluss::tils::env::get_or(key, "fallback").size() << '\n';
}
```

Output:

```text
get_or missing: fallback
get_or present: [  authored value  ]
get_or present empty size: 0
```

Use [`get()`](get.md) instead when the caller must preserve absence as a
separate state rather than choosing a default immediately.
