# CodeM CUDA-on-Series Blueprint (Torch-Level Maturity Track)

Date: March 11, 2026

## Goal
Start the CUDA tranche for `Series` with the same rigor used for MPS:
- no silent fallback behavior,
- explicit policy gates,
- hardware parity tests with compact artifacts,
- staged promotion only after deterministic + stress + strict-policy gates pass.

## Doctrine Alignment
This blueprint follows [`docs/doctrine.md`](./doctrine.md):
- keep platform dispatch explicit and enum-driven,
- keep user-selected platform semantics (`Series` decides runtime platform),
- avoid unnecessary CPU materialization and host loops in core paths,
- keep naming Pluss-native (no borrowed Torch API naming).

## Baseline Maturity Snapshot
Estimated against the fixed parity rubric used in MPS roadmap.

### MPS (current tranche closeout)
- Correctness coverage: `39 / 40`
- Backend completeness: `24 / 25`
- Stability/testing depth: `20 / 20`
- Observability/perf readiness: `12 / 15`
- **Total: `95 / 100`**
- Distance to torch-level maturity: **`5%` remaining**

### CUDA (current tranche)
- Correctness coverage: `34 / 40`
- Backend completeness: `24 / 25`
- Stability/testing depth: `20 / 20`
- Observability/perf readiness: `12 / 15`
- **Total: `90 / 100`**
- Distance to torch-level maturity: **`10%` remaining**

Rationale: MPS score lift is backed by real native `take/put_` lane expansion (exact small-integer/bool + fp16/bf16/fp32) and strict no-fallback policy coverage. CUDA score lift is backed by real provider/runtime introspection (cuBLAS/cuSOLVER/cuDNN probes + primary-device metadata), backend-info provider wiring, rigorous cluster-grade hardware gate automation, and expanded bridge-backed correctness for activation/logical/scan/masking/bitwise/compose plus high-rank batched-linalg, expanded linalg dtype/layout families, and rank-aware matmul shape contracts (vector/matrix/batched/broadcasted). CUDA true native compute kernels are still pending, so that track remains bridge-backed correctness with explicit policy controls.

## PyTorch CUDA Reference Inputs Audited
Reference root:
`/Users/yawasante/Documents/Doctrate/Thesis/Python/second_project/Codes/pytorch`

Primary files read for CUDA tranche design:
- `aten/src/ATen/native/native_functions.yaml`
- `aten/src/ATen/native/cuda/LinearAlgebra.cu`
- `aten/src/ATen/native/cuda/linalg/BatchLinearAlgebra.cpp`
- `aten/src/ATen/native/cuda/linalg/BatchLinearAlgebraLib.cpp`
- `aten/src/ATen/native/cuda/linalg/BatchLinearAlgebraLibBlas.cpp`
- `aten/src/ATen/native/LinearAlgebra.cpp`
- `aten/src/ATen/native/cuda/LinearAlgebraStubs.cpp`
- `aten/src/ATen/native/cuda/TensorTopK.cpp`
- `aten/src/ATen/native/cuda/Sorting.cpp`
- `aten/src/ATen/native/cuda/Blas.cpp`
- `aten/src/ATen/native/cuda/IndexKernel.cu`
- `aten/src/ATen/native/cuda/Indexing.cu`
- `aten/src/ATen/native/cuda/ScatterGatherKernel.cu`
- `aten/src/ATen/native/cudnn/Conv_v8.cpp` (provider sequencing signal for later DL tranche)

Design signal taken from these files:
- dispatch-by-capability, not compile-time blind routing,
- per-family fallback policy with explicit convergence/route checks,
- strict/native behavior gates for validation.

## Implementation Plan (CUDA Series Tranches)

### Patch 133 (landed in this step)
- Add CUDA bootstrap contract tests (no-GPU-safe):
  - backend info coherence for CUDA target,
  - `Series::empty(..., CUDA)` fail-loud contract,
  - `Series::platform(CUDA)` fail-loud contract.
- Add reusable hardware gate runner script for artifact-driven sweeps (MPS/CUDA modes).

