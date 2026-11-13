<!-- pluss-hand-authored -->

# Series::setxor1d

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::setxor1d(const Series& other) const
Series Series::setxor1d(const Series& other, bool sorted) const
Series Series::setxor1d(const Series& other, bool sorted, bool assume_unique) const
```

## What It Does

`setxor1d()` computes the symmetric difference between two inputs.

In plain language: it keeps the values that belong to exactly one side, not to
both.

This method is built from the same set machinery as `setdiff1d()` and
`union1d()`, so the same themes matter:

- flatten first
- compare by value
- choose ordering with `sorted`
- optionally trust uniqueness with `assume_unique`

## Parameters

**other** (`const Series&`)

The second input to compare against `self`.

**sorted** (`bool`)

- `true`: sort the final symmetric difference
- `false`: keep the left-exclusive survivors first, then the right-exclusive
  survivors, each in their preserved encounter order

**assume_unique** (`bool`)

- `false`: deduplicate both inputs before the symmetric-difference logic
- `true`: skip that deduplication

As with the other set methods, an incorrect `true` can allow duplicates to
survive.

## Returns

`Series`
    A flat 1D series containing the values that appear in exactly one of the
    two inputs.

## Gradient

`setxor1d()` is not differentiable.

It is a set-membership composition.

## Examples

Example 1. The short form returns the sorted symmetric difference.

```cpp
auto left = pluss::make_series({3, 1, 3, 2}, pluss::ScalarType::Int64);
auto right = pluss::make_series({2, 4, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, left.setxor1d(right));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[3, 4]
```

Example 2. `sorted=false` preserves the side-by-side survivor order instead of
value sorting.

```cpp
std::cout << left.setxor1d(right, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[3, 4]
```

This example happens to match the sorted result numerically, but the semantics
are still different: left-only survivors come first, then right-only survivors.

Example 3. `assume_unique=true` can preserve duplicates on either exclusive
side.

```cpp
std::cout << pluss::make_series({5, 5, 2}, pluss::ScalarType::Int64).setxor1d(
    pluss::make_series({2, 7, 7}, pluss::ScalarType::Int64),
    false,
    true);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[5, 5, 7, 7]
```

Example 4. If one side is empty, the symmetric difference reduces to the
distinct content of the non-empty side.

```cpp
std::cout << pluss::series::zeros({0}, pluss::ScalarType::Int64).setxor1d(
    pluss::make_series({2, 2, 3}, pluss::ScalarType::Int64),
    false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[2, 3]
```

## Gotchas And Failure Modes

- `sorted=false` preserves side order, not a global merge order.
- `assume_unique=true` can preserve duplicates.
- The result is always flat and 1D at this root surface.
- Both inputs must share dtype and platform.

## Summary

Use `setxor1d()` when you want the values that belong to only one side.

- the short form returns a sorted symmetric difference
- `sorted=false` preserves the exclusive survivors in left-then-right order
- `assume_unique=true` is only safe when the inputs really are unique already
- flattening happens before the symmetric difference is computed

## See Also

- [`Series::setdiff1d`](setdiff1d.md): keep only one chosen side's exclusives.
- [`Series::union1d`](union1d.md): keep everything once.
- [`Series::intersect1d`](intersect1d.md): keep only the overlap.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="setdiff1d.html">
    <span class="label">Previous</span>
    <span class="title">setdiff1d</span>
  </a>
  <a class="next-link" href="setxor_along.html">
    <span class="label">Next</span>
    <span class="title">setxor_along</span>
  </a>
</div>
