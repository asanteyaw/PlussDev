<!-- pluss-hand-authored -->

# Series::transpose

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::transpose(std::int64_t d0, std::int64_t d1) const -> Series
```

## What It Does

Swaps exactly two axes and returns the result as a view.

That last point is the real contract. `transpose()` does not numerically
reorder values in storage. It swaps the selected sizes and strides, so the same
storage is reread under a different axis interpretation. That is why the result
is usually non-contiguous, and why downstream calls such as [`view()`](view.md)
may reject it while [`reshape()`](reshape.md) may need to materialize a
contiguous copy.

`transpose()` is the narrow, explicit two-axis operation. If you need to reorder
an entire rank at once, use [`permute()`](permute.md).

## Parameters

**d0** (`std::int64_t`)

The first axis to swap.

**d1** (`std::int64_t`)

The second axis to swap.

Both parameters accept negative indexing. `-1` means the last axis, `-2` the
second-to-last, and so on. After normalization, both axes must be in
`[0, self.dim())`.

If `d0 == d1`, the operation is a no-op and the returned handle has the same
logical layout as `self`.

## Returns

`Series`
    A view with the two selected axes exchanged.

## Gradient

`transpose()` is differentiable and exact. Its backward pass is simply the same
axis swap applied to the outgoing sensitivity:

```text
transpose(d0, d1).backward == transpose(d0, d1)
```

That means no approximation, no averaging, and no data-dependent rule: whatever
position moved from `(i, j, ...)` to `(j, i, ...)` in the forward pass receives
its gradient back through the inverse layout change in the backward pass.

## Examples

The examples on this page build incrementally.

Example 1. Transpose a `2 x 3` matrix by swapping rows and columns.

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

The values are not copied into a fresh contiguous `[3, 2]` buffer. The original
storage is being reread through swapped strides, which is why the result is
non-contiguous.

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

`-1` and `-2` refer to the last two axes, so this swaps the trailing
`[3, 4]` layout into `[4, 3]` without touching the leading axis.

Example 3. Backpropagating through a transpose routes sensitivity back to the
corresponding source positions.

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

`t.select(0, 1)` selects the second row of the transposed result, which is the
second column of the original matrix. The gradient therefore lands exactly on
that original column and nowhere else.

## Notes

- `transpose()` is a two-axis view op, not a copy.
- Negative dimensions are supported.
- For a full axis permutation, use [`permute()`](permute.md) instead.

## See Also

- [`Series::permute`](permute.md): reorder more than two axes at once.
- [`Series::reshape`](reshape.md): if you need a final target shape rather than
  a pure axis swap.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reshape.html">
    <span class="label">Previous</span>
    <span class="title">reshape</span>
  </a>
  <a class="next-link" href="permute.html">
    <span class="label">Next</span>
    <span class="title">permute</span>
  </a>
</div>
