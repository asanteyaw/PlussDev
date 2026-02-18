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
cmake -S . -B build \
  -DPLUSSQIT_BUILD_CODEM=ON \
  -DPLUSS_ENABLE_TORCH=ON \
  -DBUILD_BENCHMARKS=ON

cmake --build build --target codem_torch90_tranche -j8
```

For manual gate tuning:

```bash
./build/PlussQit/bench_torch_parity --target-score=90 --emit-csv
```

## Tranche results

- 2026-02-18, Tranche-1:
  - Torch-level score: **39.28 / 100**
  - Notable gains came from contiguous fast-path work in `add/mul`, `exp`, `relu`, `sum`, and 2D `gather/scatter`.
- 2026-02-18, Tranche-2:
  - Torch-level score: **46.27 / 100**
  - Added persistent worker-pool scheduling (`MSharp::parallel_for`) and backend-accelerated contiguous paths for `add/mul/exp/relu`.
