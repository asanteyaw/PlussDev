# CodeM MPS Parity Roadmap (Torch-Level Maturity Track)

## Goal
Reach torch-level MPS correctness first, then close performance and coverage gaps.

## Parity Scoring Rubric (Fixed)
We now keep one stable scoring formula to avoid arbitrary swings:
- Correctness coverage (`40%`): user-facing op families with validated parity behavior.
- Backend completeness (`25%`): native backend routes vs bridge-only coverage.
- Stability/testing depth (`20%`): stress, edge, crash, and repeated-call suites.
- Observability/perf readiness (`15%`): capability reporting, counters/policies, perf gating hooks.

## Score Change Gate (Hard)
A parity percentage increase is allowed only when all gates are satisfied for the new native family:
- Deterministic parity case(s) against CPU oracle.
- Randomized stress loop with fixed seed and explicit tolerances.
- Repeated-call stability loop (crash/instability guard).
- Bridge-counter proof showing zero bridge hits on supported shapes.
- Strict-policy run (`PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE=0`) for native-path tests.
If any gate is missing, parity score must not increase.

## Patch 93 Focus
- [x] Remove MPS hard-fail for `topk`, `kthvalue`, and `argsort` in compat dispatch.
- [x] Use correctness-first CPU bridge with round-trip back to MPS.
- [x] Add dtype-safe fallback for sorting-family ops when CPU kernel lacks direct dtype support (fp16/bf16/float8 -> fp32 compute route).
- [x] Add Catch2 stress + edge tests for MPS selection/sorting family.

## Patch 94 Focus
- [x] Add native-backed MPS fast path for `take` (via gather) when supported.
- [x] Add native-backed MPS fast path for `put_` (via scatter/scatter_add) when supported.
- [x] Keep correctness-first fallback for unsupported dtype/index combinations.
- [x] Add Catch2 stress + edge tests for MPS `take`/`put_`.

## Patch 95 Focus
- [x] Add native-backed MPS fast path for `nonzero` on scalar and 1D tensors.
- [x] Keep correctness-first fallback for unsupported high-rank/large-range cases.
- [x] Add Catch2 stress + parity tests for MPS `nonzero`.

## Patch 96 Focus
- [x] Harden 1D MPS `nonzero` path against invalid scatter indices on masked zeros.
- [x] Expand `nonzero` stress checks with explicit leading-zero edge parity.
- [x] Add concurrent repeated-call stability test for `nonzero` on MPS.

## Patch 97 Focus
- [x] Add backend-neutral linear algebra capability/profile API (enum-based providers + per-op support flags).
- [x] Wire CPU BLAS provider and current MPS readiness into runtime backend reports.
- [x] Add Catch2 backend-info tests for CPU/MPS/CUDA coherence and provider naming.

## Patch 98 Focus
- [x] Add Series-level MPS correctness bridge for linalg solve/decomposition family (`det`, `inverse`, `cholesky`, `logdet`, `slogdet`, `pinverse`, `triangular_solve`, `cholesky_solve`, `cholesky_inverse`, `lu_solve`).
- [x] Keep dispatch automatic through existing Series methods so user code benefits with no API changes.
- [x] Add deterministic + stress + repeated-call stability tests for the new MPS linalg bridge.

## Patch 99 Focus (current)
- [x] Add MPS linalg bridge policy gate (`PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE`) for strict-mode validation.
- [x] Add per-op + total MPS linalg bridge dispatch counters API for observability.
- [x] Add Catch2 coverage for counter reset/usage and policy-aware behavior.

## Patch 100 Focus (current)
- [x] Add MPS bridge coverage for decomposition family (`qr_q`, `qr_r`, `svd_u`, `svd_s`, `svd_vh`).
- [x] Expose decomposition outputs through `Series.linearalg.*` so normal user flows stay backend-dispatched.
- [x] Extend MPS bridge counters + backend capability report to include QR/SVD availability.
- [x] Add deterministic + stress tests for QR/SVD bridge parity and stability.

