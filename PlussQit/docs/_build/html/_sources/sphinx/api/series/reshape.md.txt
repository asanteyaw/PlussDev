<!-- pluss-hand-authored -->

# Series::reshape

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::reshape(pluss::msharp::RangeView::Int new_sizes) const -> Series
```

## What It Does

Changes shape with a view-if-possible, contiguous-copy-if-necessary policy.

This is the high-level "I need this logical shape" API. `reshape()` first asks
whether the current `Series` can be reinterpreted directly. If yes, it behaves
like [`view()`](view.md). If not, it materializes a contiguous layout for the
current logical values and then produces the requested shape from that.

That difference is not cosmetic:

- `view()` means "fail unless this can stay a true alias"
- `reshape()` means "give me the requested logical shape, even if layout must
  be normalized first"

## Parameters

**new_sizes** (`pluss::msharp::RangeView::Int`)

The desired output shape.

In ordinary use you can pass brace literals such as `{3, 2}` directly, or reuse
another tensor’s [`sizes()`](sizes.md) result. The requested shape must
preserve total `numel()`.

## Returns

`Series`
    A `Series` with the requested logical shape. This may be a true alias if the
    source was already view-compatible, or a new contiguous result if it was not.

## Autograd Behavior

`reshape()` preserves reverse-mode connectivity in both paths:

- direct-view path: backward is the same shape-only mapping as `view()`
- fallback path: the internal contiguous staging copy is explicitly linked back
  to the source before the final view is created

That second point matters because it means a reshape of a non-contiguous tensor
still backpropagates to the original source rather than dead-ending at the
materialized temporary.

## Examples

The examples on this page build incrementally.

Example 1. On a contiguous source, `reshape()` behaves like `view()`.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << mat.reshape({3, 2});
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6]]
```

Example 2. On a non-contiguous transpose, `reshape()` succeeds by materializing
the transposed logical order first, then reshaping that order.

```cpp
std::cout << mat.transpose(0, 1).reshape({3, 2});
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

This is the core difference from Example 1. `reshape()` did **not** go back to
`mat`'s original row-major reading order. It preserved the transposed logical
values, made *that* layout contiguous, and then reshaped the result.

Example 3. Gradients still flow back through that non-contiguous fallback path.

```cpp
auto tracked_mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});
tracked_mat.tracks_slope_(true);

tracked_mat.transpose(0, 1)
    .reshape({3, 2})
    .select(0, 1)
    .sum()
    .reverse();

std::cout << tracked_mat.sensitivity();
```

### Output

```text
pluss::Series(shape=[2,3], strides=[1,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[0, 1, 0],
  [0, 1, 0]]
```

The selected row in the reshaped result was `[2, 5]`, so the gradient lands on
the original positions of `2` and `5` in `tracked_mat`. That confirms the
fallback path is not just numerically correct in the forward pass; it also stays
connected for reverse-mode autodiff.

## Notes

- `reshape()` is the safe default when you care about the final logical shape
  more than whether the operation stays a true alias.
- Equal `numel()` is still required; `reshape()` is flexible about layout, not
  about element count.
- On already contiguous inputs, it routes straight to [`view()`](view.md).

## See Also

- [`Series::view`](view.md): strict no-copy reinterpretation.
- [`Series::transpose`](transpose.md), [`Series::permute`](permute.md): common
  sources of non-contiguous layouts that later force `reshape()` onto its
  fallback path.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="view.html">
    <span class="label">Previous</span>
    <span class="title">view</span>
  </a>
  <a class="next-link" href="transpose.html">
    <span class="label">Next</span>
    <span class="title">transpose</span>
  </a>
</div>
