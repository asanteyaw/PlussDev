# Series::block

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::block(std::int64_t row_start,
              std::int64_t row_count,
              std::int64_t col_start,
              std::int64_t col_count) const -> Series
```

## What It Does

Extracts one rectangular 2D sub-block as a view.

`block()` is narrower than [`slice()`](slice.md) and far narrower than
[`extract()`](extract.md), but that narrowness is useful: when the intent is
"give me rows `r..r+n` and columns `c..c+m` from a matrix", `block()` says that
directly.

## Parameters

**row_start** (`std::int64_t`)

The first row to include. Negative values count from the end.

**row_count** (`std::int64_t`)

How many rows to keep from `row_start`. Must be non-negative.

**col_start** (`std::int64_t`)

The first column to include. Negative values count from the end.

**col_count** (`std::int64_t`)

How many columns to keep from `col_start`. Must be non-negative.

The requested rectangle must stay fully inside the matrix bounds. `block()` does
not clamp or auto-shrink the request.

## Returns

`Series`
    A 2D view over the requested rectangle.

## Examples

Example 1. Pull the center-right `2 x 3` rectangle from a `3 x 4` matrix.

```cpp
auto block_src = pluss::make_series(
    std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
    {3, 4});
std::cout << block_src.block(1, 2, 1, 3);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[4,1], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[6, 7, 8],
  [10, 11, 12]]
```

The output is a view into the original matrix, so it keeps the parent row stride
of `4` even though only `3` columns are visible.

## Notes

- `block()` is 2D-only.
- Unlike [`extract()`](extract.md), this helper is always rectangular and always
  row/column based.
- The result is often non-contiguous because it usually inherits the parent row
  stride.

## See Also

- [`Series::slice`](slice.md): one-axis basic slicing.
- [`Series::extract`](extract.md): dynamic indexing with mixed selector kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="extract.html">
    <span class="label">Next</span>
    <span class="title">extract</span>
  </a>
</div>
