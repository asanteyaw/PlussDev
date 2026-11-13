<!-- pluss-hand-authored -->

# Series::setdiff1d

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::setdiff1d(const Series& other) const
Series Series::setdiff1d(const Series& other, bool sorted) const
Series Series::setdiff1d(const Series& other, bool sorted, bool assume_unique) const
```

## What It Does

`setdiff1d()` keeps the values that appear in `self` but do not appear in
`other`.

This is the left-exclusive companion to [`intersect1d()`](intersect1d.md). As
with the rest of the root set family, both inputs are flattened first and the
result is 1D.

The toggles mean:

- `sorted`: choose value order or left encounter order
- `assume_unique`: decide whether the method should pre-deduplicate each input

If you falsely set `assume_unique=true`, duplicate left-only values can remain
in the output.

## Parameters

**other** (`const Series&`)

The values to subtract from `self`.

**sorted** (`bool`)

- `true`: sort the result
- `false`: preserve left encounter order

**assume_unique** (`bool`)

- `false`: deduplicate both inputs first
- `true`: trust them as already unique

This toggle affects both cost and duplicate behavior.

## Returns

`Series`
    A flat 1D series containing the values that survive on the left side after
    removing anything present in `other`.

## Gradient

`setdiff1d()` is not differentiable.

It is a membership-based set filter.

## Examples

Example 1. The short form returns the sorted left-exclusive set.

```cpp
auto left = pluss::make_series({3, 1, 3, 2}, pluss::ScalarType::Int64);
auto right = pluss::make_series({2, 4, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, left.setdiff1d(right));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[3]
```

Example 2. `sorted=false` preserves the left input's encounter order.

```cpp
std::cout << pluss::make_series({5, 2, 5, 1, 3}, pluss::ScalarType::Int64).setdiff1d(
    pluss::make_series({2, 1}, pluss::ScalarType::Int64),
    false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[5, 3]
```

Example 3. `assume_unique=true` can preserve duplicates when the left input is
not actually unique.

```cpp
std::cout << pluss::make_series({5, 5, 3, 2}, pluss::ScalarType::Int64).setdiff1d(
    pluss::make_series({2}, pluss::ScalarType::Int64),
    false,
    true);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[5, 5, 3]
```

If you need strict one-copy-per-value set semantics, do not enable that toggle
casually.

Example 4. An empty right side means nothing is removed, but normal distinct
handling still applies.

```cpp
std::cout << pluss::make_series({2, 2, 3}, pluss::ScalarType::Int64).setdiff1d(
    pluss::series::zeros({0}, pluss::ScalarType::Int64),
    false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[2, 3]
```

## Gotchas And Failure Modes

- The operation is asymmetric. `a.setdiff1d(b)` is not the same as
  `b.setdiff1d(a)`.
- `assume_unique=true` can preserve duplicate left-exclusive values.
- Empty `other` is valid.
- The root result is always flat and 1D.
- Dtype mismatch and platform mismatch fail.

## Summary

Use `setdiff1d()` for "keep what is only on the left."

- the short form gives the sorted left-exclusive set
- `sorted=false` preserves left encounter order
- `assume_unique=true` trades safety for speed and can keep duplicates
- this is value subtraction after flattening, not row-wise or slice-wise
  subtraction

## See Also

- [`Series::comp`](comp.md): subtract `self` from a universe instead of
  subtracting `other` from `self`.
- [`Series::setxor1d`](setxor1d.md): keep values exclusive to either side.
- [`Series::intersect1d`](intersect1d.md): keep shared values instead.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set.html">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
  <a class="next-link" href="setdiff_along.html">
    <span class="label">Next</span>
    <span class="title">setdiff_along</span>
  </a>
</div>
