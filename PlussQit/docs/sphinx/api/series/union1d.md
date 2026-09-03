<!-- pluss-hand-authored -->

# Series::union1d

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::union1d(const Series& other) const -> Series
Series::union1d(const Series& other, bool sorted) const -> Series
```

## What It Does

`union1d()` builds the distinct union of two inputs.

This is the simplest set-composition method on `Series`. It takes `self`, takes
`other`, flattens both to one dimension, joins them, and then removes repeated
values.

The main toggle is `sorted`:

- `true`: return the distinct union in value order
- `false`: keep the first time each distinct value is encountered while scanning
  `self` first and `other` second

That means `union1d()` can behave either like a classic set union or like a
stable "collect everything once" pass, depending on the ordering you need.

## Overload Map

**`union1d(other)`**

Equivalent to `union1d(other, true)`.

**`union1d(other, sorted)`**

Choose whether the union is value-sorted or first-seen.

## Parameters

**other** (`const Series&`)

The second input to merge with `self`.

Current root-method behavior is 1D-set-oriented:

- both inputs are flattened before the union is computed
- both inputs must have the same dtype
- both inputs must live on the same platform

**sorted** (`bool`)

Controls the output order.

- `true`: sort the final distinct union by value
- `false`: preserve first encounter order across the left-to-right scan

This toggle does not change membership. It changes only ordering.

## Returns

`Series`
    A flat 1D series containing one copy of every distinct value that appears
    in either input.

## Gradient

`union1d()` is not a differentiable numeric transform.

It is a set-like equality-and-order operation, so it does not define a smooth
backward path.

## Examples

Example 1. The zero-extra-argument form returns the sorted distinct union.

```cpp
auto left = pluss::make_series({3, 1, 3, 2}, pluss::ScalarType::Int64);
auto right = pluss::make_series({2, 4, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, left.union1d(right));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[1, 2, 3, 4]
```

Example 2. `sorted=false` keeps first encounter order instead.

```cpp
std::cout << left.union1d(right, false);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[3, 1, 2, 4]
```

The first `3` from `left` survives, then `1`, then `2`, and finally `4` from
`right`.

Example 3. Higher-rank inputs are flattened before the union is computed.

```cpp
std::cout << pluss::make_series({
    {1, 2},
    {2, 3},
}, pluss::ScalarType::Int64).union1d(
    pluss::make_series({
        {3, 4},
        {1, 5},
    }, pluss::ScalarType::Int64),
    false);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[1, 2, 3, 4, 5]
```

So this root method is about value membership, not about preserving rows,
columns, or slice structure.

Example 4. An empty side is fine; the result is the distinct content of the
non-empty side.

```cpp
std::cout << pluss::series::zeros({0}, pluss::ScalarType::Int64).union1d(
    pluss::make_series({2, 2, 3}, pluss::ScalarType::Int64),
    false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[2, 3]
```

Example 5. Dtype mismatch is rejected immediately.

```cpp
try {
  std::cout << pluss::make_series({1.0, 2.0}).union1d(right, false) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
union1d: input dtypes must match
```

## Gotchas And Failure Modes

- `union1d()` always returns a flat 1D result at this root surface.
- `sorted=false` is stable first-seen order, not "keep the left side exactly as
  written and append every unseen right value blindly." Duplicates are still
  collapsed.
- Dtype mismatch is a hard error.
- Platform mismatch is also a hard error through the shared set-op checks.

## Summary

Use `union1d()` when you want one copy of every value seen across two inputs.

- choose `union1d(other)` or `union1d(other, true)` for a sorted set union
- choose `union1d(other, false)` when encounter order matters
- expect flattening before the union is computed
- expect exactly one copy of each distinct surviving value

## See Also

- [`Series::unionize`](unionize.md): same idea across many inputs.
- [`Series::intersect1d`](intersect1d.md): keep only shared values.
- [`Series::setxor1d`](setxor1d.md): keep only values exclusive to one side.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unfold.html">
    <span class="label">Previous</span>
    <span class="title">unfold</span>
  </a>
  <a class="next-link" href="unionize.html">
    <span class="label">Next</span>
    <span class="title">unionize</span>
  </a>
</div>
