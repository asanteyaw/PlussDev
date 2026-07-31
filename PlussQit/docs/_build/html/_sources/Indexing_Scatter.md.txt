# Indexing, index_put, scatter

## index_put / index_put_

PlussQit supports CPU-only advanced indexing inspired by Torch.

Supported index forms:

- `Int64` index tensors per-dimension (any subset of dimensions).
- Per-dimension `Bool` masks (1D mask for the corresponding dimension).
- You may mix bool and int indices across dimensions.
- If indices are provided for only `k < ndim` dimensions, they apply to the leading dimensions.

### Semantics

- Advanced indices are broadcast together.
- The advanced broadcasted result is *moved to the front* of the indexing result (Torch behavior for non-contiguous advanced dims).
- Any dimension without an index is treated as a full slice (`:`).

### Overlap rules

- If `accumulate=false` and indices overlap, `index_put_` throws (explicit and safe).
- If `accumulate=true`, overlapping indices accumulate.

## scatter_reduce knobs

`scatter_reduce_*` supports:

- `include_self`:
  - `true` (default): start from `self` and apply reductions
  - `false`: start from `initial` (if provided) or the identity for the reduction

- `initial_scalar` (optional): scalar initial value applied to all output elements
