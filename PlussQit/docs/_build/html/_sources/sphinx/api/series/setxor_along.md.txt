<!-- pluss-hand-authored -->

# Series::setxor_along

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::setxor_along(const Series& other, std::int64_t dim) const -> Series
Series::setxor_along(const Series& other, std::int64_t dim, bool sorted) const -> Series
Series::setxor_along(const Series& other,
                     std::int64_t dim,
                     bool sorted,
                     bool assume_unique) const -> Series
```

## What It Does

`setxor_along()` computes the symmetric difference of slices along one axis.

It keeps the rows, columns, or higher-rank slices that belong to exactly one
side. This is the slice-wise counterpart to [`setxor1d()`](setxor1d.md).

## Parameters

**other** (`const Series&`)

The second input to compare against `self`.

**dim** (`std::int64_t`)

The axis whose slices are treated as the compared items.

Negative dims are accepted.

**sorted** (`bool`)

- `true`: sort the surviving slices
- `false`: keep left-exclusive slices first, then right-exclusive slices

**assume_unique** (`bool`)

- `false`: deduplicate slices before the symmetric-difference logic
- `true`: trust the inputs and skip that work

## Returns

`Series`
    A series containing the slices that belong to exactly one input along the
    target axis.

## Gradient

`setxor_along()` is not differentiable.

It is slice-membership composition.

## Examples

Example 1. `dim = 0` keeps rows that belong to only one side.

```cpp
auto left = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
}, pluss::ScalarType::Int64);
auto right = pluss::make_series({
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, left.setxor_along(right, 0, false, false));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 1 2 3 ⎤
⎣ 7 8 9 ⎦
```

Example 2. `dim = 1` switches the compared unit from rows to columns.

```cpp
std::cout << left.setxor_along(
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
pluss::Series(shape=[2,4], strides=[1,2], dtype=int64, platform=cpu, numel=8, contiguous=false)
data=[[1, 3, 7, 8],
  [4, 6, 9, 10]]
```

Example 3. The same slice-wise exclusive logic works on higher-rank input.

```cpp
auto nd_left =
    pluss::make_series({1, 2, 3, 4, 5, 6}, pluss::ScalarType::Int64).reshape({1, 2, 3});
auto nd_right =
    pluss::make_series({3, 2, 7, 4, 5, 6}, pluss::ScalarType::Int64).reshape({1, 2, 3});

std::cout << nd_left.setxor_along(nd_right, 2, false, false);
```

### Output

```text
pluss::Series(shape=[1,2,4], strides=[1,1,2], dtype=int64, platform=cpu, numel=8, contiguous=false)
data=[[[1, 3, 3, 7],
  [4, 6, 4, 6]]]
```

Example 4. `assume_unique=true` is a trust-me toggle and can change duplicate
behavior.

```cpp
std::cout << pluss::make_series({
    {1, 2},
    {1, 2},
    {3, 4},
}, pluss::ScalarType::Int64).setxor_along(
    pluss::make_series({
        {3, 4},
        {9, 9},
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
  [9, 9]]
```

## Gotchas And Failure Modes

- This method works slice-wise, not scalar-wise.
- `sorted=false` keeps left-exclusive survivors before right-exclusive
  survivors.
- `assume_unique=true` can alter duplicate behavior.
- All non-target dimensions must match exactly.
- Rank, dtype, and platform must match too.

## Summary

Use `setxor_along()` for slice-wise symmetric difference.

- choose `dim` to define the compared slice
- use it when exclusivity is about rows, columns, or higher-rank lanes
- keep `assume_unique=false` unless uniqueness is already guaranteed
- expect strict compatibility outside the target axis

## See Also

- [`Series::setxor1d`](setxor1d.md): flat value-wise symmetric difference.
- [`Series::unionize_along`](unionize_along.md): slice-wise union.
- [`Series::setdiff_along`](setdiff_along.md): slice-wise one-sided exclusion.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="setxor1d.html">
    <span class="label">Previous</span>
    <span class="title">setxor1d</span>
  </a>
  <a class="next-link" href="sigmoid.html">
    <span class="label">Next</span>
    <span class="title">sigmoid</span>
  </a>
</div>
