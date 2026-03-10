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

## Parity Workstreams
1. Selection/Sorting family
- [x] `sort` (values path) on MPS via safe fallback
- [x] `argsort` values-only API on MPS via safe fallback
- [x] `topk` values-only API on MPS via safe fallback
- [x] `kthvalue` values-only API on MPS via safe fallback
- [ ] Stable tie-order parity guarantees across all dtypes

2. Indexing/Set family
- [x] Native-backed fast paths for `take` / `put_` on supported MPS dtype/index combinations
- [ ] Full native `take` / `put_` dtype coverage (remove fallback paths)
- [x] Native-backed fast path for `nonzero` (scalar + 1D)
- [x] Safety-hardened 1D `nonzero` index mapping under sparse/leading-zero patterns
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
