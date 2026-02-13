# Performance roadmap (CPU)

PlussQit is designed so correctness and API stability come first, while still enabling aggressive CPU performance later.

## Current state

- Elementwise loops use `SeriesIterator` with contiguous fast paths where available.
- Views are cheap; storage is shared.
- Many methods exist early for API stability; unimplemented ones throw.

## SIMD and parallelism

SIMD-grade vectorization is most effective when applied to a stable kernel set and stable dtype/layout rules.
Once the semantics and edge-cases are fixed, PlussQit can add:

- contiguous-specialized kernels (Float32/Float64/Float16/BFloat16) with NEON (Apple) and AVX2/AVX-512 (x86)
- small-threadpool parallel-for for large tensors
- fused loops (where safe) to reduce memory traffic

This avoids rewriting vectorized kernels multiple times due to shifting semantics.
