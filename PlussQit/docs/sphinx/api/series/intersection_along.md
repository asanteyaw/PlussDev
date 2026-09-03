<!-- pluss-hand-authored -->

# Series::intersection_along

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::intersection_along(const Series& other, std::int64_t dim) const -> Series
Series::intersection_along(const Series& other, std::int64_t dim, bool sorted) const -> Series
Series::intersection_along(const Series& other,
                           std::int64_t dim,
                           bool sorted,
                           bool assume_unique) const -> Series
```

## What It Does

`intersection_along()` keeps the slices along `dim` that appear in both inputs.

This is the axis-aware counterpart to [`intersect1d()`](intersect1d.md). The
key difference is that equality is checked slice-by-slice instead of
value-by-value after flattening.

## Parameters

**other** (`const Series&`)

The second input that determines which slices count as shared.

**dim** (`std::int64_t`)

The axis whose slices are compared.

Negative dims are accepted and count from the back.

**sorted** (`bool`)

- `true`: sort the surviving slices
- `false`: preserve encounter order inherited from `self`

**assume_unique** (`bool`)

- `false`: deduplicate slices first
- `true`: trust the inputs and skip that work

If that trust is misplaced, duplicate matching slices can survive.

## Returns

`Series`
    A series whose target dimension contains only the slices shared by both
    inputs.

## Gradient

`intersection_along()` is not differentiable.

It is slice-membership filtering.

## Examples

Example 1. `dim = 0` intersects rows.

```cpp
auto left = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
}, pluss::ScalarType::Int64);
auto right = pluss::make_series({
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, left.intersection_along(right, 0, false, false));
```

### Output

```text
Series(shape=[1,3], strides=[3,1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=
[ 4 5 6 ]
```

Example 2. `dim = 1` intersects columns instead.

```cpp
std::cout << left.intersection_along(
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
pluss::Series(shape=[2,1], strides=[1,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[2],
  [5]]
```

Example 3. Higher-rank inputs can use the same idea with a trailing axis.

```cpp
auto nd_left =
    pluss::make_series({1, 2, 3, 4, 5, 6}, pluss::ScalarType::Int64).reshape({1, 2, 3});
auto nd_right =
    pluss::make_series({3, 2, 7, 4, 5, 6}, pluss::ScalarType::Int64).reshape({1, 2, 3});

std::cout << nd_left.intersection_along(nd_right, 2, false, false);
```

### Output

```text
pluss::Series(shape=[1,2,1], strides=[2,1,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[[2],
  [5]]]
```

Example 4. `assume_unique=true` can preserve repeated matching slices.

```cpp
std::cout << pluss::make_series({
    {1, 2},
    {1, 2},
    {3, 4},
}, pluss::ScalarType::Int64).intersection_along(
    pluss::make_series({
        {1, 2},
        {9, 9},
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

- Equality is slice-wise, not scalar-wise.
- `assume_unique=true` can preserve duplicate slices.
- `sorted=false` preserves `self`'s surviving slice order.
- Rank, dtype, platform, and non-target dimensions must match.
- Invalid axes fail through the shared set-along contract.

## Summary

Use `intersection_along()` when whole rows, columns, or higher-rank slices must
match.

- `dim` chooses the slice unit
- `sorted=false` preserves the left input's surviving order
- `assume_unique=true` is only safe when slices are already unique
- compatibility is strict outside the target axis

## See Also

- [`Series::intersect1d`](intersect1d.md): flat value-wise intersection.
- [`Series::unionize_along`](unionize_along.md): slice-wise distinct union.
- [`Series::setdiff_along`](setdiff_along.md): keep only left-exclusive slices.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersection.html">
    <span class="label">Previous</span>
    <span class="title">intersection</span>
  </a>
  <a class="next-link" href="invalid.html">
    <span class="label">Next</span>
    <span class="title">invalid</span>
  </a>
</div>
