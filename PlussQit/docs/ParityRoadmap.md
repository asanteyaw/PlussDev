# Torch CPU parity roadmap

This document is a living checklist of remaining work to reach "Torch-like CPU completeness".

## Big remaining mountains
- Method surface (thousands of ops + overloads)
- Full promotion lattice parity (Torch rules are extensive)
- Advanced indexing corner cases (broadcasting variants, views/strides, negative tests)
- Overlap / alias rules for in-place updates across arbitrary strides
- SIMD coverage across many more kernels

## Patch 52 progress
- index_put_ accumulate supports half/bf16/complex/integer widening paths (advanced path)
- tanh/softplus contiguous hot paths (float32/64)

## Torch-90 tranche loop

- Active measured cycle is documented in `docs/Torch90_TrancheCycle.md`.
- Gate command: `cmake --build build --target codem_torch90_tranche`.
