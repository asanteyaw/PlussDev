<!-- pluss-hand-authored -->

# Series::intersect1d

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::intersect1d(const Series& other) const
Series Series::intersect1d(const Series& other, bool sorted) const
Series Series::intersect1d(const Series& other, bool sorted, bool assume_unique) const
```

## What It Does

`intersect1d()` keeps the values that appear in both inputs.

Like the other root set methods, it flattens both inputs first and produces a
flat 1D result. The two toggles on this method matter for different reasons:

- `sorted` controls output order
- `assume_unique` controls whether the method first deduplicates the inputs

That second toggle is the load-bearing quirk on this page. If
`assume_unique=false`, the method behaves like ordinary set intersection. If
`assume_unique=true`, the method trusts you and skips the pre-deduplication
step, which means duplicates from `self` can survive into the result.

## Parameters

**other** (`const Series&`)

The second input whose membership is tested against `self`.

**sorted** (`bool`)

Controls output ordering.

- `true`: sorted intersection
- `false`: preserve the left input's first-seen order after any needed
  deduplication

**assume_unique** (`bool`)

Tells the method whether it should trust both inputs to already contain no
repeated values.

- `false`: deduplicate both sides first
- `true`: skip that deduplication work

This toggle can affect both performance and semantics. If the inputs are not
actually unique and you still pass `true`, duplicate survivors can remain.

## Returns

`Series`
    A flat 1D series containing the values shared by both inputs.

## Gradient

`intersect1d()` is not differentiable.

It is a membership-selection operation, not a smooth numeric map.

## Examples

Example 1. The short form returns the sorted shared set.

```cpp
auto left = pluss::make_series({3, 1, 3, 2}, pluss::ScalarType::Int64);
auto right = pluss::make_series({2, 4, 1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, left.intersect1d(right));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
```

Example 2. `sorted=false` preserves the left side's encounter order.

```cpp
std::cout << pluss::make_series({5, 2, 5, 1, 2}, pluss::ScalarType::Int64).intersect1d(
    pluss::make_series({2, 1}, pluss::ScalarType::Int64),
    false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[2, 1]
```

The surviving order comes from the left input after stable deduplication.

Example 3. `assume_unique=true` skips the deduplication pass and can therefore
preserve duplicates.

```cpp
std::cout << pluss::make_series({2, 2, 1}, pluss::ScalarType::Int64).intersect1d(
    pluss::make_series({1, 2}, pluss::ScalarType::Int64),
    false,
    true);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 1]
```

That is not ordinary mathematical set behavior. It is the direct consequence of
telling the method to trust uniqueness that is not actually present.

Example 4. No overlap simply returns an empty 1D result.

```cpp
std::cout << pluss::make_series({8, 9}, pluss::ScalarType::Int64).intersect1d(
    pluss::make_series({1, 2}, pluss::ScalarType::Int64),
    false);
```

### Output

```text
pluss::Series(shape=[0], strides=[1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

## Gotchas And Failure Modes

- `assume_unique=true` is a trust-me toggle, not just a speed hint.
- If you need strict set semantics, keep `assume_unique=false`.
- The result is always flat and 1D at this root surface.
- Dtype mismatch and platform mismatch are hard errors.

## Summary

Use `intersect1d()` when you want the overlap between two value collections.

- `intersect1d(other)` gives the sorted shared set
- `sorted=false` preserves left-side encounter order
- `assume_unique=true` can keep duplicates if your inputs were not really unique
- the method is about flattened value membership, not shape-preserving slices

## See Also

- [`Series::intersection`](intersection.md): generalize the same idea to many
  inputs.
- [`Series::setdiff1d`](setdiff1d.md): keep only left-exclusive values.
- [`Series::isin`](isin.md): test membership without collapsing to a distinct
  output set.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="inner.html">
    <span class="label">Previous</span>
    <span class="title">inner</span>
  </a>
  <a class="next-link" href="intersection.html">
    <span class="label">Next</span>
    <span class="title">intersection</span>
  </a>
</div>
