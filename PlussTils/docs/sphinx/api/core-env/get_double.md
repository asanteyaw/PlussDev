# `pluss::tils::env::get_double`

[Back to the Environment API Reference](../core/environment.md)

```cpp
double pluss::tils::env::get_double(std::string_view key, double fallback)
```

`get_double()` reads a finite floating-point setting using locale-independent
decimal parsing. Surrounding ASCII whitespace is ignored, and ordinary decimal
or scientific notation is accepted. The complete trimmed token must parse:
unit suffixes, separators, and trailing comments make the value invalid.

The reader rejects overflow and every non-finite result, including textual
`nan` and infinity. Invalid, empty, and absent values return `fallback`
verbatim. The fallback itself is not constrained by this unbounded reader; a
caller that supplies a non-finite fallback receives that value when parsing
fails.

No exception is thrown for malformed environment text. This makes the function
appropriate where the fallback is the complete recovery policy. Use
[`get_double_clamped()`](get_double_clamped.md) when the application also needs
a guaranteed finite interval.

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

  set_env(key, "6.25e1");
  std::cout << "double valid: "
            << pluss::tils::env::get_double(key, -1.0) << '\n';

  set_env(key, "nan");
  std::cout << "double nonfinite fallback: "
            << pluss::tils::env::get_double(key, -1.0) << '\n';

  set_env(key, "1.5ms");
  std::cout << "double partial fallback: "
            << pluss::tils::env::get_double(key, -1.0) << '\n';
}
```

Output:

```text
double valid: 62.5
double nonfinite fallback: -1
double partial fallback: -1
```

The returned value has `double` precision. Parsing does not preserve the
original spelling or decimal scale; call `get()` when those textual properties
are part of the data.
