# `pluss::tils::env::get_int`

[Back to the Environment API Reference](../core/environment.md)

```cpp
int pluss::tils::env::get_int(std::string_view key, int fallback)
```

`get_int()` reads a signed decimal host `int`. It trims surrounding ASCII
whitespace and requires the remaining token to be consumed completely. A
leading minus is accepted. Unit suffixes, fractional syntax, digit separators,
hexadecimal prefixes, trailing comments, and values outside the host `int`
range are rejected.

Rejection is a normal fallback path, not an exception. The same fallback is
used for an absent key, an empty key, or an empty value. This contract is useful
for optional process settings; use `get()` plus application validation when a
malformed present value must be distinguished from an absent one.

Parsing is locale-independent and does not allocate beyond the copied
environment token. The result is an ordinary C++ `int`; it has no Pluss dtype,
shape, or platform metadata.

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

int main() {
  constexpr auto key = "PLUSS_DOC_ENV_NUMBER";

  set_env(key, " -42 ");
  std::cout << "int valid: "
            << pluss::tils::env::get_int(key, 7) << '\n';

  set_env(key, "42px");
  std::cout << "int partial fallback: "
            << pluss::tils::env::get_int(key, 7) << '\n';

  set_env(key, "999999999999999999999999");
  std::cout << "int overflow fallback: "
            << pluss::tils::env::get_int(key, 7) << '\n';
}
```

Output:

```text
int valid: -42
int partial fallback: 7
int overflow fallback: 7
```

Use [`get_int_clamped()`](get_int_clamped.md) when a valid parse still needs an
inclusive operational bound.
