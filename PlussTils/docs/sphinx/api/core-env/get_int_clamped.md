# `pluss::tils::env::get_int_clamped`

[Back to the Environment API Reference](../core/environment.md)

```cpp
int pluss::tils::env::get_int_clamped(
    std::string_view key,
    int fallback,
    int min_value,
    int max_value)
```

`get_int_clamped()` turns an optional integer setting into a value inside a
closed interval. Parsing happens first using `get_int()`. A valid token selects
its parsed integer; absent, empty, malformed, or overflowing text selects
`fallback`. The selected value is then clamped, so both the environment value
and the fallback obey the same final range.

Bounds are inclusive. Reversed endpoints are normalized by swapping them,
which avoids a second call-site branch when minimum and maximum values come
from independent sources. Equal endpoints are valid and force that one result.
All arithmetic remains in the host `int` domain, so there is no intermediate
subtraction that could overflow.

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
  constexpr auto key = "PLUSS_DOC_ENV_NUMBER";

  set_env(key, "99");
  std::cout << "int clamp high: "
            << pluss::tils::env::get_int_clamped(key, 5, 0, 10) << '\n';
  std::cout << "int reversed bounds: "
            << pluss::tils::env::get_int_clamped(key, 5, 10, 0) << '\n';

  unset_env(key);
  std::cout << "int clamped fallback: "
            << pluss::tils::env::get_int_clamped(key, 15, 0, 10) << '\n';
}
```

Output:

```text
int clamp high: 10
int reversed bounds: 10
int clamped fallback: 10
```

The operation deliberately does not report whether clamping occurred. If that
fact is significant, call `get_int()` and compare the result with the desired
bounds before applying the application's policy.
