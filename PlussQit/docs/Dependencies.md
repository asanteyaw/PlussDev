# Dependencies

PlussQit is designed to be reproducible and buildable in offline environments.

## fmt

- Optional, used for richer error formatting.
- Enabled via `-DPLUSSQIT_USE_FMT=ON`.
- By default PlussQit does **not** fetch fmt from the network.
  - Install `fmt` via your package manager, or
  - set `-DPLUSSQIT_FETCH_DEPS=ON` to allow FetchContent.

## Catch2 (tests)

- Required only when `-DPLUSSQIT_BUILD_TESTS=ON`.
- Same policy: install it yourself or set `-DPLUSSQIT_FETCH_DEPS=ON`.
