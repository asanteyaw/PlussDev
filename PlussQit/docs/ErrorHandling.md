# Error handling and stack traces (C++)

PlussQit is a C++ library, so it cannot replicate Python-level tracebacks perfectly.
However, it provides two practical mechanisms:

1. **Source location**: error messages include file/line/function at the throw site.
2. **Internal stack trace**: internal API entry points can register call frames so errors include a lightweight stack trace.

## Internal usage

Library entry points should add a guard:

```cpp
PLUSS_CALL_GUARD();
```

Then any `throw_error(...)` includes a formatted stack.

## User code location

For APIs where we want to capture the **user** call site, we may add `std::source_location`
default parameters on public wrappers (especially factories), so the library can report
where the user invoked the API.
