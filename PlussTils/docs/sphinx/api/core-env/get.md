# `pluss::tils::env::get`

[Back to the Environment API Reference](../core/environment.md)

```cpp
std::optional<std::string> pluss::tils::env::get(std::string_view key)
```

`get()` is the lossless environment reader. It answers two questions without
collapsing them into one value: whether the process environment contains the
key, and, when it does, what byte sequence was stored there. This distinction
matters because a present empty variable and an absent variable commonly carry
different configuration meanings.

The returned `std::string` owns its characters. It is not a view into the C
runtime's environment table, so the result remains valid after later
environment changes. No trimming, case conversion, interpolation, path
expansion, or typed parsing occurs. Surrounding spaces are part of the value.

An empty key is treated as absent and returns `std::nullopt`. A nonempty key
that is not present does the same. `get()` does not throw for either condition.
Environment lookup is platform-provided; copying a present value takes time and
storage proportional to its length.

## Complete Example

This program covers a missing key, the empty-key boundary, and a present value
whose spaces must survive unchanged.

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
  std::cout << std::boolalpha;

  unset_env(key);
  std::cout << "get missing: "
            << pluss::tils::env::get(key).has_value() << '\n';
  std::cout << "get empty key: "
            << pluss::tils::env::get("").has_value() << '\n';

  set_env(key, "  authored value  ");
  std::cout << "get present: ["
            << *pluss::tils::env::get(key) << "]\n";

  unset_env(key);
}
```

Output:

```text
get missing: false
get empty key: false
get present: [  authored value  ]
```

Use [`get_or()`](get_or.md) when absence should immediately become a textual
default. Use a typed reader when the variable represents a boolean or number;
those readers define strict token rules that a raw string does not.
