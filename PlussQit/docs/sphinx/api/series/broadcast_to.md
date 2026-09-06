<!-- pluss-hand-authored -->

# Series::broadcast_to

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::broadcast_to(pluss::RangeView::Int shape) const -> Series
```

## What It Does

`broadcast_to()` returns a view of the series enlarged to `shape` by the
ordinary broadcasting rules — no data is copied.

The target `shape` is matched against the current shape from the right:

- extra leading dimensions in `shape` are added (the series is treated as if it
  had size-`1` axes in front)
- an axis of size `1` is stretched to the target size
- an axis whose size already equals the target is left alone
- any other mismatch is an error

The stretched axes get a **stride of `0`**, so every "copy" along them reads the
same element of storage. That makes `broadcast_to()` free and non-contiguous;
call [`contiguous()`](contiguous.md) or [`clone()`](clone.md) if you need a real
materialized array.

This is the same expansion the elementwise binary ops ([`add()`](add.md) and
friends) do implicitly; `broadcast_to()` just lets you request it explicitly.

## Parameters

**shape** (`pluss::RangeView::Int`)

The target shape. Must be broadcast-compatible with the current shape, and have
at least as many dimensions. A `-1` in an entry means "keep this axis's current
size".

## Returns

`Series`
    A stride-`0`-on-the-stretched-axes view with the requested shape. Same
    dtype and platform.

## Gradient

`broadcast_to()` is differentiable. Because one input element feeds many output
positions, the backward is the reverse: the sensitivity is **summed** over each
stretched (and each newly added leading) axis, then shaped back to the original.

## Examples

Example 1. A length-3 row broadcast to `2 x 3` — a new leading axis, both rows
aliasing the same storage.

```cpp
auto row = pluss::make_series({1.0, 2.0, 3.0});

std::cout << row.broadcast_to({2, 3});
```

### Output

```text
pluss::Series(shape=[2,3], strides=[0,1], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 2, 3],
  [1, 2, 3]]
```

The `strides=[0,1]` is the tell: stepping along axis `0` does not move in
storage.

Example 2. A `3 x 1` column stretched to `3 x 4`.

```cpp
auto col = pluss::make_series({1.0, 2.0, 3.0}).unsqueeze(1);

std::cout << col.broadcast_to({3, 4});
```

### Output

```text
pluss::Series(shape=[3,4], strides=[1,0], dtype=float64, platform=cpu, numel=12, contiguous=false)
data=[[1, 1, 1, 1],
  [2, 2, 2, 2],
  [3, 3, 3, 3]]
```

Example 3. A non-size-`1` mismatch is rejected.

```cpp
try {
  pluss::make_series({1.0, 2.0, 3.0}).broadcast_to({2, 4});
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
expand: non-singleton dimension mismatch
```

Example 4. The backward sums the upstream gradient over the stretched axis.

```cpp
auto row = pluss::make_series({1.0, 2.0, 3.0});
row.tracks_slope_(true);

row.broadcast_to({2, 3}).sum().reverse();

std::cout << "dx=" << row.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 2]
```

Each element was replicated into two rows, so each accumulates a gradient of
`2`.

## Gotchas And Failure Modes

- The result is a stride-`0` view, not a copy. Mutating it in place, or relying
  on it being contiguous, will surprise you — materialize with
  [`contiguous()`](contiguous.md) first.
- Only size-`1` (and newly added leading) axes can grow. A `[3]` cannot
  `broadcast_to({2, 4})`.
- The rejection message names `expand`, the shared implementation.

## Summary

`broadcast_to(shape)` is an explicit, zero-copy broadcast view:

- right-aligned shape match; size-`1` axes get stride `0`
- non-contiguous; materialize if needed
- differentiable, backward sums the gradient over the stretched axes

## See Also

- [`Series::expand`](expand.md): the same operation; `expand()` additionally
  accepts `-1` to mean "keep this axis".
- [`Series::repeat`](repeat.md): the copying alternative when you need real
  independent storage.
- [`Series::contiguous`](contiguous.md): turn the broadcast view into a dense
  array.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bmm.html">
    <span class="label">Previous</span>
    <span class="title">bmm</span>
  </a>
  <a class="next-link" href="cartprod.html">
    <span class="label">Next</span>
    <span class="title">cartprod</span>
  </a>
</div>
