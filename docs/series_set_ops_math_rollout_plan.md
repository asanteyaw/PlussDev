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

Actuarial subspace blueprint (active):
- `PlussMa/Actuarial/core`: shared first-principles life-contingency EPV + commutation primitives.
- `PlussMa/Actuarial/life_contingencies`: mortality/survival transforms and life-contingency utilities.
- `PlussMa/Actuarial/loss_models`: severity/frequency and aggregate-loss building blocks.
- `PlussMa/Actuarial/actuarial_models`: ratemaking/reserving workflow primitives.
- Per-subfolder tests under each module `test/` directory with edge-case and regression contracts.

### C1 Status (Current)
- Completed: base scaffold and build wiring for `core`, `life_contingencies`, `loss_models`, `actuarial_models`.
- Completed: baseline APIs + tests in all three new actuarial subfolders.
- Completed: `life_contingencies` L1 foundation consolidation (`future_lifetime_pmf`, `curtate_expectation_curve`, core-survival reuse).
- Completed: `loss_models` deep tranche L1-L4 foundation pass (distribution-backed moments, retention/layer kernels, parametric LEV/layer approximations).
- Completed: `actuarial_models` deep tranche foundation pass (credibility/rate-indication workflows, BF reserving, chain-ladder scaffolds).

### C1A. Actuarial Doctrine Guardrails (Must Hold)
- Reuse existing Math/Stats/Actuarial code before adding new helpers.
- Keep all heavy paths Series-native/vectorized; avoid host/scalar loops except when unavoidable.
- Preserve platform fidelity (no forced CPU materialization in runtime kernels).
- Keep centralized error handling (`PLUSS_CHECK`/`PLUSS_FAIL`/`pluss::throw_error`).
- Prefer additive API growth and avoid duplicate semantics across subfolders.

### C1B. Life Contingencies Blueprint (L1 Complete; L2-L4 Queued)
L1. Foundation consolidation:
- Reuse `core` validation/survival primitives where equivalent behavior already exists.
- Keep `life_contingencies` as transform/derived utility layer, not a duplicate EPV engine.
- Current primitives:
  - `px_from_qx`, `qx_from_px`
  - `force_of_mortality_from_qx`, `cumulative_hazard_from_qx`
  - `survival_from_force`
  - `future_lifetime_pmf`
  - `curtate_expectation_of_life`, `curtate_expectation_curve`

L2. Table + law layer (next in this subfolder):
- Mortality law surfaces (Gompertz/Makeham-style hazard parameterizations).
- Deterministic life-table transforms (`q_x`, `p_x`, `l_x`, `d_x`) with explicit shape/range contracts.
- Select/ultimate table composition interfaces.

L3. Product kernels in this subfolder (after L2):
- Temporary/whole-life term families as reusable blocks that map onto `core` EPV primitives.
- Deferred and increasing/decreasing benefit variants composed from base Series transforms.
- Moment-level outputs where analytically available (mean/second moment/variance for selected products).

L4. Test/perf gates for this subfolder:
- Accuracy contracts vs closed forms and hand-worked toy tables.
- Edge contracts: empty horizon, boundary probabilities, near-degenerate tails.
- Determinism + platform-parity checks (CPU vs available accelerator backends).
- Performance pass: enforce vectorized Series compositions and remove avoidable recomputation.

### C1C. Sequencing After Life Contingencies
- `loss_models` deep tranche:
  - frequency/severity families, aggregate approximations, stop-loss and retention contracts.
- `actuarial_models` deep tranche:
  - pure-premium/loss-ratio ratemaking workflows, indicated-rate decomposition, reserving scaffolds.

### C1D. Loss Models Blueprint (Completed Foundation Pass)
L1. Frequency + severity families:
- Reuse `pluss::subsp::stats::distr` objects/cumulants for Poisson, NegativeBinomial, Lognormal, and Gamma moment primitives.
- Keep runtime kernels Series-native and platform-preserving; avoid bespoke scalar recurrences when distribution APIs already exist.

L2. Aggregate-loss moments:
- Keep `aggregate_moments(...)` as the canonical independent compound-moment combiner.
- Add compound wrappers (`compound_poisson_*`, `compound_negative_binomial_*`) that compose frequency moments with scalar severity moments.

L3. Retention/stop-loss contracts:
- Retain `stop_loss_premium(...)` as empirical mean-excess contract.
- Add reusable payout transforms (`excess_loss`, `limited_excess_loss`) and layer EPV helper (`layer_premium`) to avoid duplicated deductible/limit math.

L4. Next in this subfolder:
- Add severity-tail and stop-loss approximations from parametric severity CDFs where numerically stable.
- Add aggregate approximation surfaces (normal/translated-gamma style) with explicit error/contract tests.

### C1E. Actuarial Models Blueprint (Completed Foundation Pass)
L1. Ratemaking core:
- `expected_claim_cost`, `pure_premium`, `loss_ratio`.
- Zero-safe curve variants: `pure_premium_curve`, `loss_ratio_curve`.

L2. Credibility + indication:
- `credibility_weight`, `credibility_blended_pure_premium`.
- `indicated_premium`, `indicated_rate_change`.
- Workflow primitive: `indicated_rate_change_from_experience`.

L3. Reserving scaffolds:
- Bornhuetter-Ferguson: `bornhuetter_ferguson_ultimate`, `bornhuetter_ferguson_reserve`.
- Chain ladder: `chain_ladder_age_to_age`, `chain_ladder_cdf`, `chain_ladder_ultimate`, `chain_ladder_reserve`.

L4. Validation gates:
- Added focused regression tests for workflow parity, chain-ladder factor/cdf/ultimate/reserve outputs, and invalid CDF contracts.

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
3. Begin `life_contingencies` L2 (mortality-law surfaces and deterministic table transforms).
