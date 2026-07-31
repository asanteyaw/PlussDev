# Series::slice

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::slice(std::int64_t dim,
              std::int64_t start,
              std::int64_t end,
              std::int64_t step = 1) const -> Series

Series::slice(std::int64_t dim, std::int64_t start, std::int64_t end) const -> Series
Series::slice(std::int64_t dim, std::int64_t start) const -> Series
Series::slice(std::int64_t dim) const -> Series
```

## What It Does

Returns a cheap strided view over one axis, very much in the "basic slice"
sense from Torch or NumPy.

`slice()` is intentionally the light, view-oriented helper:

- positive step only
- one axis at a time
- no `Ellipsis`, `NewAxis`, or advanced index tensors

If you need richer selector grammar or negative-step reversal, use
[`extract()`](extract.md) instead.

## Parameters

**dim** (`std::int64_t`)

The axis to slice. Negative dims are allowed, so `-1` means "last axis".

**start** (`std::int64_t`)

The first included index on `dim`. Negative values resolve from the end.

**end** (`std::int64_t`)

The exclusive stop bound. Negative values resolve from the end. Omitted-end
overloads run to the natural end of the axis.

**step** (`std::int64_t`)

Stride along the sliced axis. Must be strictly positive here; a negative step
throws because `slice()` is the cheap view path, not the materializing one.

## Returns

`Series`
    A strided view over the requested range.

## Examples

Example 1. Take every other column from a `3 x 4` series.

```cpp
auto slice_src = pluss::make_series(
    std::vector<double>{10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33},
    {3, 4});
std::cout << slice_src.slice(1, 1, 4, 2);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[4,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[11, 13],
  [21, 23],
  [31, 33]]
```

The result is non-contiguous because `step = 2` skips through the original
storage.

Example 2. `slice()` composes cleanly with other transforms; here a derived
series is sliced along its first axis.

```cpp
auto cubic = slice_src.cube();
std::cout << cubic.slice(0, 0, 1);
```

### Output

```text
pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1000, 1331, 1728, 2197]]
```

## Notes

- `slice()` is for positive-step basic slicing only.
- Negative bounds are fine; negative `step` is not.
- Because `slice()` stays in the cheap-view lane, reversed selections belong on
  [`extract()`](extract.md), not here.

## See Also

- [`Series::extract`](extract.md): the richer selector surface, including
  negative-step slices and advanced indexing.
- [`Series::block`](block.md): a specialized 2D rectangle helper.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reshape.html">
    <span class="label">Previous</span>
    <span class="title">reshape</span>
  </a>
  <a class="next-link" href="block.html">
    <span class="label">Next</span>
    <span class="title">block</span>
  </a>
</div>
