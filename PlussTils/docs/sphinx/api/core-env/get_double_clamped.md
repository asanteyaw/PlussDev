# `pluss::tils::env::get_double_clamped`

[Back to the Environment API Reference](../core/environment.md)

```cpp
double pluss::tils::env::get_double_clamped(
    std::string_view key,
    double fallback,
    double min_value,
    double max_value)
```

`get_double_clamped()` combines strict finite parsing with an inclusive numeric
boundary. It first obtains a value using the same rules as `get_double()`:
valid environment text produces a parsed value, while absent or malformed text
produces `fallback`. It then clamps that selected value to the closed interval.
Consequently, the fallback is not an escape from the range; it is clamped too.

If `min_value` is greater than `max_value`, the function swaps them. This makes
bounds assembled from independent configuration sources order-insensitive.
The fallback and both bounds must be finite. A non-finite configuration throws
`std::invalid_argument` before reading or returning a value, because no finite
interval contract can be formed from such arguments.

| Parameter | Meaning |
|---|---|
| `key` | Environment key to parse |
| `fallback` | Candidate value used when parsing fails or the key is absent |
| `min_value` | One endpoint of the inclusive interval |
| `max_value` | The other endpoint; endpoints are reordered when necessary |

## Complete Example

```cpp
#include <pluss/tils/env.h>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

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

  set_env(key, "-3.5");
  std::cout << "double clamp low: "
            << pluss::tils::env::get_double_clamped(key, 1.0, -2.0, 2.0) << '\n';

  unset_env(key);
  std::cout << "double clamped fallback: "
            << pluss::tils::env::get_double_clamped(key, 7.0, -2.0, 2.0) << '\n';

  try {
    (void)pluss::tils::env::get_double_clamped(
        key, 0.0, -2.0, std::numeric_limits<double>::infinity());
  } catch (const std::invalid_argument&) {
    std::cout << "double invalid bounds: throws\n";
  }
}
```

Output:

```text
double clamp low: -2
double clamped fallback: 2
double invalid bounds: throws
```

Clamping is recovery, not validation reporting. If an out-of-range deployment
value should stop startup rather than be brought into range, use `get_double()`
and perform an explicit checked comparison at the application boundary.
