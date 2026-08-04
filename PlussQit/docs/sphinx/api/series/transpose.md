<!-- pluss-hand-authored -->

# Series::transpose

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::transpose(std::int64_t d0, std::int64_t d1) const -> Series
```

## What It Does

Swaps exactly two axes and returns the result as a view.

That last point matters: `transpose()` does not reorder the underlying values in
storage. It only changes how the same storage is interpreted, by swapping the
two selected sizes and strides. That is why a transposed result is usually
non-contiguous, and why downstream calls like [`view()`](view.md) may reject it
while [`reshape()`](reshape.md) may materialize a contiguous copy.

`transpose()` is the narrow, explicit two-axis operation. If you need to reorder
more than two axes at once, use [`permute()`](permute.md).

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

Example 1. Transpose a `2 x 3` matrix by swapping rows and columns.

```cpp
auto mat = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
std::cout << mat.transpose(0, 1);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

The values are not copied into a new contiguous `[3, 2]` buffer. The original
storage is being reread through swapped strides, which is why the output is
non-contiguous.

Example 2. Negative dimensions are normalized from the end.

```cpp
auto cube = pluss::make_series(
    std::vector<double>{
        1,  2,  3,  4,
        5,  6,  7,  8,
        9,  10, 11, 12,
        13, 14, 15, 16,
        17, 18, 19, 20,
        21, 22, 23, 24},
    {2, 3, 4});
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
auto tracked = pluss::make_series(
    std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {2, 3});
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