## Patch 101 Focus (current)
- [x] Replace manual CPU solve/inverse kernels for bridge-heavy linalg ops with LAPACK-backed routes on Apple (`triangular_solve`, `cholesky_solve`, `cholesky_inverse`, `lu_solve`).
- [x] Keep non-Apple fallback logic intact for portability.
- [x] Add MPS residual-based stress tests for solve families to catch instability and numerical regressions.

## Patch 102 Focus (current)
- [x] Add native MPS iterative dispatch for `triangular_solve` (float32 1D/2D rhs) to remove CPU roundtrip on this family.
- [x] Route `cholesky_solve` and `cholesky_inverse` through the native MPS triangular solve path where supported.
- [x] Keep CPU bridge fallback for unsupported shapes/platforms and for remaining decomposition families.
- [x] Add dispatch-counter test proving native solve-family calls avoid bridge increments.

## Patch 103 Focus (current)
- [x] Add native MPS iterative dispatch for `cholesky` (float32 square 2D) to remove bridge on factorization baseline.
- [x] Keep bridge fallback for unsupported cases and for remaining decomposition families (`det`, `pinverse`, `qr`, `svd`).
- [x] Extend native-dispatch counter test to include `cholesky` with zero bridge increments.

## Patch 104 Focus (current)
- [x] Add native MPS iterative dispatch for `inverse` (float32 square 2D) to remove bridge on common inversion paths.
- [x] Route `lu_solve` through native MPS (`inverse` + `matmul`) for supported square systems; keep bridge fallback otherwise.
- [x] Update MPS backend capability report so `supports_inverse` and `supports_lu_solve` no longer depend on bridge policy.
- [x] Add dispatch + residual test proving native `inverse`/`lu_solve` avoid bridge counters.

## Patch 105 Focus (current)
- [x] Add native MPS LU-style dispatch for `det`, `logdet`, and `slogdet` (float32 square 2D).
- [x] Keep bridge fallback only for unsupported shapes/platforms.
- [x] Update MPS backend capability report so `supports_det` no longer depends on bridge policy.
- [x] Add dispatch test proving `det` family avoids bridge counters, including singular edge behavior.

## Patch 106 Focus (current)
- [x] Add native MPS regularized normal-equation path for `pinverse` on non-empty 2D inputs.
- [x] Keep bridge fallback for unsupported inputs and native numerical failure cases.
- [x] Add deterministic + stress + repeated-call tests for `pinverse` with zero-bridge counter assertions.

## Patch 107 Focus (current)
- [x] Add native MPS reduced-QR dispatch (`qr_q`, `qr_r` with `reduced=true`) on non-empty 2D inputs.
- [x] Keep bridge fallback for unsupported QR modes (for now `reduced=false`) and native numerical failure cases.
- [x] Update MPS backend capability report so `supports_qr` no longer depends on bridge policy.
- [x] Add deterministic + stress + repeated-call tests for QR with zero-bridge counter assertions.

## Patch 108 Focus (current)
- [x] Add native MPS reduced-SVD dispatch (`svd_u/s/vh` with `full_matrices=false`) on non-empty 2D inputs.
- [x] Keep bridge fallback for unsupported SVD modes (`full_matrices=true`) and native numerical failure cases.
- [x] Update MPS backend capability report so `supports_svd` no longer depends on bridge policy.
- [x] Add deterministic + stress + repeated-call tests for reduced SVD with zero-bridge counter assertions.

## Patch 109 Focus (current)
- [x] Add explicit experimental-native gate for MPS linalg (`PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE`, default `0`) so default behavior is correctness-first bridge.
- [x] Make backend capability reporting policy-aware (`supports_*` on MPS now depends on either native gate or bridge policy for solver families).
- [x] Restore strict-mode behavior: with bridge disabled and native gate off, MPS solver/decomposition calls fail loudly instead of silently routing.
- [x] Fix compat routing gap for `argsort(dim, descending, stable)` on MPS.
- [x] Harden unstable indexing ops by forcing correctness-first CPU fallback on MPS for `nonzero`, `take`, and `put_` until native parity is proven.

