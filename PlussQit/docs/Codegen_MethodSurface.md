# Method surface codegen

Patch 53 introduces a second codegen stream to accelerate the Series method surface:

- Parses `src/CodeM/core/OpsInternal.h`
- Generates method wrappers that follow your philosophy:
  - if the Series already exists, call as a **method**
  - uses `ops_internal::*` as the canonical implementation

Generated outputs (in build directory):
- `generated/CodeM/Series/SeriesOpsInternalMethods.inc`
- `generated/CodeM/Series/SeriesOpsInternalMethods.cpp.inc`

This adds dozens of method overloads immediately, and as OpsInternal grows, the public surface grows automatically.
