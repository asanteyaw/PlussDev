# Series::reshape

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::reshape(pluss::msharp::RangeView::Int new_sizes) const -> Series
```

## What It Does

Changes shape with a "view if possible, copy if necessary" policy.

`reshape()` first asks whether the source can be reinterpreted directly. If yes,
it delegates to [`view()`](view.md). If not, it materializes a contiguous copy
and then views that contiguous buffer under the requested shape.

That makes `reshape()` the forgiving surface, while `view()` is the strict one.

## Parameters

**new_sizes** (`pluss::msharp::RangeView::Int`)

The desired output shape. The total element count must still match the source.

Unlike [`view()`](view.md), `reshape()` does not require the input to already be
contiguous. Non-contiguous inputs are normalized internally through a
`contiguous().view(...)` fallback path.

## Returns

`Series`
    A reshaped `Series`. If the input was already view-compatible, this can be a
    true view; otherwise it is a new contiguous result.

## Examples

Example 1. On a contiguous source, `reshape()` behaves like `view()`.

```cpp
auto mat = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
const std::array<std::int64_t, 2> shape_3x2{{3, 2}};

std::cout << mat.reshape(
    std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6]]
```

Example 2. On a non-contiguous transpose, `reshape()` succeeds by first
materializing contiguous order, then reshaping that order.

```cpp
std::cout << mat.transpose(0, 1).reshape(
    std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

That output is not the same as Example 1, and that is the point: once the
transpose changed the logical iteration order, `reshape()` preserved the
transposed values, not the original contiguous storage layout from `mat`.

## Notes

- `reshape()` is the safe choice when you care about the final shape more than
  whether the operation is a true view.
- It still preserves element count; mismatched `numel()` is not allowed.
- On already contiguous inputs, `reshape()` does not pay an extra copy tax; it
  simply routes to [`view()`](view.md).

## See Also

- [`Series::view`](view.md): the strict no-copy version.
- [`Series::slice`](slice.md): change visible layout by taking a range rather
  than reinterpreting shape.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="view.html">
    <span class="label">Previous</span>
    <span class="title">view</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
