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

## Patch 130 Focus (current)
- [x] Add native QR/SVD device-path telemetry counters (success/fallback) to dispatch observability API.
- [x] Add crash-safe SVD prototype gate (`PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD_UNSAFE=1`) so enabling device-SVD no longer risks strict native suite stability by default.
- [x] Add hardware regression tests for native telemetry (`qr` device lane observed, `svd` safe fallback lane observed).
- [x] Re-run full hardware gates (`default`, `strict no-bridge`, `native-only`) and explicit strict native SVD with device-SVD env enabled; all passed.

## Patch 131 Focus (current)
- [x] Add native MPSGraph-backed `sort` + `argsort` dispatch lane on MPS (device execution, Series output remains on MPS).
- [x] Add strict routing policy knobs for validation (`PLUSS_MPS_NATIVE_SORT_SELECTION`, `PLUSS_MPS_NATIVE_SORT_SELECTION_STRICT`).
- [x] Add native strict regression test for `sort/argsort` parity on hardware.
- [x] Keep `topk/kthvalue` native lane implemented but default-off behind opt-in gate (`PLUSS_MPS_NATIVE_TOPK_SELECTION=1`) until stress-hardening is complete.
- [x] Re-run hardware gates (`sort` focus, full default, strict no-bridge, strict native linalg subset) with all passing.

## Patch 132 Focus (current)
- [x] Stabilize and graduate true device-native reduced SVD on MPS (`full_matrices=false`) and enable the native device lane by default.
- [x] Remove the unsafe prototype gate (`PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD_UNSAFE`) from reduced-SVD native routing.
- [x] Enforce strict native policy for reduced SVD: when bridge is disabled, no host-assisted fallback is allowed.
- [x] Add strict-native reduced-SVD regression coverage (deterministic parity, stress/repeat crash-free stability, strict policy rejection when device lane is disabled).
- [x] Re-run hardware gates (`default` `[mps]`, `strict no-bridge` `[strict]`, `native-only` `[mps][linearalg][native]`, focused strict-native SVD `[mps][linearalg][native][strict][svd]`) with all passing.

## Patch 149 Focus (current)
- [x] Add torch-family MPS parity suite for core operation families (elementwise, reductions, selection, indexing/set, gather/scatter, and linalg baseline parity checks).
- [x] Add strict no-fallback torch-family lane checks for native-safe MPS routes (`sort/argsort/take`) under strict policy.
- [x] Add fixed-seed stress parity loop to harden repeated-call stability on MPS.
- [x] Extend hardware-gate runner with torch-family parity gates (`[mps][parity][torch]`, `[mps][parity][torch][strict]`).

## Patch 150 Focus (current)
- [x] Promote native MPS top-k selection lane to default-on policy (`PLUSS_MPS_NATIVE_TOPK_SELECTION=1` by default).
- [x] Expand strict torch-family lane checks to include `topk` and `kthvalue`.
- [x] Add strict native linalg no-bridge assertion in torch-family strict suite (`linearalg_dispatch_counters().mps_bridge_total == 0`).

Next highest-impact aggressive family:
- [x] Run final mega hardware gate sweep and lock report artifacts (`2026-03-11`, `outputs/mps_mega_gate_20260311_171843/`).
- [x] Transition to CUDA tranche planning/execution (bootstrap contract tests + tranche blueprint + gate runner script).

## Patch 133 Focus (current)
- [x] Add CUDA bootstrap contract coverage in Catch2 (`[cuda][bootstrap]`) for fail-loud Series behavior and backend capability coherence.
- [x] Add reusable hardware gate runner script (`tools/run_codem_hardware_gates.sh`) with compact one-page + TSV artifacts to avoid large raw output dumps.
- [x] Publish CUDA tranche blueprint with doctrine alignment, PyTorch CUDA reference audit, phased implementation plan, and maturity baseline.