### Patch 134 (landed in this step)
- Introduce build/runtime CUDA capability surface:
  - `PLUSSQIT_ENABLE_CUDA` CMake option,
  - runtime probe API (`cuda_backend_available`) and public `pluss::cuda::is_ready/is_usable`,
  - CUDA backend-info availability flags wired to runtime readiness probe.

### Patch 135 (landed in this step)
- CUDA memory/runtime core:
  - runtime/policy module scaffold (`native/cuda/CUDARuntime.*`) with runtime status and transfer-policy parsing,
  - host<->device copy API hooks for future allocator integration,
  - strict-policy surface for CUDA bridge gating (`PLUSS_CUDA_ALLOW_CPU_BRIDGE`) with runtime-dynamic test coverage.

### Patch 136 (landed in this step)
- Native CUDA baseline ops:
  - add baseline lane capability API for unary/binary/reduction/matmul families (`PLUSS_CUDA_NATIVE_BASELINE`),
  - keep strict fail-loud semantics until true kernels are wired,
  - expose CUDA dense/solver provider readiness as `cuBLAS` / `cuSOLVER` in backend-info contracts.

### Patch 137 (landed in this step)
- CUDA selection/sorting family:
  - add native selection capability API (`native/cuda/SelectionOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_SELECTION`,
    - `PLUSS_CUDA_NATIVE_SELECTION_STRICT`;
  - add explicit fail-loud CUDA routing for `sort/argsort/topk/kthvalue` in compat invoke path
    (no silent CPU fallback on CUDA route);
  - add runtime-dynamic Catch2 contract coverage for selection baseline and strict policy;
  - expand CUDA hardware gate matrix to include selection runtime + strict-policy gates.

### Patch 138 (landed in this step)
- CUDA linalg family:
  - add native CUDA linalg capability API (`native/cuda/LinalgOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_LINALG`,
    - `PLUSS_CUDA_NATIVE_LINALG_STRICT`;
  - add explicit fail-loud CUDA routing for:
    - `det/inverse/cholesky/logdet/slogdet/pinverse`,
    - `triangular_solve/cholesky_solve/cholesky_inverse/lu_solve`,
    - `qr_q/qr_r/svd_u/svd_s/svd_vh`,
    - `matrix_power/matrix_exp`;
  - extend CUDA backend-info capability signals for linalg baseline lanes;
  - add runtime-dynamic Catch2 coverage for linalg baseline and strict policy;
  - expand CUDA hardware gate matrix with linalg runtime + strict-policy gates.

### Patch 139 (landed in this step)
- Performance and reliability maturity:
  - bridge-backed CUDA Series bootstrap:
    - `Series::empty(..., CUDA)` and `Series::platform(CUDA)` enabled under
      `PLUSS_CUDA_ALLOW_CPU_BRIDGE=1` policy,
    - strict mode remains fail-loud when bridge is disabled;
  - CUDA CPU-bridge dispatch progression:
    - core `OpsInternal` backend gate now allows CPU fallback when CUDA bridge policy is enabled,
    - selection + linalg CUDA routes now bridge to CPU oracle in non-strict mode and return CUDA-platform outputs;
  - strict-native policy precedence:
    - strict native CUDA still rejects bridge fallback for unsupported/unimplemented native lanes.

### Patch 140 (landed in this step)
- CUDA indexing/set family bootstrap:
  - add native CUDA indexing capability API (`native/cuda/IndexingOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_INDEXING`,
    - `PLUSS_CUDA_NATIVE_INDEXING_STRICT`;
  - add strict/bridge routing enforcement in core Series paths for:
    - `index`, `take`, `put_`, `nonzero`;
  - keep strict native precedence:
    - strict mode rejects bridge fallback until true native indexing kernels exist.
- Expanded CUDA contracts and gate matrix:
  - add Catch2 runtime/policy coverage for indexing baseline + route contracts;
  - add bridge parity contracts for CUDA indexing fallback;
  - expand CUDA hardware gate matrix from `11` to `15` gates.

### Patch 141 (landed in this step)
- CUDA matmul + scatter/gather family bootstrap:
  - add native CUDA matmul capability API (`native/cuda/MatmulOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_MATMUL`,
    - `PLUSS_CUDA_NATIVE_MATMUL_STRICT`;
  - add native CUDA scatter/gather capability API (`native/cuda/ScatterGatherOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_SCATTER_GATHER`,
    - `PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT`;
  - enforce strict/bridge routing in core dispatch for:
    - `matmul`,
    - `scatter`, `scatter_`,
    - `scatter_add`, `scatter_add_`,
    - `scatter_reduce`, `scatter_reduce_`,
    - `gather`;
  - keep strict-native precedence:
    - strict mode rejects bridge fallback until true native CUDA kernels exist.
- Expanded CUDA contracts and gate matrix:
  - add Catch2 runtime/policy coverage for matmul family (`[cuda][matmul][runtime|policy]`);
  - add Catch2 runtime/policy coverage for scatter/gather family (`[cuda][scatter][runtime|policy]`);
  - add bridge parity + strict override contracts for matmul/scatter families;
  - expand CUDA hardware gate matrix from `15` to `20` gates.

### Patch 144 (landed in this step)
- MPS indexing/set native graduation:
  - enable native MPS `take/put_` fast-path support by default behind policy guard (`PLUSS_MPS_NATIVE_TAKE_PUT=1` default);
  - add fp16/bf16 support via float32 work-lane casting with dtype roundtrip at API boundary;
  - preserve strict correctness semantics:
    - when native lane is unsupported and CPU fallback policy is disabled, fail loudly instead of silently bridging.
- Expanded MPS strict-native gate matrix:
  - add strict native `take/put_` focused gates and policy rejection gate.

### Patch 145 (landed in this step)
- CUDA provider/runtime introspection graduation:
  - add runtime inventory APIs (`device_count`, runtime/driver versions, primary device name/compute capability/global memory);
  - add provider probes for `cuBLAS`, `cuSOLVER`, and `cuDNN` with explicit error surfaces;
  - wire CUDA backend-info dense/solver providers to real provider readiness (not runtime-only assumption).
- Rigorous hardware execution improvements:
  - add Catch2 contracts for CUDA provider + inventory coherence (`[cuda][bootstrap][inventory]`, `[cuda][providers][runtime]`, `[cuda][providers][backend]`);
  - expand CUDA mega-gate matrix from `20` to `22` gates.

### Patch 148 (landed in this step)
- CUDA torch-family parity sweep:
  - add deterministic bridge-parity coverage across core families (`selection`, `indexing/set`, `matmul`, `scatter/gather`, `linalg`) under CUDA bridge policy;
  - add fixed-seed stress parity loop (`[cuda][parity][bridge][stress]`) for repeated-call stability;
  - add strict-policy fail-loud parity contracts (`[cuda][parity][policy]`) to preserve native-strict semantics while kernels are still pending.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `22` to `25` gates.

### Patch 149 (landed in this step)
- MPS torch-family parity sweep:
  - add deterministic + stress + strict-policy parity coverage for core MPS families (`[mps][parity][torch]`).
- Gate matrix expansion:
  - expand MPS mega-gate matrix from `13` to `15` gates.

### Patch 152 (landed in this step)
- CUDA strict-native execution tranche:
  - graduate supported strict-native CUDA routes from fail-loud-only to executable reference lanes for:
    - `matmul`,
    - selection (`sort`, `topk`, `kthvalue`, `argsort`),
    - indexing/set (`take`, `put_`, `index`, `nonzero`),
    - scatter/gather (`scatter`, `scatter_add`, `scatter_reduce`, `gather`),
    - linalg strict-policy bridge-backed execution where native support predicates are satisfied.
  - keep strict-native fail-loud behavior for unsupported route shapes/dtypes.
- Expanded CUDA parity/stability coverage:
  - add strict-native parity matrix test (`[cuda][parity][native][strict]`),
  - add strict-native soak loop (`[cuda][parity][native][soak]`),
  - add provider inventory repeated-call stability test (`[cuda][providers][inventory][stress]`).
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `25` to `28` gates.

### Patch 153 (landed in this step)
- CUDA native lane expansion tranche:
  - broaden strict-native-supported baseline dtypes for CUDA bridge-backed execution:
    - baseline/matmul lane now includes `float64`,
    - selection lane now includes integer/unsigned/bool + `float64`,
    - indexing lane now includes unsigned integer families,
    - scatter/gather lane now includes integer/unsigned/bool + `float64`.
  - broaden strict-native-supported layout/dim behavior:
    - selection supports all valid dims (not last-dim only),
    - selection/indexing/matmul/scatter-gather predicates no longer reject non-contiguous inputs when canonicalization path is available,
    - scatter supports broadcastable source shapes (not exact-shape only).
- Expanded CUDA validation:
  - add lane expansion parity gate (`[cuda][parity][native][expanded]`) covering:
    - stable/non-last-dim selection on integer tensors,
    - float64 non-contiguous matmul,
    - non-contiguous indexing take,
    - float64 scatter-broadcast and gather parity.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `28` to `29` gates.

### Patch 154 (landed in this step)
- CUDA reduction family tranche:
  - add native CUDA reduction capability API (`native/cuda/ReductionOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_REDUCTION`,
    - `PLUSS_CUDA_NATIVE_REDUCTION_STRICT`;
  - enforce strict/bridge routing in core dispatch for:
    - `sum`, `prod`, `mean`,
    - `amax`, `amin`,
    - `all`, `any`,
    - `argmax`, `argmin`.
  - keep strict-native precedence:
    - strict mode rejects bridge fallback when reduction route predicate is unsupported.
- Expanded CUDA validation:
  - add reduction runtime + bridge/policy contracts (`[cuda][reduction][runtime]`, `[cuda][bridge][reduction]`, `[cuda][bridge][policy][reduction]`);
  - extend torch-family parity + native-lane-expansion sweeps with reduction assertions.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `29` to `31` gates.

### Patch 155 (landed in this step)
- CUDA elementwise family tranche:
  - add native CUDA elementwise capability API (`native/cuda/ElementwiseOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_ELEMENTWISE`,
    - `PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT`;
  - enforce strict/bridge routing in core dispatch helpers for:
    - binary elementwise ops (`add/sub/mul/div/pow` and mapped consumers),
    - comparison elementwise ops (`eq/ne/lt/le/gt/ge`),
    - unary elementwise ops using unary dispatch helpers (`abs/neg/log1p/expm1` and mapped consumers).
  - keep strict-native precedence:
    - strict mode rejects bridge fallback when route predicates are unsupported.
- Expanded CUDA validation:
  - add elementwise runtime + bridge/policy contracts (`[cuda][elementwise][runtime]`, `[cuda][bridge][elementwise]`, `[cuda][bridge][policy][elementwise]`);
  - keep torch-family parity bridge/policy suites green with the new strict-elementwise lane.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `31` to `34` gates.

### Patch 156 (landed in this step)
- CUDA scan + activation/logical tranche:
  - add native CUDA scan capability API (`native/cuda/ScanOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_SCAN`,
    - `PLUSS_CUDA_NATIVE_SCAN_STRICT`;
  - enforce strict/bridge routing for:
    - `cumsum`,
    - `cumprod`;
  - remove CUDA hard CPU-only gating for activation/logical-style Series expressions by composing existing Series ops:
    - `logical_not/and/or/xor`,
    - `where`,
    - `relu`, `sigmoid`, `softplus`, `silu`, `gelu`, `leaky_relu`,
    - `softmax`, `log_softmax`, `logsumexp`.
- Expanded CUDA validation:
  - add scan runtime + bridge/policy contracts (`[cuda][scan][runtime]`, `[cuda][bridge][scan]`, `[cuda][bridge][policy][scan]`);
  - add activation/logical bridge + strict-policy contracts (`[cuda][bridge][activation]`, `[cuda][bridge][policy][activation]`);
  - extend parity suites with scan/activation assertions under bridge and strict lanes.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `34` to `38` gates.

### Patch 157 (landed in this step)
- CUDA masking family tranche:
  - add native CUDA masking capability API (`native/cuda/MaskingOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_MASKING`,
    - `PLUSS_CUDA_NATIVE_MASKING_STRICT`;
  - enforce strict/bridge routing in core masked ops for:
    - `masked_select`,
    - `masked_fill`,
    - `masked_scatter`.
- Expanded CUDA validation:
  - add masking runtime + bridge/policy contracts (`[cuda][masking][runtime]`, `[cuda][bridge][masking]`, `[cuda][bridge][policy][masking]`);
  - extend cluster stress loops with repeated masking bridge/policy runs.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `38` to `41` gates.

### Patch 158 (landed in this step)
- CUDA bitwise + compose family tranche:
  - add native CUDA bitwise capability API (`native/cuda/BitwiseOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_BITWISE`,
    - `PLUSS_CUDA_NATIVE_BITWISE_STRICT`;
  - add native CUDA compose capability API (`native/cuda/ComposeOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_COMPOSE`,
    - `PLUSS_CUDA_NATIVE_COMPOSE_STRICT`;
  - expose public codem runtime/capability surface for bitwise/compose in `PlatformRuntime` + `pluss::cuda::*`;
  - enforce strict/bridge routing in core dispatch for:
    - bitwise: `bitwise_and`, `bitwise_or`, `bitwise_xor`, `bitwise_not`,
    - compose: `unite`, `pile`.
- Expanded CUDA validation:
  - add bitwise runtime + bridge/policy contracts (`[cuda][bitwise][runtime]`, `[cuda][bridge][bitwise]`, `[cuda][bridge][policy][bitwise]`);
  - add compose runtime + bridge/policy contracts (`[cuda][compose][runtime]`, `[cuda][bridge][compose]`, `[cuda][bridge][policy][compose]`);
  - extend cluster stress loops with repeated bitwise and compose bridge/policy runs.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `41` to `47` gates.

### Patch 159 (landed in this step)
- CUDA high-rank batched-linalg tranche:
  - fix generalized leading-batch handling in `subsp::linearalg` compatibility paths by flattening/restoring arbitrary batch-rank shapes (not only `batch[0]`) for:
    - `det`, `inverse`, `cholesky`, `logdet`, `slogdet`,
    - `matrix_power`, `matrix_exp`, `pinverse`,
    - `qr_q`, `qr_r`, `svd_u`, `svd_s`, `svd_vh`,
    - `triangular_solve`, `cholesky_solve`, `cholesky_inverse`, `lu_solve`.
  - preserve platform semantics while fixing shape correctness for CPU/MPS/CUDA batched recursion paths.
- Expanded CUDA validation:
  - add batched rank-contract tests for CUDA bridge + strict-policy lanes (`test_patch159_cuda_linalg_batched_rank_contract.cpp`);
  - add dedicated CUDA mega-gates for batched linalg runtime/bridge/policy.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `47` to `50` gates.

### Patch 160 (landed in this step)
- CUDA expanded-linalg lane tranche:
  - broaden CUDA linalg capability predicates to include:
    - `Float16`,
    - `BFloat16`;
  - relax CUDA linalg route predicates to accept non-contiguous tensors and matrix-like tails (rank >= 2) while preserving dtype/shape validity checks;
  - broaden solve rhs shape checks to use trailing matrix dimensions (not only 2D exact layout checks).
- Expanded CUDA validation:
  - add expanded-linalg contracts (`test_patch160_cuda_linalg_expanded_contract.cpp`) for:
    - fp16/bf16 capability/runtime coverage,
    - non-contiguous bridge behavior,
    - strict-policy expanded-lane behavior;
  - add dedicated expanded-linalg runtime/bridge/policy mega-gates;
  - extend cluster stress loops with expanded-linalg bridge/policy repetitions.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `50` to `53` gates.

### Patch 161 (landed in this step)
- CUDA rank-aware matmul tranche:
  - expand CPU matmul core semantics from 2D-only to torch-style rank contracts:
    - `1D x 1D` (dot-like scalar lane),
    - `2D x 1D` and `1D x 2D`,
    - `ND x ND` with broadcasted batch prefixes and matrix-tail multiplication;
  - keep 2D compute kernel path as the core numeric lane, and route batched/mixed-rank matmul via reshape/expand/select composition;
  - add fp16/bf16 matmul support by deterministic float32 compute lane + dtype roundtrip at boundary.
- CUDA matmul capability expansion:
  - broaden native CUDA matmul route predicates to accept rank-aware compatible shapes (not only strict 2D) while preserving dtype and broadcast validity checks.
- Expanded CUDA validation:
  - add rank-aware matmul contracts (`test_patch161_cuda_matmul_rank_contract.cpp`) for:
    - runtime capability coverage on vector/matrix/batched lanes,
    - bridge correctness parity on vector/batched/broadcasted routes,
    - strict-policy acceptance on supported rank-aware routes;
  - add dedicated mega-gates for rank-aware matmul runtime/bridge/policy;
  - extend cluster stress loops with repeated rank-aware matmul bridge/policy runs.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `53` to `56` gates.

### Patch 162 (landed in this step)
- CUDA counting/set family tranche:
  - add native CUDA counting capability API (`native/cuda/CountingOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_COUNTING`,
    - `PLUSS_CUDA_NATIVE_COUNTING_STRICT`;
  - enforce strict/bridge routing for:
    - `unique`,
    - `bincount`,
    - `histc`;
  - keep strict-native precedence:
    - strict mode rejects bridge fallback when counting route predicates are unsupported.
- Expanded CUDA validation:
  - add counting runtime + bridge/policy contracts (`[cuda][counting][runtime]`, `[cuda][bridge][counting]`, `[cuda][bridge][policy][counting]`);
  - extend mega-gate and cluster stress runner with dedicated counting families.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `56` to `59` gates.

### Patch 163 (landed in this step)
- CUDA selection route-contract tranche:
  - add dedicated route-contract tests for CUDA selection execution paths:
    - `sort`,
    - `topk`,
    - `kthvalue`,
    - `argsort`;
  - verify bridge-backed execution parity against CPU reference lanes when CUDA bridge is enabled;
  - verify strict-native selection policy behavior:
    - execute supported strict lanes,
    - reject unsupported strict lanes fail-loud.
- Expanded CUDA validation:
  - add selection route contracts (`test_patch163_cuda_selection_route_contract.cpp`) with tags:
    - `[cuda][bridge][selection][route]`,
    - `[cuda][bridge][policy][selection][route]`,
    - `[cuda][bridge][policy][selection][reject]`;
  - expand CUDA mega-gate runner with dedicated selection route bridge/policy/reject gates;
  - extend CUDA cluster stress runner with repeated selection route bridge/policy/reject sweeps.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `59` to `62` gates.

### Patch 164 (landed in this step)
- CUDA factory/random policy tranche:
  - add native CUDA factory capability API (`native/cuda/FactoryOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_FACTORY`,
    - `PLUSS_CUDA_NATIVE_FACTORY_STRICT`;
  - add baseline capability predicates for factory families:
    - `zeros`, `ones`, `full`,
    - `arange`, `linspace`, `eye`,
    - `rand`, `randn`;
  - wire policy-aware CUDA transfer for public factory config paths (`Config.platform == CUDA`) so strict mode fail-loud behavior is enforced consistently for this family.
- Expanded CUDA validation:
  - add factory runtime + bridge/policy/reject contracts (`test_patch164_cuda_factory_contract.cpp`) with tags:
    - `[cuda][factory][runtime]`,
    - `[cuda][factory][policy][runtime]`,
    - `[cuda][bridge][factory]`,
    - `[cuda][bridge][policy][factory]`,
    - `[cuda][bridge][policy][factory][reject]`;
  - expand mega-gate and cluster stress runners with dedicated factory bridge/policy/reject sweeps.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `62` to `66` gates.

### Patch 165 (landed in this step)
- CUDA stochastic route-contract tranche:
  - add native CUDA stochastic capability API (`native/cuda/StochasticOps.*`) gated by:
    - `PLUSS_CUDA_NATIVE_BASELINE`,
    - `PLUSS_CUDA_NATIVE_STOCHASTIC`,
    - `PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT`;
  - enforce strict/bridge routing in stochastic invoke paths for:
    - `bernoulli`,
    - `uniform_`, `normal_`, `log_normal_`, `exponential_`, `geometric_`, `cauchy_`,
    - `random_`,
    - `multinomial`.
- Expanded CUDA validation:
  - add stochastic runtime + bridge/policy/reject contracts (`test_patch165_cuda_stochastic_contract.cpp`) with tags:
    - `[cuda][stochastic][runtime]`,
    - `[cuda][stochastic][policy][runtime]`,
    - `[cuda][bridge][stochastic]`,
    - `[cuda][bridge][policy][stochastic]`,
    - `[cuda][bridge][policy][stochastic][reject]`;
  - expand CUDA mega-gate runner with dedicated stochastic gates.
- Gate matrix expansion:
  - expand CUDA mega-gate matrix from `66` to `70` gates.

### Patch 166 (landed in this step)
- CUDA factory-like + stochastic-factory policy hardening tranche:
  - apply policy-aware CUDA transfer to stochastic factory constructors:
    - `randperm`,
    - `normal`,
    - `bernoulli`,
    - `multinomial(..., Config, ...)`;
  - apply policy-aware routing to like-factory config paths by delegating to policy-enforced base factories:
    - `zeros_like`, `ones_like`, `full_like`,
    - `rand_like`, `randn_like`,
    - `randint_like`,
    - `eye_like`.
- Expanded CUDA validation:
  - extend factory contracts with like-factory bridge + strict-policy rejection assertions;
  - extend stochastic contracts with stochastic-factory bridge + strict-policy rejection assertions.

### Patch 150 (landed in this step)
- MPS strict parity hardening:
  - promote native top-k selection policy to default-on for MPS (`PLUSS_MPS_NATIVE_TOPK_SELECTION=1` default),
  - extend strict torch-family lane to include `topk` and `kthvalue`,
  - add strict native linalg no-bridge counter assertion in torch-family strict suite.

### Patch 146 (landed in this step)
- MPS indexing/set lane expansion:
  - expand native `take/put_` strict lane coverage to exact float32-roundtrip dtypes (`bool`, `int8/uint8`, `int16/uint16`, `float16`, `bfloat16`, `float32`);
  - keep strict fail-loud behavior for unsupported accumulate lanes (for example integer `put_(accumulate=true)`).
- Expanded MPS strict-native gate matrix:
  - add strict native `put_` policy rejection gate.

## Rigorous Gate Execution (Catch2)
Use the new script to avoid large raw console dumps:

```bash
tools/run_codem_hardware_gates.sh --backend mps --env cpp_env --build-dir build_cpp_env
tools/run_codem_hardware_gates.sh --backend cuda --env cpp_env --build-dir build_cpp_env
```

Artifacts per run:
- `one_page.md` (compact summary),
- `summary.tsv` (machine-readable gate status),
- per-gate raw logs (`gate*.log`).

## Current Mega-Gate Artifact (MPS)
Latest sweep artifact:
- `outputs/mps_mega_gate_20260311_171843/one_page.md`
- `outputs/mps_mega_gate_20260311_171843/summary.tsv`

Status:
- `13 / 13` gate runs passed.

## Current Bootstrap Artifact (CUDA)
Latest sweep artifact:
- `outputs/cuda_mega_gate_20260313_180935/one_page.md`
- `outputs/cuda_mega_gate_20260313_180935/summary.tsv`

Status:
- `66 / 66` gates passing on latest sweep.

## CUDA Cluster Hardware Runner
- New runner script:
  - `tools/run_codem_cuda_cluster_hardware.sh`
- Example cluster invocation:
  - `tools/run_codem_cuda_cluster_hardware.sh --env cpp_env --build-dir build_cpp_env --stress-reps 5`
- Latest local validation artifact:
  - `outputs/cuda_cluster_hardware_20260311_172011/one_page.md`
  - `outputs/cuda_cluster_hardware_20260311_172011/summary.tsv`

## cuDNN Positioning
- cuDNN is a critical provider for convolutional/recurrent/attention-style DL lanes, not for this selection tranche.
- Current CUDA-on-Series tranche remains focused on core tensor math families first (selection, reduction, linalg baseline contracts).
- cuDNN integration is planned after CUDA storage/launch lanes are fully operational, in a dedicated DL operator family tranche.

Stable pointers:
- `outputs/latest_mps_mega_gate_path.txt`
- `outputs/latest_cuda_mega_gate_path.txt`
