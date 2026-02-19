# CodeM CPU Torch-90 Tranche Cycle

This is the execution loop to drive CodeM toward "90% torch-level" CPU performance maturity.

## Goal

- Reach and sustain a **Torch-level score >= 90** on the CPU parity benchmark (`bench_torch_parity`).
- Keep correctness stable while increasing throughput and scalability.

## Score definition

- Benchmark computes per-op score as `clamp(torch_ms / series_ms, 0, 1)`.
- Final score is average of per-op scores, scaled to `[0, 100]`.
- Current scope is **perf-only** score over representative ops:
  - elementwise: `add`, `mul`, `exp`, `relu`
  - reductions: `sum`
  - indexing: `gather`, `scatter`
  - linear algebra: `matmul`

## Tranche loop

1. Baseline:
   - Build and run `codem_torch90_tranche`.
   - Capture score + per-op ratios.
2. Focus:
   - Pick top 1-2 worst operators by ratio (`Series/Torch`).
   - Implement fast paths first for contiguous Float32/Float64.
3. Guard:
   - Run smoke/tests and benchmark again.
   - Keep regressions out before moving to next tranche.
4. Promote:
   - If score improves and correctness is stable, lock tranche and start next one.

## Tranche priorities

1. Kernel hot paths:
   - contiguous specialization for high-volume ops
   - avoid iterator overhead in simple contiguous cases
2. Vectorization:
   - explicit SIMD backend routing (AVX/NEON) for hot kernels
3. Parallel scaling:
   - parallel-for and reduction scheduling tuning
4. Memory behavior:
   - temporary reduction/fusion opportunities
   - data locality and stride-aware layout paths
5. Promotion/casting cost:
   - minimize avoidable casts in hot loops

## Commands

```bash
cmake -S . -B build-rel \
  -DCMAKE_BUILD_TYPE=Release \
  -DPLUSSQIT_BUILD_CODEM=ON \
  -DPLUSS_ENABLE_TORCH=ON \
  -DBUILD_BENCHMARKS=ON \
  -DPKG_CONFIG_EXECUTABLE=/Users/yawasante/software/miniconda3/envs/cpp_env/bin/pkg-config \
  -DCMAKE_PREFIX_PATH=/Users/yawasante/software/miniconda3/envs/cpp_env/lib/python3.12/site-packages/torch \
  -DTorch_DIR=/Users/yawasante/software/miniconda3/envs/cpp_env/lib/python3.12/site-packages/torch/share/cmake/Torch

cmake --build build-rel --target codem_torch90_tranche -j8
```

For manual gate tuning:

```bash
./build-rel/PlussQit/bench_torch_parity --target-score=90 --emit-csv
```

Safety toggle (runtime):

```bash
# disable Storage block reuse pool for debugging/sanitizers
PLUSS_CODEM_STORAGE_POOL=off ./build-rel/PlussQit/bench_torch_parity
```

## Tranche results

- 2026-02-18, Tranche-1:
  - Torch-level score: **39.28 / 100**
  - Notable gains came from contiguous fast-path work in `add/mul`, `exp`, `relu`, `sum`, and 2D `gather/scatter`.
- 2026-02-18, Tranche-2:
  - Torch-level score: **46.27 / 100**
  - Added persistent worker-pool scheduling (`MSharp::parallel_for`) and backend-accelerated contiguous paths for `add/mul/exp/relu`.
- 2026-02-18, Tranche-3:
  - Torch-level score: **73.25 / 100**
  - Added chunked backend acceleration for `add/mul/exp`, optimized `sum_all` reductions, and a vDSP-backed contiguous 2D gather path.
- 2026-02-18, Tranche-4:
  - Torch-level score: **74.67 / 100**
  - Improved 2D gather/scatter fast-path loops (non-negative index specialization, branch reduction, and tighter bounds validation) while preserving correctness checks.
- 2026-02-19, Tranche-5:
  - Torch-level score: **75.01 / 100**
  - Tuned gather/scatter row scheduling grain and switched default 2D dim-1 gather to tighter unrolled pointer-based loops (vDSP gather retained only for very wide rows).
- 2026-02-19, Tranche-6:
  - Torch-level score: **95.61 / 100** (Release benchmark build)
  - Added release-benchmark normalization (dedicated `build-rel` flow), warmup for indexing/matmul in parity benchmark, and thread-local aligned storage reuse for repeated temporary allocations.
- 2026-02-19, Tranche-7:
  - Torch-level score: **90.61 / 100** (Release, median of 5 runs)
  - Added robustness controls for Storage pooling: runtime kill-switch (`PLUSS_CODEM_STORAGE_POOL`), ASan auto-disable path, plus new no-catch smoke coverage for pool toggle semantics and stress stability.
