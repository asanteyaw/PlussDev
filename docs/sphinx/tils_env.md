# PlussTils Env Guide

`pluss::tils::env` provides strict, typed environment-variable readers for runtime configuration.

## Include

```cpp
#include <pluss/tils.h>
```

Or direct header:

```cpp
#include <pluss/tils/env.h>
```

## Namespace

```cpp
namespace pluss::tils::env
```

## API Surface

`std::optional<std::string> get(std::string_view key)`  
Returns `std::nullopt` when the variable is unset or when `key` is empty.

`std::string get_or(std::string_view key, std::string fallback = {})`  
Returns `fallback` when the variable is unset or when `key` is empty.

`bool get_bool(std::string_view key, bool fallback = false)`  
Parses these case-insensitive tokens after trimming spaces:

- true: `1`, `true`, `yes`, `on`
- false: `0`, `false`, `no`, `off`

Returns `fallback` when unset, empty, or invalid.

`int get_int(std::string_view key, int fallback)`  
Strict integer parse (`from_chars`). Returns `fallback` on unset, empty, or invalid token.

`double get_double(std::string_view key, double fallback)`  
Strict floating parse. Returns `fallback` on unset, empty, invalid, or non-finite token (`nan`, `inf`).

`int get_int_clamped(std::string_view key, int fallback, int min_value, int max_value)`  
Reads with `get_int`, then clamps to `[min_value, max_value]`. If bounds are inverted, they are normalized internally.

`double get_double_clamped(std::string_view key, double fallback, double min_value, double max_value)`  
Reads with `get_double`, then clamps to `[min_value, max_value]`. If bounds are inverted, they are normalized internally.

## Example

```cpp
#include <pluss/tils.h>
#include <iostream>

int main() {
  const auto mode = pluss::tils::env::get_or("PLUSS_MODE", "dev");
  const bool verbose = pluss::tils::env::get_bool("PLUSS_VERBOSE", false);
  const int threads = pluss::tils::env::get_int_clamped("PLUSS_THREADS", 4, 1, 64);
  const double lr = pluss::tils::env::get_double("PLUSS_LR", 1e-3);

  if (auto token = pluss::tils::env::get("PLUSS_API_TOKEN")) {
    std::cout << "token-set " << (token->empty() ? "empty" : "non-empty") << "\n";
  } else {
    std::cout << "token-unset\n";
  }

  std::cout << "mode=" << mode << "\n";
  std::cout << "verbose=" << (verbose ? "true" : "false") << "\n";
  std::cout << "threads=" << threads << "\n";
  std::cout << "lr=" << lr << "\n";
}
```

## Notes

- `get` distinguishes unset (`nullopt`) from set values (including empty-string values).
- `get_bool`, `get_int`, and `get_double` do not throw; they return the provided fallback when parsing fails.
- Use clamped helpers for user-provided tunables to keep runtime bounds explicit.