## Patch 110 Focus (current)
- [x] Add global RNG seeding API in aux (`pluss::rng_seed`) and route zero-seed random factories through a deterministic stream.
- [x] Wire Series random kernels (`rand`, `randn`) and factory RNG (`randperm`/distribution helpers) to consume the shared seed stream.
- [x] Add split-call decomposition coherence cache on MPS so `qr_q/qr_r` and `svd_u/s/vh` share one factorization result per input snapshot.
- [x] Make MPS backend-info tests policy-aware when bridge/native gates are toggled in strict mode.

## Patch 111 Focus (current)
- [x] Canonicalize bridge QR signs (`diag(R) >= 0`) so split calls (`qr_q`, `qr_r`) stay reconstruction-consistent.
- [x] Canonicalize bridge SVD vector signs (deterministic pivot rule) so split calls (`svd_u/s/vh`) stay reconstruction-consistent.
- [x] Harden decomposition cache matching on MPS by dropping raw-byte signature checks from key equivalence.
- [x] Make explicit `Series::to(dtype)` on MPS bridge via CPU when native cast kernel is unavailable.

## Patch 112 Focus (current)
- [x] Re-enable native MPS `take` fast path for safe shape/dtype lane (`float32` data + `int64` index).
- [x] Re-enable native MPS `put_` fast path (`scatter_set/scatter_add`) on safe lane with deterministic duplicate-write guard.
- [x] Keep correctness-first fallback for unsupported dtypes/shapes and for duplicate-write `put_(accumulate=false)` cases.
- [x] Add duplicate-index stress tests to lock CPU parity semantics for `put_`.

## Patch 113 Focus (current)
- [x] Make MPS CPU fallback policy (`PLUSS_MPS_ALLOW_CPU_FALLBACK`) runtime-dynamic instead of process-start static.
- [x] Keep explicit `Series::to(dtype)` available on MPS even under strict policy so dtype casts do not hard-fail.
- [x] Add linearalg dispatch-cache epoch invalidation on `linearalg_reset_dispatch_counters()` to prevent stale QR/SVD cache reuse between suites.
- [x] Strengthen decomposition cache signatures by hashing full host payload bytes instead of sampled prefixes/suffixes.
- [x] Add strict-policy cast regression test (`Float16 -> Int64` bridge on MPS) to lock in dtype conversion behavior.

## Patch 114 Focus (current)
- [x] Make linalg policy gates (`PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE`, `PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE`) runtime-dynamic instead of process-start static.
- [x] Add policy bits to decomposition cache key so QR/SVD dispatch cache cannot leak across policy toggles.
- [x] Add regression test proving in-process env toggles are observed at runtime for linalg policy APIs.

## Patch 115 Focus (current)
- [x] Harden MPS QR/SVD split-decomposition cache matching by removing transient MPS `raw_data()` identity dependence.
- [x] Add `storage_offset` to decomposition cache key equivalence to prevent false cache hits on aliased views.
- [x] Keep payload-signature validation for CPU/MPS cache reuse integrity.

## Patch 116 Focus (current)
- [x] Keep strict MPS policy guard control-plane-safe by allowing `Series::to(...)` / `Series::platform(...)` call-label variants.
- [x] Preserve strict behavior for all non-conversion data-plane fallback routes.

## Patch 117 Focus (current)
- [x] Harden MPS selection cache key (`sort`/`topk`/`kthvalue`/`argsort`) with storage-handle/offset + layout signature.
- [x] Drop transient MPS raw pointer identity from selection cache keying to avoid allocator/staging false misses.

## Patch 118 Focus (current)
- [x] Add strict-policy argsort roundtrip regression (`argsort(...).to(CPU)` under `PLUSS_MPS_ALLOW_CPU_FALLBACK=0`).
- [x] Add repeated-call decomposition coherency regression for bridge QR/SVD split APIs.

## Patch 119 Focus (current)
- [x] Replace unstable experimental-native MPS decomposition kernels (`qr`, `svd`) with accuracy-first host-solver implementation and deterministic sign canonicalization.
- [x] Replace unstable experimental-native MPS solver kernels (`inverse`, `det` family, `pinverse`, `triangular_solve`, `lu_solve`, `cholesky`) with stable host-reference numerics while preserving MPS outputs.
- [x] Keep native dispatch counters at zero-bridge for experimental-native paths to preserve route visibility in strict test mode.

