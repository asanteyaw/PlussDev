# Error Handling

Pluss APIs normalize runtime failures to `pluss::Error`.

`pluss::Error` messages include:

- the failure reason
- source location (`file:line (function)`)
- logical Pluss call stack (when call guards are active)

`pluss::codem::Error` remains available as a compatibility alias, but new code should catch `pluss::Error`.

## Basic Usage

```cpp
#include <pluss/pluss.h>
#include <iostream>

int main() {
  try {
    auto a = pluss::zeros({2, 2});
    auto bad = a.view({3, 3}); // shape mismatch
    (void)bad;
  } catch (const pluss::Error& e) {
    std::cerr << e.what() << '\n';
  }
}
```

## Validate Facade

Function-style checks are also available:

- `pluss::validate::check(condition, message = "validation check failed") -> void`
- `pluss::validate::warn(message) -> void`
- `pluss::validate::fail(message = "validation failed") -> noreturn`
- `pluss::validate::inspect(condition, message = "validation inspect failed") -> void`
- `pluss::validate::check_eq(lhs, rhs, lhs_name = "lhs", rhs_name = "rhs") -> void`

These delegate to the same centralized Pluss error/warn path used by `PLUSS_CHECK` / `PLUSS_WARN`.

```cpp
#include <pluss/pluss.h>

void run_step(const pluss::Series& y_pred, const pluss::Series& y_true) {
  pluss::validate::check(y_pred.defined(), "y_pred must be defined");
  pluss::validate::check_eq(y_pred.numel(), y_true.numel(), "y_pred.numel()", "y_true.numel()");
  pluss::validate::warn("running validation with default tolerance");
}
```

## Notes

- C++ cannot always provide Python-style full user-to-runtime tracebacks by default.
- Pluss improves debugging by attaching source location and internal call-stack context to thrown errors.
- Dispatch boundaries in CodeM/CodeN/CodeH now rethrow non-Pluss exceptions as `pluss::Error` with operation context.
- Use macros or `pluss::validate::*` based on style preference; both surface through `pluss::Error`.
- Native call trace uses demangled C++ symbols and attempts source-line resolution (`atos` on macOS, `addr2line` on Linux) when symbols are available.
- For best file:line traces, build with debug info (for example `-g`) in both your app and linked Pluss binaries.
- Native trace mode is controlled by `PLUSS_ERROR_NATIVE_TRACE`:
  - unset: full native stack trace (default)
  - `focused`: focused `file -> pluss trigger` trace
  - `full`: full native stack trace
  - `0`/`off`/`false`: disable native trace section
