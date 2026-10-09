# `pluss::tils::env::get_bool`

[Back to the Environment API Reference](../core/environment.md)

```cpp
bool pluss::tils::env::get_bool(std::string_view key, bool fallback = false)
```

`get_bool()` reads a human-authored boolean switch without making every caller
repeat token normalization. It trims surrounding ASCII whitespace, compares
case-insensitively, and recognizes this closed vocabulary:

| Result | Accepted tokens |
|---|---|
| `true` | `1`, `true`, `yes`, `on` |
| `false` | `0`, `false`, `no`, `off` |

An absent key, an empty key, an empty value, or any unrecognized token returns
`fallback`. The default fallback is `false`, making an ordinary feature flag
disabled unless it is explicitly enabled. Pass `true` when the variable is an
opt-out switch instead.

The function never interprets arbitrary nonzero numbers as true: `"2"` is
invalid and therefore selects the fallback. It does not throw for malformed
text. The result is a host `bool`, not a `Series` or a retained view of the
environment.

## Complete Example

The three calls demonstrate a normalized true token, an explicit false token,
and the caller-controlled result of malformed input.

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

int main() {
  constexpr auto key = "PLUSS_DOC_ENV_FLAG";
  std::cout << std::boolalpha;

  set_env(key, " YES ");
  std::cout << "bool true token: "
            << pluss::tils::env::get_bool(key, false) << '\n';

  set_env(key, "off");
  std::cout << "bool false token: "
            << pluss::tils::env::get_bool(key, true) << '\n';

  set_env(key, "perhaps");
  std::cout << "bool invalid fallback: "
            << pluss::tils::env::get_bool(key, true) << '\n';
}
```

Output:

```text
bool true token: true
bool false token: false
bool invalid fallback: true
```

Choose the fallback deliberately. An invalid deployment value and a missing
value are intentionally indistinguishable at this API boundary; use `get()`
first if the application must diagnose those states separately.
