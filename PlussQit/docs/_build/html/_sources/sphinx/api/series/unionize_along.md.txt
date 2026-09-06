<!-- pluss-hand-authored -->

# Series::unionize_along

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::unionize_along(const Series& other, std::int64_t dim) const -> Series
Series::unionize_along(const Series& other, std::int64_t dim, bool sorted) const -> Series
```

## What It Does

`unionize_along()` computes a distinct union of slices along one axis instead of
flattening everything into one value list.

That distinction is the whole point of the method:

- `union1d()` and `unionize()` compare scalar values after flattening
- `unionize_along()` compares whole slices along `dim`

For `dim = 0`, the compared slices are rows or front slices. For `dim = 1` on
a matrix, the compared slices are columns. On higher-rank input, the same idea
extends naturally: the target axis defines the unit whose duplicates are
removed.

## Parameters

**other** (`const Series&`)

The second input whose slices are unioned with the slices from `self`.

`self` and `other` must agree on:

- dtype
- platform
- rank
- every non-target dimension

Only the size of the target dimension may differ.

**dim** (`std::int64_t`)

The axis whose slices should be treated as the compared units.

- `0`: compare rows/front slices
- `1`: compare columns on a matrix
- negative dims are accepted and count from the back

Changing `dim` changes what one distinct item means.

**sorted** (`bool`)

Controls the order of the surviving slices.

- `true`: sort the distinct slices
- `false`: preserve first encounter order from `self`, then `other`

## Returns

`Series`
    A series with the same non-target dimensions as the inputs and a target
    dimension large enough to hold the distinct slice union.

## Gradient

`unionize_along()` is not differentiable.

It is a slice-membership and ordering operation.

## Examples

Example 1. `dim = 0` treats rows as the compared slices.

```cpp
auto left = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
}, pluss::ScalarType::Int64);
auto right = pluss::make_series({
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, left.unionize_along(right, 0, false));
```

### Output

```text
Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=
⎡ 1 2 3 ⎤
⎢ 4 5 6 ⎥
⎣ 7 8 9 ⎦
```

Example 2. `dim = 1` changes the compared slices from rows to columns.

```cpp
std::cout << left.unionize_along(
    pluss::make_series({
        {2, 7, 8},
        {5, 9, 10},
    }, pluss::ScalarType::Int64),
    1,
    false);
```

### Output

```text
pluss::Series(shape=[2,5], strides=[1,2], dtype=int64, platform=cpu, numel=10, contiguous=false)
data=[[1, 2, 3, 7, 8],
  [4, 5, 6, 9, 10]]
```

Example 3. Negative dims count from the back on higher-rank input too.

```cpp
auto nd_left =
    pluss::make_series({1, 2, 3, 4, 5, 6}, pluss::ScalarType::Int64).reshape({1, 2, 3});
auto nd_right =
    pluss::make_series({3, 2, 7, 4, 5, 6}, pluss::ScalarType::Int64).reshape({1, 2, 3});

std::cout << nd_left.unionize_along(nd_right, -1, false);
```

### Output

```text
pluss::Series(shape=[1,2,5], strides=[1,1,2], dtype=int64, platform=cpu, numel=10, contiguous=false)
data=[[[1, 2, 3, 3, 7],
  [4, 5, 6, 4, 6]]]
```

Example 4. `sorted=true` sorts the surviving slices rather than preserving
encounter order.

```cpp
std::cout << pluss::make_series({
    {4, 5, 6},
    {1, 2, 3},
}, pluss::ScalarType::Int64).unionize_along(
    pluss::make_series({
        {7, 8, 9},
        {4, 5, 6},
    }, pluss::ScalarType::Int64),
    0,
    true);
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=[[1, 2, 3],
  [4, 5, 6],
  [7, 8, 9]]
```

Example 5. Non-target dimensions must match exactly.

```cpp
try {
  std::cout << left.unionize_along(
      pluss::make_series({
          {1, 2},
          {3, 4},
      }, pluss::ScalarType::Int64),
      0,
      false) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
unionize_along: non-target dimensions must match
```

Example 6. An out-of-range axis fails immediately.

```cpp
try {
  std::cout << left.unionize_along(right, 3, false) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
unionize_along: dim out of range
```

## Gotchas And Failure Modes

- This is slice-wise set logic, not value-wise flatten-then-union logic.
- Changing `dim` changes what one comparable item is.
- `sorted=true` sorts slices, not scalar positions inside each slice.
- Non-target dimensions must match exactly.
- Rank, dtype, and platform must also match.

## Summary

Use `unionize_along()` when distinctness should apply to rows, columns, or
higher-rank slices.

- `dim` chooses the slice unit
- `sorted=false` preserves source order
- `sorted=true` sorts the surviving slices
- compatibility is strict outside the target axis

## See Also

- [`Series::unionize`](unionize.md): flat many-input union by value.
- [`Series::intersection_along`](intersection_along.md): keep only shared
  slices.
- [`Series::setxor_along`](setxor_along.md): keep only slices exclusive to one
  side.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unionize.html">
    <span class="label">Previous</span>
    <span class="title">unionize</span>
  </a>
  <a class="next-link" href="uniform_.html">
    <span class="label">Next</span>
    <span class="title">uniform_</span>
  </a>
</div>