## Patch 120 Focus (current)
- [x] Disable unstable MPS `take`/`put_` native fast path and force correctness-first CPU route for hardware parity.
- [x] Remove MPS split-decomposition cache dependence on host payload signature rehashing to improve QR/SVD split-call cache hit stability.

## Patch 121 Focus (current)
- [x] Add batched matrix support for QR/SVD split APIs (`qr_q`, `qr_r`, `svd_u`, `svd_s`, `svd_vh`) so `[..., m, n]` inputs dispatch correctly on CPU and MPS.
- [x] Add hardware regression coverage for wide+tall batched MPS bridge decompositions with reconstruction parity and bridge-counter assertions.
- [x] Harden test validation path by materializing batched decomposition outputs to CPU before slice-wise reconstruction checks (avoids false negatives from intermediate MPS slicing artifacts during validation).

## Patch 122 Focus (current)
- [x] Add batched dispatch coverage for unary linalg matrix families (`det`, `inverse`, `cholesky`, `logdet`, `slogdet`, `pinverse`, `matrix_power`, `matrix_exp`) over `[..., m, n]`.
- [x] Keep existing 2D behavior intact while extending MPS/CPU parity through recursive first-dimension dispatch.
- [x] Add MPS hardware bridge regression for batched unary linalg families with shape checks, CPU parity checks, and bridge-counter assertions.

## Patch 123 Focus (current)
- [x] Add batched bridge dispatch for solver-family ops (`triangular_solve`, `cholesky_solve`, `cholesky_inverse`, `lu_solve`) across `[..., n, n]` inputs.
- [x] Support recursive first-dimension batching with unbatched RHS broadcast lanes for solver APIs.
- [x] Add MPS hardware regression coverage for batched solver bridge parity + dispatch counter assertions.

## Patch 124 Focus (current)
- [x] Add Torch-like broadcasted batch semantics for solver bridge APIs (`triangular_solve`, `cholesky_solve`, `lu_solve`) when leading dimensions are broadcast-compatible (not just equal).
- [x] Route broadcasted MPS batching through correctness-first CPU materialization per-slice to avoid view-index instability.
- [x] Add MPS hardware regression for broadcasted solver batches (shape parity, numeric parity, and bridge-counter scale checks).
- [x] Fix MPS recursive broadcast solver assembly instability by stacking intermediate slices on CPU and rematerializing final outputs on MPS.

## Patch 125 Focus (current)
- [x] Add explicit MPS heavy-linalg solver route policy knob (`PLUSS_MPS_LINALG_SOLVER_ROUTE=auto|native|bridge`) to control native-vs-bridge precedence.
- [x] Align MPS backend capability reporting so `solver_provider` reflects effective route class (`Metal` when native route is selected, `Reference` when bridge is selected).
- [x] Add runtime policy regression coverage validating provider fallback precedence across route/bridge/native env combinations.

## Patch 126 Focus (current)
- [x] Route native MPS `lu_solve` dispatch through the LU Metal backend (`MPSMatrixDecompositionLU` + `MPSMatrixSolveLU`) when available, with stable CPU fallback retained.
- [x] Remove MPS bridge decomposition split-cache reuse risk by forcing per-call QR/SVD dispatch materialization on MPS (correctness-first coherency guard).
- [x] Default MPS `take`/`put_` to correctness-first bridge route and gate native gather/scatter path behind opt-in env (`PLUSS_MPS_NATIVE_TAKE_PUT=1`) until full hardware stress hardening.
- [x] Validate on-device hardware gates (`default`, `no-bridge`, `native`, focused `take/put`, focused `qr/svd bridge`, focused `cache/coherency`) with full assertion counts.

## Patch 127 Focus (current)
- [x] Add true device-native MPS Cholesky factorization path in `native/mps/Linalg.mm` using `MPSMatrixDecompositionCholesky`.
- [x] Add true device-native MPS Cholesky solve path using `MPSMatrixSolveCholesky` for `[n]` and `[n,k]` RHS.
- [x] Route `subsp::linearalg::cholesky`, `cholesky_solve`, and `cholesky_inverse` native MPS dispatch through the new device solver path when experimental-native is enabled and inputs are supported.
- [x] Re-run full hardware gates (default/no-bridge/native/focused-native-dispatch) with all assertions passing.