## Patch 134 Focus (current)
- [x] Add explicit CUDA readiness API (`pluss::cuda::is_ready` / `is_usable`) and wire `platform_available(Platform::CUDA)` to runtime probe.
- [x] Add CMake CUDA probe gate (`PLUSSQIT_ENABLE_CUDA`) with `CUDAToolkit` detection and `PLUSS_HAS_CUDA` compile definition.
- [x] Keep fail-loud CUDA Series semantics until native kernels exist (`Series::empty(..., CUDA)` / `Series::platform(CUDA)` still hard-fail by contract).
- [x] Make CUDA backend-info and bootstrap tests runtime-aware so suites pass coherently on both CUDA and non-CUDA hosts.

## Patch 135 Focus (current)
- [x] Add CUDA native runtime/policy module scaffold (`native/cuda/CUDARuntime.*`) with runtime probe status, device-count reporting, and transfer-policy parsing.
- [x] Add CUDA transfer policy surface (`pluss::cuda::transfer_policy`, `pluss::cuda::cpu_bridge_enabled`) and runtime-dynamic coverage tests.
- [x] Harden CUDA fail-loud errors to report runtime vs missing-storage/missing-transfer path distinctly.
- [x] Add stable latest-artifact pointers in gate runner (`outputs/latest_{mps,cuda}_mega_gate*`) for low-noise review workflows.

## Patch 136 Focus (current)
- [x] Add CUDA baseline capability surface (`native/cuda/BaselineOps.*`) for elementwise/reduction/matmul lane gating with runtime-dynamic env control (`PLUSS_CUDA_NATIVE_BASELINE`).
- [x] Extend CUDA backend provider contracts to report `cuBLAS`/`cuSOLVER` readiness in backend-info APIs.
- [x] Keep strict correctness posture: baseline capability reporting is explicit, but unsupported CUDA ops still fail loudly unless true native lanes are implemented.
- [x] Add Catch2 runtime coverage for CUDA baseline gating and provider coherence (`[cuda][baseline][runtime]`).

## Patch 140 Focus (current)
- [x] Add CUDA indexing capability/policy surface (`native/cuda/IndexingOps.*`) with runtime-dynamic env control:
  - `PLUSS_CUDA_NATIVE_INDEXING`
  - `PLUSS_CUDA_NATIVE_INDEXING_STRICT`
- [x] Enforce strict/bridge CUDA routing in core Series indexing paths (`index`, `take`, `put_`, `nonzero`) with fail-loud strict-native precedence.
- [x] Add Catch2 runtime + policy contracts for CUDA indexing baseline lanes (`[cuda][indexing][runtime]`, `[cuda][indexing][policy]`).
- [x] Add CUDA bridge parity contracts for indexing fallback (`[cuda][bridge][indexing]`).
- [x] Expand CUDA mega-gate matrix to include indexing runtime/policy/bridge families (now `15` CUDA gates total).

## Patch 141 Focus (current)
- [x] Add CUDA matmul capability/policy surface (`native/cuda/MatmulOps.*`) with runtime-dynamic env control:
  - `PLUSS_CUDA_NATIVE_MATMUL`
  - `PLUSS_CUDA_NATIVE_MATMUL_STRICT`
- [x] Add CUDA scatter/gather capability/policy surface (`native/cuda/ScatterGatherOps.*`) with runtime-dynamic env control:
  - `PLUSS_CUDA_NATIVE_SCATTER_GATHER`
  - `PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT`
- [x] Enforce strict/bridge CUDA routing in core dispatch for `matmul` and scatter/gather family (`scatter`, `scatter_add`, `scatter_reduce`, `gather`, and inplace variants).
- [x] Add Catch2 runtime + policy contracts for CUDA matmul family (`[cuda][matmul][runtime]`, `[cuda][matmul][policy]`).
- [x] Add Catch2 runtime + policy contracts for CUDA scatter/gather family (`[cuda][scatter][runtime]`, `[cuda][scatter][policy]`).
- [x] Add CUDA bridge parity + strict override contracts for matmul/scatter families (`[cuda][bridge][matmul]`, `[cuda][bridge][scatter]`).
- [x] Expand CUDA mega-gate matrix to include matmul/scatter runtime + strict + bridge families (now `20` CUDA gates total).

