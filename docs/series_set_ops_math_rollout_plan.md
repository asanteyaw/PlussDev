# Series Set Ops + Math Surface Rollout Plan

## Doctrine Constraints (Applied)
- Accuracy first, then performance.
- Prefer Series-native vectorized ops; avoid avoidable host materialization and nested loops.
- Keep platform behavior aligned with input Series platform; only fallback when unavoidable.
- Use centralized error handling (`PLUSS_CHECK`/`PLUSS_FAIL`).
- Keep API lean and method-centric on `Series` for tensor/set primitives.

## Phase A: Series Set Ops (Current Focus)

### A1. API tranche (method surface)
- Add `Series` methods backed by `ops_internal`:
  - `isin(const Series& test_elements, bool assume_unique=false, bool invert=false)`
  - `union1d(const Series& other, bool sorted=true)`
  - `intersect1d(const Series& other, bool sorted=true, bool assume_unique=false)`
  - `setdiff1d(const Series& other, bool sorted=true, bool assume_unique=false)`
  - `setxor1d(const Series& other, bool sorted=true, bool assume_unique=false)`

### A2. Correctness contract (v1)
- 1D semantics by contract (inputs flattened to 1D set domain internally).
- Deterministic behavior:
  - `sorted=true`: sorted set output.
  - `sorted=false`: stable-by-first-occurrence where applicable.
- DType/platform checks are explicit and fail-loud when unsupported/mismatched.
- `isin` returns boolean mask with same shape as `elements`.

### A2 Status
- Completed: method surface wired (`isin`, `union1d`, `intersect1d`, `setdiff1d`, `setxor1d`) with direct `ops_internal` bindings.
- Completed: `unique(..., sorted=false)` now performs true first-occurrence dedup (not adjacent-only dedup), fixing unsorted set semantics.
- Completed: compat dispatch now routes `unique(dim)` and `unique(dim, sorted)` correctly.

### A3. Accuracy tests
- Add focused CodeM tests for:
  - integer set ops core examples
  - empty-input edge cases
  - `assume_unique` path parity
  - `isin` + `invert`, including non-1D `elements` shape restoration

### A3 Status
- Completed: focused set-op tests added and passing.
- Completed: unique-dim contract tests passing after compat routing fix.

### A4. Performance pass (after correctness)
- Remove avoidable repeated dedup/sort work.
- Keep operations composed from Series ops (`eq`, `any`, `masked_select`, `unique`) to minimize host loops.
- Reuse pre-unique inputs when `assume_unique=true`.

### A4 Status
- Completed (v1): removed redundant dedup/sort stages in set-op paths (`union1d`, `intersect1d`, `setdiff1d`, `setxor1d`) to cut avoidable work.
- Pending (v2): larger-kernel perf pass for high-cardinality membership (`isin`/intersection) to reduce NxM broadcast pressure when needed.

## Phase B: Extended Set Theory Surface

### B1. Series-level finite-set extensions
- Subset/superset/disjoint checks.
- Optional along-dimension set operations for ND (`*_along(dim)`).

### B2. Measure-theory abstractions (not Series methods)
- Build sigma-algebra/family abstractions under Math analysis/measure-theory as dedicated types.
- Keep these separate from low-level tensor set ops.

## Phase C: Math-Heavy Expansion (PlussMa)

### C1. Structure
- Create dedicated actuarial subspace:
  - `PlussMa/Actuarial/...`
- Keep numerical analysis under `PlussMa/Math/...`.

Planned actuarial tranche (first pass):
- `PlussMa/Actuarial/LifeContingencies`: survival functions, discounting helpers, expected present value primitives.
- `PlussMa/Actuarial/LossModels`: severity/frequency building blocks and aggregate loss approximations.
- `PlussMa/Actuarial/Risk`: risk measures (VaR/CVaR) on top of existing Series math.
- Catch2 coverage per module in `PlussMa/Actuarial/test` with accuracy contracts, edge-case guards, and regression tests.

### C2. Integration strategy
- Multi-method integrator router:
  - adaptive deterministic quadrature (1D smooth)
  - cubature/sparse-grid (low/moderate D smooth)
  - QMC/MC with variance reduction (high D/noisy)
- Method selection by problem class, not MC-only.

### C3. ODE/PDE and vector calculus
- Stage solvers/ops under `PlussMa/Math/Numerical` and `PlussMa/Math/Analysis` with shared interfaces and strict tests.

## Build + Validation Workflow
- Compile target after each tranche:
  - `plussqit_codem` and representative test targets.
- Run focused tests for new set-op contracts before perf tuning.
- Keep API docs updates in lockstep after surface stabilization.

## Immediate Next Steps (Now)
1. Extend set-op tests for empty/degenerate and mixed dtype-path failures.
2. Add ND axis-aware set variants (`*_along(dim)`) after contracts are finalized.
3. Start PlussMa scaffold split: `Math/*` numerical analysis vs `Actuarial/*` domain models.
