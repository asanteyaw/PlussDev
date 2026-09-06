<!-- pluss-hand-authored -->

# Series::adjoint

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::adjoint() const -> Series
Series::mT() const -> Series
```

This page also covers `mT()`, the closely related view.

## What It Does

Both operations swap the **last two axes** of the series, leaving every leading
(batch) axis in place. On a stack of matrices they transpose each matrix
independently.

- **`mT()`** is the batched matrix transpose. It returns a stride view — no data
  is copied. It requires rank `>= 2`.
- **`adjoint()`** is the conjugate transpose: it swaps the last two axes *and*
  conjugates the values. For a real dtype there is nothing to conjugate, so it
  is the same rearrangement as `mT()`; the current implementation returns it as
  a materialized (contiguous) array rather than a view.

For a plain 2-D matrix both are simply the transpose, and match
[`t()`](t.md) and `transpose(0, 1)`.

The difference from [`transpose()`](transpose.md) is that you never name the
axes: `mT()` / `adjoint()` always mean "the last two", which is exactly what a
batched linear-algebra expression wants.

## Parameters

None.

## Returns

`Series`
    The series with its last two axes swapped (`adjoint()` also conjugates).
    `mT()` is a non-contiguous view; `adjoint()` on a real dtype is contiguous.

## Gradient

Both are differentiable. Swapping axes is its own inverse, so the backward
applies the same last-two-axis swap to the upstream gradient (for `adjoint()`
on a complex dtype it also conjugates). Values pass through unchanged.

## Examples

Example 1. On a `2 x 3` matrix, `mT()` is the ordinary transpose.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

std::cout << m.mT();
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

Example 2. On a batch of two `3 x 2` matrices (a `2 x 3 x 2` cube), `mT()`
transposes each matrix on its own, giving a `2 x 2 x 3` result.

```cpp
std::vector<double> d(12);
for (int i = 0; i < 12; ++i) d[i] = i + 1;
auto cube = pluss::make_series(d, {2, 2, 3});   // two 2 x 3 matrices

pluss::series::print(std::cout, cube.mT());
```

### Output

```text
Series(shape=[2,3,2], strides=[6,1,3], dtype=float64, platform=cpu, numel=12, contiguous=false)
data[0, :, :] =
⎡ 1.0000 4.0000 ⎤
⎢ 2.0000 5.0000 ⎥
⎣ 3.0000 6.0000 ⎦

data[1, :, :] =
⎡ 7.0000 10.0000 ⎤
⎢ 8.0000 11.0000 ⎥
⎣ 9.0000 12.0000 ⎦
```

The leading axis of length `2` is untouched; within each batch the `2 x 3`
block became `3 x 2`.

Example 3. `adjoint()` on a real matrix is the same rearrangement as `mT()`,
returned as a contiguous array.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

std::cout << m.adjoint();
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

Same values as `mT()`; note `contiguous=true` and the row-major `strides=[2,1]`.

Example 4. Both raise on a rank-1 input — there is no "last two axes" to swap.

```cpp
try {
  pluss::make_series({1.0, 2.0, 3.0}).mT();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
mT: expected a series with at least 2 dimensions, got a 1-D series
```

## Gotchas And Failure Modes

- Rank `>= 2` is required. Use [`t()`](t.md) if you specifically want the
  no-op-on-vectors behavior.
- `mT()` is a view (non-contiguous); `adjoint()` on a real dtype materializes.
  Neither should be assumed to alias or not alias the source.
- For a real dtype `adjoint()` and `mT()` produce identical values; the
  conjugation only matters for complex data.
- To swap two axes that are **not** the last two, use
  [`transpose()`](transpose.md) or [`swapaxes()`](swapaxes.md).

## Summary

`mT()` and `adjoint()` swap the last two axes of a series:

- batch axes are preserved; each trailing matrix is transposed independently
- `adjoint()` also conjugates (a no-op for real dtypes) and returns contiguous;
  `mT()` is a zero-copy view
- both require rank `>= 2` and are differentiable

## See Also

- [`Series::t`](t.md): the 2-D-only transpose that is a no-op on other ranks.
- [`Series::transpose`](transpose.md): swap any two named axes.
- [`Series::swapaxes`](swapaxes.md): swap any two axes of any rank.
- [`Series::linearalg`](linearalg.md): the batched matrix products `mT()` pairs
  with.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="addr.html">
    <span class="label">Previous</span>
    <span class="title">addr</span>
  </a>
  <a class="next-link" href="all.html">
    <span class="label">Next</span>
    <span class="title">all</span>
  </a>
</div>
