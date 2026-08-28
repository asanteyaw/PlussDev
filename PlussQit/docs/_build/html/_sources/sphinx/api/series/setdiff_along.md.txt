<!-- pluss-hand-authored -->

# Series::setdiff_along

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::setdiff_along(const Series& other, std::int64_t dim) const -> Series
Series::setdiff_along(const Series& other, std::int64_t dim, bool sorted) const -> Series
Series::setdiff_along(const Series& other,
                      std::int64_t dim,
                      bool sorted,
                      bool assume_unique) const -> Series
```

## What It Does

`setdiff_along()` keeps the slices from `self` that do not appear in `other`
along the chosen axis.

This is the slice-wise counterpart to [`setdiff1d()`](setdiff1d.md). It is the
right method when the compared unit is a row, a column, or a higher-rank slice
rather than one scalar value.

## Parameters

**other** (`const Series&`)

The right-hand side whose slices are removed from `self`.

**dim** (`std::int64_t`)

The axis whose slices are treated as the compared items.

Negative dims are accepted.

**sorted** (`bool`)

- `true`: sort the surviving slices
- `false`: preserve the left input's encounter order

**assume_unique** (`bool`)

- `false`: deduplicate slices before subtraction
- `true`: trust the inputs and skip that cleanup

If the slices are not actually unique, duplicate left-exclusive slices can
remain.

## Returns

`Series`
    A series containing only the slices from `self` that are absent from
    `other` along the target axis.

## Gradient

`setdiff_along()` is not differentiable.

It is slice-membership subtraction.

## Examples

Example 1. `dim = 0` removes matching rows.

```cpp
auto left = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
}, pluss::ScalarType::Int64);
auto right = pluss::make_series({
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, left.setdiff_along(right, 0, false, false));
```

### Output

```text
Series(shape=[1,3], strides=[3,1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=
[ 1 2 3 ]
```

Example 2. `dim = 1` removes matching columns instead.

```cpp
std::cout << left.setdiff_along(
    pluss::make_series({
        {2, 7, 8},
        {5, 9, 10},
    }, pluss::ScalarType::Int64),
    1,
    false,
    false);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[[1, 3],
  [4, 6]]
```

Example 3. Higher-rank inputs can use the same axis-based subtraction.

```cpp
auto nd_left =
    pluss::make_series({1, 2, 3, 4, 5, 6}, pluss::ScalarType::Int64).reshape({1, 2, 3});
auto nd_right =
    pluss::make_series({3, 2, 7, 4, 5, 6}, pluss::ScalarType::Int64).reshape({1, 2, 3});

std::cout << nd_left.setdiff_along(nd_right, 2, false, false);
```

### Output

```text
pluss::Series(shape=[1,2,2], strides=[4,2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[[[1, 3],
  [4, 6]]]
```

Example 4. `assume_unique=true` can preserve duplicate left-exclusive slices.

```cpp
std::cout << pluss::make_series({
    {1, 2},
    {1, 2},
    {3, 4},
}, pluss::ScalarType::Int64).setdiff_along(
    pluss::make_series({
        {3, 4},
    }, pluss::ScalarType::Int64),
    0,
    false,
    true);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2],
  [1, 2]]
```

## Gotchas And Failure Modes

- The operation is asymmetric.
- `dim` decides what one removable unit is.
- `assume_unique=true` can preserve repeated surviving slices.
- Non-target dimensions must match exactly.
- Dtype, platform, and rank must match too.

## Summary

Use `setdiff_along()` for slice-wise left-exclusive subtraction.

- rows, columns, or higher-rank slices can be the compared unit
- `sorted=false` preserves the left input's surviving order
- `assume_unique=true` can keep duplicate survivors
- compatibility is strict outside the target axis

## See Also

- [`Series::setdiff1d`](setdiff1d.md): flat value-wise subtraction.
- [`Series::intersection_along`](intersection_along.md): keep shared slices.
- [`Series::setxor_along`](setxor_along.md): keep slices exclusive to either
  side.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="setdiff1d.html">
    <span class="label">Previous</span>
    <span class="title">setdiff1d</span>
  </a>
  <a class="next-link" href="setxor1d.html">
    <span class="label">Next</span>
    <span class="title">setxor1d</span>
  </a>
</div>
