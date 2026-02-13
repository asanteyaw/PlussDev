# Errors and Stack Traces

PlussQit provides deterministic, portable error reporting suitable for production.

- Throws `pluss::codem::Error` with:
  - user-visible message
  - triggering C++ source location (`std::source_location`)
  - logical stack trace of library calls

## Call guards

Use `PLUSS_CALL_GUARD()` at public API boundaries and important internal entry points.

## Path trimming

To keep logs readable:

- If `PLUSS_SOURCE_ROOT` is set, file paths are shown relative to that directory.
- Otherwise, extremely long paths are reduced to their basename.
