# CodeM Routed-Methods Plan (CPU First)

This plan ranks missing `SeriesMethodsGen.h` routes by end-user impact for CPU production readiness.
The goal is strict correctness first, then vectorized kernel quality, then breadth.

## Rank 1: Core Tensor Workflow (highest impact)

1. Shape/index transforms used in almost every model/data path.
2. Immediate parity wins for NumPy/PyTorch-style scripting.

Priority ops:
- `flip`, `fliplr`, `flipud`
- `split_with_sizes`, `unsafe_split_with_sizes`
- `hsplit`, `vsplit`, `dsplit`
- `diff`
- `tril_`, `triu_`
- `fill_diagonal_`

Status:
- Implemented in `src/CodeM/compat/Invoke.cpp` in this patch.

## Rank 2: Sampling + Random Utilities

1. Frequently used for initialization and data simulation.
2. Needed for practical feature parity in CPU workflows.

Priority ops/factories:
- `logspace`, `randperm`, `normal`, `bernoulli`, `multinomial`
- `*_like` generator family (`empty_like`, `zeros_like`, `ones_like`, `full_like`, `rand_like`, `randn_like`, `randint_like`, `eye_like`)

Status:
- Implemented as factories in `include/pluss/codem/series_factory.h`.

## Rank 3: Advanced Linear Algebra + Stats

1. Important but less universal than Rank 1/2.
2. Requires careful numerical/kernel policy alignment.

Priority ops:
- `cholesky`, `inverse`, `det`, `linalg_*`
- advanced stats kernels (`cov`, `corrcoef`, distribution helpers)

Status:
- Planned; requires dedicated CPU kernels (beyond invoke routing).

## Rank 4: Sparse + Quantized + Backend-specific Surface

1. Necessary for full API completeness, but not for dense CPU production core.
2. Should fail explicitly until backend/runtime is ready.

Priority constructors:
- `sparse_coo_tensor`, `sparse_csr_tensor`, `sparse_csc_tensor`, `sparse_bsr_tensor`, `sparse_bsc_tensor`
- `quantize_per_tensor`, `quantize_per_channel`

Status:
- Explicit `NotImplemented` factory stubs added in `include/pluss/codem/series_factory.h`.

## Rank 5: Autograd/Device/Meta Surface

1. Out-of-scope for CPU-only runtime, but wrappers should remain generated for compatibility.
2. Must throw explicit, deterministic errors when invoked.

Examples:
- `_backward`, `_fw_primal`, `_autocast_*`, sparse layout internals.

Status:
- Generated wrappers exist; unrouted methods fail fast via invoke backstop.

