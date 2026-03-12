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
- Correctness coverage: `18 / 40`
- Backend completeness: `24 / 25`
- Stability/testing depth: `16 / 20`
- Observability/perf readiness: `12 / 15`
- **Total: `70 / 100`**
- Distance to torch-level maturity: **`30%` remaining**

Rationale: MPS score lift is backed by real native `take/put_` lane expansion (exact small-integer/bool + fp16/bf16/fp32) and strict no-fallback policy coverage. CUDA score lift is backed by real provider/runtime introspection (cuBLAS/cuSOLVER/cuDNN probes + primary-device metadata), backend-info provider wiring, and rigorous cluster-grade hardware gate automation. CUDA native compute kernels are still pending, so that track remains bridge-backed correctness with explicit policy controls.

## PyTorch CUDA Reference Inputs Audited
Reference root:
`/Users/yawasante/Documents/Doctrate/Thesis/Python/second_project/Codes/pytorch`

Primary files read for CUDA tranche design:
- `aten/src/ATen/native/native_functions.yaml`
- `aten/src/ATen/native/cuda/LinearAlgebra.cu`
- `aten/src/ATen/native/cuda/linalg/BatchLinearAlgebra.cpp`
- `aten/src/ATen/native/cuda/linalg/BatchLinearAlgebraLib.cpp`
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
- `outputs/cuda_mega_gate_20260311_172017/one_page.md`
- `outputs/cuda_mega_gate_20260311_172017/summary.tsv`

Status:
- `25` gate matrix now defined (legacy artifacts may still reflect `22` before parity-gate expansion).

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
