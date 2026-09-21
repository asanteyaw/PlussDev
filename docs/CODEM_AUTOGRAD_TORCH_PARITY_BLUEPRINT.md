# CodeM Autodiff Parity Blueprint (CPU-First)

## Objective
Close remaining autodiff method and result parity gaps against Torch-referenced behavior while keeping Pluss-native API naming and semantics.

## Doctrine Locks
- No verbatim Torch method names on the public Pluss surface.
- Keep canonical autodiff naming as `reverse`, `tracks_slope`, `sensitivity`, `clear_sensitivity`.
- Prefer `Series`-native/data-parallel kernels and avoid host scalarization unless unavoidable.
- CPU parity is the correctness benchmark; MPS/CUDA must match CPU parity behavior/tolerances.

## Current Patch Status

### Patch A1: Surface De-Torching
Status: **Completed**

Scope:
- Removed verbatim Torch alias methods from `Series`:
  - `backward(...)`
  - `requires_grad()/requires_grad_(...)`
  - `grad()`
  - `zero_grad()`
- Removed alias dispatch routing:
  - `backward -> _backward`
  - `requires_grad -> tracks_slope`
  - `requires_grad_ -> tracks_slope_`
- Removed synthetic `grad` invoke op.

Patched files:
- `PlussQit/src/CodeM/core/Series.h`
- `PlussQit/src/CodeM/compat/Invoke.cpp`
- `PlussQit/tests/CodeM/test_patch67_autodiff_transforms.cpp`

### Patch A2: CPU Advanced Index Reverse Expansion
Status: **Completed**

Scope:
- Expanded tracked `index(...)` reverse rule on CPU from a single-lane case to generic CPU indexing lanes by routing reverse accumulation through:
  - `native::cpu::index_put(zeros_like_input, indices, out_sens, accumulate=true)`
- Preserved fail-loud behavior for unsupported non-CPU tracked advanced-index routes.

Patched files:
- `PlussQit/src/CodeM/core/OpsInternal.cpp`
- `PlussQit/tests/CodeM/test_patch67_autodiff_transforms.cpp`

### Patch A3: Reverse Policy + Retention Semantics Matrix
Status: **Completed**

Scope:
- Added one-shot guard semantics for pullback closures when `keep_graph=false`.
- Preserved reusable pullback semantics when `keep_graph=true`.
- Added internal multi-lane policy harmonization:
  - `vjp_lanes` now forces graph retention when more than one lane is requested.
- Added matrix tests for one-shot/reusable pullback behavior and lane retention behavior.

Patched files:
- `PlussQit/include/pluss/subsp/math/diff.h`
- `PlussQit/tests/CodeM/test_patch67_autodiff_transforms.cpp`

## Remaining Patches To Full CPU Method + Result Parity
**Count: 1 patch remaining**

### Patch A4: CPU Result-Parity Harness (Deterministic)
Status: **Completed**

Scope:
- Add deterministic CPU parity harness comparing Pluss vs Torch references for selected autodiff families:
  - indexing/gather/select/take
  - reductions and simple training lanes
  - repeated-index accumulation and mask routes
- Codify tolerance envelopes per op family.

Patched files:
- `PlussQit/tests/CodeM/test_patch167_cpu_autodiff_parity.cpp`
- `PlussQit/CMakeLists.txt`
- `docs/sphinx/dl_torch_maturity_blueprint.md`

### Patch A5: Engine Integrity + Mutation/Versioning Stress Gates
Status: **Completed**

Scope:
- Expand fail-loud diagnostics and stress coverage for:
  - saved-value mutation/version bumps
  - in-place safety under tracked graphs
  - retained-graph reuse/misuse paths

Patched files:
- `PlussQit/src/CodeM/core/SeriesTape.cpp`
- `PlussQit/src/CodeM/core/SeriesTape.h`
- `PlussQit/tests/CodeM/test_patch67_autodiff_transforms.cpp`

Notes:
- Added captured-output version checks alongside parent version checks.
- Added explicit one-shot graph-reuse fail-loud path after `keep_graph=false`.
- Added diagnostics tests for captured-output mutation, one-shot reverse misuse, and in-place tracked mutation rejection.

### Patch A6: Device Lift (CPU as Benchmark for MPS/CUDA)
Status: **In Progress**

Scope:
- Promote validated CPU semantics/results to MPS/CUDA.
- Add strict parity checks against CPU baseline:
  - value parity
  - sensitivity parity
  - fail-loud parity where unsupported
- Keep strict no-silent-fallback expectations in parity tests.

Targets:
- `PlussQit/src/CodeM/core/OpsInternal.cpp`
- `PlussQit/src/CodeM/native/cuda/*`
- `PlussQit/src/CodeM/native/mps/*`
- `PlussQit/tests/CodeM/test_patch148_cuda_torch_family_parity.cpp`
- `PlussQit/tests/CodeM/test_patch149_mps_torch_family_parity.cpp`

Current A6 progress:
- Added device autodiff parity lanes in CUDA/MPS gate suites:
  - CPU-vs-device value parity on deterministic objective
  - CPU-vs-device sensitivity parity on deterministic objective
  - one-shot reverse misuse fail-loud parity checks
- Compiles on both gate targets.

## Immediate Execution Order
1. Build and run CPU autodiff tests after A1/A2/A3/A4.
2. Continue A6 using CPU gates as hard baseline for MPS/CUDA.
3. Expand A6 coverage to broader autodiff op families and unsupported-route fail-loud parity.
