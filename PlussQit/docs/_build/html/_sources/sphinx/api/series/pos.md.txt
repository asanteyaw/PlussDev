# Series::pos

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::pos(std::int64_t index) const -> Series
Series::pos(std::int64_t row, std::int64_t col) const -> Series
Series::pos(std::int64_t i, std::int64_t j, std::int64_t k) const -> Series
```

## What It Does

Returns one scalar cell as a rank-0 `Series`, with the calling rank enforced by
the overload you choose:

- `pos(index)` is the 1D form.
- `pos(row, col)` is the 2D form.
- `pos(i, j, k)` is the 3D form.

`pos()` is intentionally narrower than [`extract`](extract.md) or
[`index`](index.md): it does not slice, broadcast, or accept selector objects.
It is the "give me exactly one element" helper.

Negative indices are accepted on every axis and resolve from the end, just like
`select()` and `extract()` do.

## Parameters

**index / row / col / i / j / k** (`std::int64_t`)

The scalar coordinate(s) to read. Each coordinate is bounds-checked after
negative-index normalization:

- `-1` means "last element on this axis"
- `-2` means "second from the end"
- any coordinate still outside the axis after normalization throws
  `"Series::pos: index out of range"`

The rank must match the overload exactly. Calling the 2D overload on a 1D
series, or the 1D overload on a 2D series, throws an explicit rank error rather
than flattening implicitly.

## Returns

`Series`
    A rank-0 scalar `Series` holding the selected value.

## Examples

Example 1. In 1D, `-1` means the last element.

```cpp
auto vec = pluss::make_series(std::vector<double>{10, 20, 30, 40}, {4});
std::cout << vec.pos(-1);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=40
```

Example 2. In 2D, each axis normalizes independently, so `-1` on the second
axis means "last column of that row."

```cpp
auto mat = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
std::cout << mat.pos(1, -1);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=6
```

Example 3. The 3D overload follows the same rule on all three axes.

```cpp
auto cube = pluss::make_series(
    std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8},
    {2, 2, 2});
std::cout << cube.pos(1, -1, -1);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=8
```

## Notes

- `pos()` always returns a scalar rank-0 `Series`, never a view with a retained
  length-1 axis.
- Rank is part of the contract. If you need a dynamic selector that works across
  more shapes, use [`extract`](extract.md) instead.
- `pos()` is for scalar reads only. For scalar writes, use `index_put_()` or
  `extract_put_()` depending on how rich the selector needs to be.

## See Also

- [`Series::extract`](extract.md): the full dynamic indexing surface.
- [`Series::index`](index.md): the Torch-style entrypoint, especially for
  advanced index tensors.
- [`Series::slice`](slice.md): keep a range instead of collapsing to one scalar.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
