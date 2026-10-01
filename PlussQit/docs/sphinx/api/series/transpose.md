<!-- pluss-hand-authored -->

# Series::transpose

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::transpose(std::int64_t d0, std::int64_t d1) const -> Series
Series& Series::transpose_(std::int64_t dim0, std::int64_t dim1) const
```

## What It Does

Swaps exactly two axes and returns the result as a view.

That last clause is the part users need to internalize. `transpose()` is not
"rebuild the tensor with values moved somewhere else." It is:

- same storage
- same values
- same `numel()`
- different axis interpretation

Because of that:

- the result is often non-contiguous
- [`view()`](view.md) may reject it afterward
- [`reshape()`](reshape.md) may need to materialize if you ask for a later final
  shape

`transpose()` is the two-axis special case of [`permute()`](permute.md). Use it
when the operation is conceptually "swap these two axes," not "rewrite the
entire axis order."

## Parameters

**d0** (`std::int64_t`)

The first axis to swap.

**d1** (`std::int64_t`)

The second axis to swap.

Both parameters accept:

- positive indexing from the front
- negative indexing from the back

After normalization, each dimension must refer to a real axis in
`[0, self.dim())`.

One toggle-like behavior matters here:

- if `d0 == d1`, the method is a logical no-op

That makes the API predictable instead of forcing callers to special-case equal
axes outside the method.

## Returns

`Series`
    A view whose two selected axes have been exchanged.

The return contract is structural:

- shape changes
- stride pattern usually changes
- contiguity may be lost
- values are not numerically recomputed

## Gradient

`transpose()` is differentiable and exact.

Backward is just the inverse axis swap, which for a simple two-axis exchange is
the same swap again. That is why `transpose()` is safe structural code in
autodiff pipelines: it is metadata movement, not lossy arithmetic.

## In-Place Form

`transpose_()` is the in-place sibling of [`transpose()`](transpose.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `transpose_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
x.transpose_(0, 1);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

## Verified Examples

Example 1. Swap the row and column axes of a `2 x 3` matrix.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});
std::cout << mat.transpose(0, 1);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

The non-contiguous result is not an implementation accident. It is the natural
consequence of rereading the same storage through swapped strides.

Example 2. Negative dimensions are normalized from the end.

```cpp
auto cube = pluss::make_series(
    {
        {
            {1.0, 2.0, 3.0, 4.0},
            {5.0, 6.0, 7.0, 8.0},
            {9.0, 10.0, 11.0, 12.0},
        },
        {
            {13.0, 14.0, 15.0, 16.0},
            {17.0, 18.0, 19.0, 20.0},
            {21.0, 22.0, 23.0, 24.0},
        },
    });
std::cout << cube.transpose(-1, -2);
```

### Output

```text
pluss::Series(shape=[2,4,3], strides=[12,1,4], dtype=float64, platform=cpu, numel=24, contiguous=false)
data=[[[1, 5, 9],
  [2, 6, 10],
  [3, 7, 11],
  [4, 8, 12]],
  [[13, 17, 21],
  [14, 18, 22],
  [15, 19, 23],
  [16, 20, 24]]]
```

So `-1` and `-2` are just another way to name the last two axes.

Example 3. Swapping an axis with itself is a no-op.

```cpp
std::cout << mat.transpose(1, 1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 3],
  [4, 5, 6]]
```

This is worth documenting because it shows there is no hidden "dims must be
different" precondition.

Example 4. Gradients route back to the original source positions.

```cpp
auto tracked = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});
tracked.tracks_slope_(true);

auto t = tracked.transpose(0, 1);
t.select(0, 1).sum().reverse();
std::cout << tracked.sensitivity();
```

### Output

```text
pluss::Series(shape=[2,3], strides=[1,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[0, 1, 0],
  [0, 1, 0]]
```

The selected row of the transposed result corresponds to the second column of
the original matrix, so that is exactly where the gradient lands.

## Notes

- `transpose()` is a two-axis view op.
- It preserves values and `numel()`, but often changes contiguity.
- For a full axis permutation, use [`permute()`](permute.md).
- If you later need a specific final shape, remember that the transposed result
  may no longer be directly view-compatible.

## See Also

- [`Series::permute`](permute.md): full axis reordering.
- [`Series::reshape`](reshape.md): target-shape-first reshape with fallback.
- [`Series::view`](view.md): strict aliasing reinterpretation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tracks_slope_.html">
    <span class="label">Previous</span>
    <span class="title">tracks_slope_</span>
  </a>
  <a class="next-link" href="permute.html">
    <span class="label">Next</span>
    <span class="title">permute</span>
  </a>
</div>
