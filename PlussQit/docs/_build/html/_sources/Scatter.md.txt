# Scatter

Torch-inspired scatter ops (CPU-only subset).

## Methods

```cpp
auto y = x.scatter_add(dim, index, src);
x.scatter_add_(dim, index, src);

auto z = x.scatter_reduce(dim, index, src, "sum");
auto m = x.scatter_reduce(dim, index, src, "amax");
auto n = x.scatter_reduce(dim, index, src, "amin");

// include_self controls whether existing values participate.
auto y2 = x.scatter_reduce(dim, index, src, "sum", /*include_self=*/false);
```

## Semantics (Patch 49)

- `x` is the destination/base tensor (output has the same shape as `x`).
- `index` provides destination indices along `dim`.
- `src` is broadcastable to `index` (Torch-like).

Shape rules implemented:
- `index.dim == x.dim`
- for dims != `dim`: `index.size(d) == x.size(d)`
- for dim: `index.size(dim)` may differ from `x.size(dim)`
- `src` must be broadcastable to `index` (each dim either same or 1)

Constraints (still intentional):
- index dtype: int64
- current in-place implementation requires contiguous tensors