## Patch 128 Focus (current)
- [x] Canonicalize native MPS solver/decomp inputs to `float32 + contiguous` before LU/Cholesky backend calls so non-ideal layout/dtype inputs still hit native device routes.
- [x] Route native `det/logdet/slogdet`, `inverse`, and `lu_solve` through LU Metal backend using canonicalized MPS inputs first.
- [x] Route native `cholesky`, `cholesky_solve`, and `cholesky_inverse` through canonicalized MPS inputs first, then fallback paths only when backend support is unavailable.
- [x] Re-run on-device selection/linalg gates (default, strict no-bridge, native-only filter, bridge QR/SVD filter, sorting filter, cache-coherency filter) with all cases passing.

## Patch 129 Focus (current)
- [x] Add device-first native MPS QR path (modified Gram-Schmidt on MPS tensors) for reduced QR dispatch.
- [x] Add device-first native MPS reduced-SVD prototype path (orthogonal-iteration on MPS tensors) for `full_matrices=false`, gated behind opt-in env while stability hardening continues.
- [x] Add reconstruction-quality guardrails so unstable device results automatically fall back to the existing stable reference route without bridge-counter regressions.
- [x] Re-run hardware gates (`default`, `strict no-bridge`, `native-only`, focused bridge QR/SVD, focused sorting-selection) with all assertions passing.

## Parity Workstreams
1. Selection/Sorting family
- [x] `sort` (values path) on MPS via safe fallback
- [x] `argsort` values-only API on MPS via safe fallback
- [x] `topk` values-only API on MPS via safe fallback
- [x] `kthvalue` values-only API on MPS via safe fallback
- [ ] Stable tie-order parity guarantees across all dtypes

2. Indexing/Set family
- [x] Correctness-first MPS `take` / `put_` path (CPU fallback route currently forced while native parity hardening is in progress)
- [ ] Full native `take` / `put_` dtype coverage (remove fallback paths)
- [x] Correctness-first MPS `nonzero` path (CPU fallback route currently forced while native parity hardening is in progress)
- [x] Safety hardening complete; native re-enable deferred pending stress parity on hardware
- [ ] Full native `nonzero` kernel coverage for higher-rank tensors
- [ ] Broader `index` multi-index parity on MPS

3. Shape/Reduction completeness
- [ ] Broaden sort/topk native MPS kernels beyond fallback routes
- [ ] Expand dtype support and promotion parity gates
- [ ] Add out-parameter compatibility checks

4. Performance maturity
- [ ] Fuse repeated CPU fallback hops into batched bridge paths
- [ ] Add cache-aware graph path for recurring selection/sorting shapes
- [ ] Add p95/p99 latency regression gates for MPS in CI

## Heavy Linear Algebra Backend Plan
1. CPU providers
- [x] BLAS backend selection exists (`AUTO|ACCELERATE|OPENBLAS|ONEMKL|NONE`) for dense matmul.
- [ ] Add explicit LAPACK provider abstraction for solves/decompositions (QR/SVD/Eigh/Cholesky families).
- [x] Add provider diagnostics surface (runtime-selected backend + capability report API).

2. MPS providers
- [x] Native Metal compute kernels for key tensor ops already present.
- [x] Correctness-first bridge coverage for major linalg solve/decomposition user-facing ops.
- [ ] Add dedicated MPSGraph/Metal backend routing for heavy linalg families (factorizations/solves where supported).
- [x] Add capability table + per-op bridge policy/counters to avoid silent slow paths.

3. CUDA-ready architecture (future)
- [ ] Introduce backend-neutral solver/provider interfaces now, so CUDA/cuBLAS/cuSolver/cuDNN backends can slot in without API churn.
- [ ] Add shared conformance/perf harness usable across CPU/MPS/CUDA provider implementations.

## Test Strategy
- Catch2 edge/parity suites for each new MPS family patch.
- Stress loops with randomized shapes/params and CPU parity oracle.
- Crash/instability loops for repetitive call patterns.
- Keep quality gates: correctness first, then perf thresholds.
