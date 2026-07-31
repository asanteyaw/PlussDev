# Errors and Stack Traces

PlussQit provides deterministic, portable error reporting suitable for production.

- Throws `pluss::Error` with:
  - user-visible message
  - triggering C++ source location (`std::source_location`)
  - logical stack trace of library calls

`pluss::codem::Error` is a compatibility alias to `pluss::Error`.

Native trace controls:

- default: full native trace
- `PLUSS_ERROR_NATIVE_TRACE=focused`: focused `file -> pluss trigger` call trace
- `PLUSS_ERROR_NATIVE_TRACE=full`: full native trace
- `PLUSS_ERROR_NATIVE_TRACE=0` (or `off`/`false`): disable native trace section

## Call guards

Use `PLUSS_CALL_GUARD()` at public API boundaries and important internal entry points.

## Path trimming

To keep logs readable:

- If `PLUSS_SOURCE_ROOT` is set, file paths are shown relative to that directory.
- Otherwise, extremely long paths are reduced to their basename.
