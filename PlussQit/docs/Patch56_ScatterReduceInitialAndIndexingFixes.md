# Patch 56: scatter_reduce knobs + indexing correctness fixes

## scatter_reduce: include_self + initial value policies
Torch exposes knobs on `scatter_reduce` like:
- `include_self` (whether the original tensor values participate)
- `initial` (seed value for the reduction)

This patch adds CPU-only support for both knobs (contiguous-only safety policy):
- `scatter_reduce(..., include_self, has_initial, initial)`
- `scatter_reduce_(..., include_self, has_initial, initial)`

Semantics:
- If `has_initial=true`:
  - output is seeded with `initial`
  - if `include_self=true`, we additionally fold the original self values into the reduction
  - then scatter-reduce the src values

Complex:
- only `"sum"` is supported (consistent with earlier design), and `initial` seeds real-part only.

## Indexing fix: do not permute index tensor contents in non-contiguous advanced path
Index tensors live in the broadcasted advanced-index space; permuting them by the input tensor dim permutation is incorrect.
We now only reorder *which index tensor corresponds to which dimension*.

Files:
- `src/CodeM/native/cpu/AdvancedIndexing.cpp`
- `src/CodeM/native/cpu/ScatterKernels.{h,cpp}`
- `src/CodeM/core/OpsInternal.{h,cpp}`
