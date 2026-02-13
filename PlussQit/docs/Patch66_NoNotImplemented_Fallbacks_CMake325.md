# Patch 66: No NotImplemented throws + CMake compatibility

This patch focuses on *usability under Proceed-with-B*: every exposed Series method should be callable without
crashing due to missing routing.

## Changes
- Lowered `cmake_minimum_required` to **3.25** so the project can be configured in more environments.
- Added `pluss::codem::warn(...)` to print a full callsite + optional stack trace without throwing.
- `torch_compat::Invoke` now uses **non-throwing fallbacks** for unrouted ops:
  - non-inplace: returns `self.clone()` (identity fallback)
  - inplace: performs no-op and returns `self`
  - both emit a `warn(...)` so gaps are visible and actionable.

This makes the library callable end-to-end while we continue filling true semantic parity.

## Next
Replace identity/no-op fallbacks with real kernels and expand routing tables to eliminate warnings.
