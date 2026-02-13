# CodeM roadmap (CPU-only)

This repo is structured ATen-style under `src/CodeM/` (headers + sources co-located).

## What is production-grade here?
- Correct metadata/view semantics (sizes/strides/storage_offset)
- Broadcasting for elementwise ops
- Type promotion policy (torch-like baseline; deviations centralized)
- Deterministic CPU kernels (with room for parallel/vectorized fastpaths)
- Tests and a single public umbrella include: `<pluss/pluss.h>`

## Next steps (recommended)
- Add parallel reductions (thread pool)
- Add boolean masking + advanced indexing (gather/scatter/masked_select)
- Expand dtype cast coverage (bf16/f16/f8 as storage + compute widen)
- Add BLAS-backed matmul (Accelerate on macOS)
- Expand `native_functions.yaml` and implement kernels using iterator templates.

## Core design principles

- **Method-first API**: if a `Series` exists, operate via methods (avoid global free-function bloat).
- **Factories** are free functions only when they *create* a new `Series`.
- **Cheap handles**: `Series` is a small value type holding a `SnapPtr<SeriesImpl>`.
- **Views and aliasing**: views share storage; in-place ops preserve alias semantics.
- **Debug invariants**: enforce non-overlapping/dense invariants where required.

## Infrastructure

- `pluss::msharp::SnapPtr<T>`: intrusive reference counted pointer for low overhead.
- `Storage` supports external ownership for safe interop.

## Naming deviations

PlussQit may rename a small set of methods away from Torch to reduce conceptual clutter (e.g., `unite/pile`). These are documented and versioned.