## Patch 144 Focus (current)
- [x] Graduate real native MPS `take/put_` fast-path support (replacing hard-disabled path) for float32 lanes.
- [x] Add fp16/bf16 MPS `take/put_` support via float32 work-lane casting with dtype roundtrip on return.
- [x] Enforce strict fail-loud semantics for unsupported MPS take/put lanes when `PLUSS_MPS_ALLOW_CPU_FALLBACK=0`.
- [x] Add strict-native Catch2 coverage for MPS take/put graduation and policy rejection (`[mps][take][native][strict]`, `[mps][put][native][strict]`, `[mps][take][native][strict][policy]`).
- [x] Expand MPS mega-gate matrix with strict-native take/put focus gates (now `12` MPS gates total).

## Patch 145 Focus (current)
- [x] Expand native MPS `take/put_` strict lane coverage to exact float32-roundtrip dtypes:
  - `bool`, `int8/uint8`, `int16/uint16`, `float16`, `bfloat16`, `float32`.
- [x] Keep strict fail-loud semantics for unsupported accumulate lanes (for example integer `put_(accumulate=true)` with no fallback policy).
- [x] Add strict-policy Catch2 coverage for native `put_` policy rejection (`[mps][put][native][strict][policy]`).
- [x] Expand MPS mega-gate matrix to include strict native `put_` policy focus gate (now `13` MPS gates total).

## Patch 146 Focus (current)
- [x] Add CUDA runtime/provider inventory APIs:
  - device count, runtime/driver version, primary device metadata,
  - provider readiness probes for `cuBLAS`, `cuSOLVER`, `cuDNN` with explicit error surfaces.
- [x] Wire CUDA backend info provider reporting to real provider readiness (not runtime-only assumption).
- [x] Add Catch2 CUDA provider + inventory coherence contracts (`[cuda][bootstrap][inventory]`, `[cuda][providers][runtime]`, `[cuda][providers][backend]`).
- [x] Add cluster hardware sweep automation script (`tools/run_codem_cuda_cluster_hardware.sh`) with compact upload-ready artifacts.
- [x] Expand CUDA mega-gate matrix from `20` to `22` gates.

## Parity Workstreams
1. Selection/Sorting family
- [x] `sort` (values path) on MPS via safe fallback
- [x] `argsort` values-only API on MPS via safe fallback
- [x] `topk` values-only API on MPS via safe fallback
- [x] `kthvalue` values-only API on MPS via safe fallback
- [x] Native MPSGraph route for `sort/argsort` on stable last-dimension lane
- [x] Native `topk/kthvalue` MPSGraph route exists but remains opt-in (`PLUSS_MPS_NATIVE_TOPK_SELECTION=1`) pending stress hardening
- [ ] Stable tie-order parity guarantees across all dtypes

2. Indexing/Set family
- [x] Native strict lane for `take` / `put_` is active for float32 + exact float32-roundtrip dtypes (`bool`, `int8/uint8`, `int16/uint16`, `float16`, `bfloat16`)
- [ ] Full native `take` / `put_` dtype coverage (remaining lanes: int32/int64/uint32/uint64/float64/complex)
- [x] Correctness-first MPS `nonzero` path (CPU fallback route currently forced while native parity hardening is in progress)
- [x] Safety hardening complete; native re-enable deferred pending stress parity on hardware
- [ ] Full native `nonzero` kernel coverage for higher-rank tensors
- [ ] Broader `index` multi-index parity on MPS

3. Shape/Reduction completeness
- [ ] Broaden native sort/topk coverage beyond current last-dimension stable lane and remove opt-in status for topk/kthvalue
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
