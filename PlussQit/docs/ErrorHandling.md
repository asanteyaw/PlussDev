# Error handling and stack traces (C++)

PlussQit is a C++ library, so it cannot replicate Python-level tracebacks perfectly.
However, it provides two practical mechanisms:

1. **Source location**: error messages include file/line/function at the throw site.
2. **Internal stack trace**: internal API entry points can register call frames so errors include a lightweight stack trace.
3. **Unified exception type**: API-facing failures are normalized to `pluss::Error`.

## Internal usage

Library entry points should add a guard:

```cpp
PLUSS_CALL_GUARD();
```

Then any `throw_error(...)` includes a formatted stack.

## Canonical exception type

Use `pluss::Error` as the primary catch type in user code.

`pluss::codem::Error` remains a compatibility alias to `pluss::Error`.

```cpp
try {
  auto y = x.log();
} catch (const pluss::Error& e) {
  std::cerr << e.what() << '\n';
}
```

## Native Call Trace Modes

`throw_error(...)` now appends a native call trace by default to help pinpoint
user-site call chains without `std::cout` hunting.

- `PLUSS_ERROR_NATIVE_TRACE` unset: full native stack trace (default)
- `PLUSS_ERROR_NATIVE_TRACE=focused`: focused `file -> pluss trigger` view
- `PLUSS_ERROR_NATIVE_TRACE=full`: full native stack trace
- `PLUSS_ERROR_NATIVE_TRACE=0` (or `off`/`false`): disable native stack trace section

## User code location

For APIs where we want to capture the **user** call site, we may add `std::source_location`
default parameters on public wrappers (especially factories), so the library can report
where the user invoked the API.
