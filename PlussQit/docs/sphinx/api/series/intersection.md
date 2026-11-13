<!-- pluss-hand-authored -->

# Series::intersection

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::intersection(pluss::RangeView<Series> others,
                            bool sorted = true,
                            bool assume_unique = false) const
```

## What It Does

`intersection()` is the many-input counterpart to
[`intersect1d()`](intersect1d.md).

You provide `self` plus a list of additional inputs, and the method keeps only
the values that survive in every one of them.

The toggles have the same jobs as in `intersect1d()`, but they matter even more
here because repeated pairwise filtering compounds the effect:

- `sorted` decides whether final output is sorted or left-order-preserving
- `assume_unique` decides whether each input is pre-deduplicated before the
  repeated intersections begin

If `assume_unique=true` is wrong, duplicates can keep propagating through the
chain.

## Parameters

**others** (`pluss::RangeView<Series>`)

The additional inputs that every surviving value must appear in.

In everyday code, pass them as a brace list like `{b, c, d}`.

**sorted** (`bool`)

Controls final output order.

- `true`: sort the final distinct result
- `false`: preserve the encounter order inherited from `self`

**assume_unique** (`bool`)

Controls whether the method first deduplicates each participating input.

- `false`: enforce ordinary set-style uniqueness first
- `true`: trust the inputs and skip that cleanup

This is the main semantics-sensitive toggle on the method.

## Returns

`Series`
    A flat 1D series containing the values that appear in `self` and in every
    input from `others`.

## Gradient

`intersection()` is not differentiable.

It is a repeated set-membership filter, not a smooth transform.

## Examples

Example 1. Empty `others` means "intersection of just `self`," which reduces to
distinct extraction.

```cpp
auto base = pluss::make_series({3, 1, 3, 2}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, base.intersection({}, false, false));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[3, 1, 2]
```

Example 2. With several inputs, only values present everywhere survive.

```cpp
auto b = pluss::make_series({2, 4, 1}, pluss::ScalarType::Int64);
auto c = pluss::make_series({4, 2, 9}, pluss::ScalarType::Int64);

std::cout << base.intersection({b, c}, false, false);
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[2]
```

Example 3. `sorted=false` preserves the surviving order inherited from the left
input.

```cpp
std::cout << pluss::make_series({5, 2, 1, 3, 2}, pluss::ScalarType::Int64).intersection({
    pluss::make_series({2, 5, 9}, pluss::ScalarType::Int64),
    pluss::make_series({5, 2, 7}, pluss::ScalarType::Int64),
}, false, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[5, 2]
```

Example 4. `assume_unique=true` can preserve duplicates if you promise
uniqueness incorrectly.

```cpp
std::cout << pluss::make_series({2, 2, 1}, pluss::ScalarType::Int64).intersection({
    pluss::make_series({2, 1}, pluss::ScalarType::Int64),
    pluss::make_series({2}, pluss::ScalarType::Int64),
}, false, true);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[2, 2]
```

This example is the key warning on the method: `assume_unique=true` is allowed
to change the semantic shape of the result.

Example 5. The chain can collapse to empty as soon as one input eliminates all
survivors.

```cpp
std::cout << pluss::make_series({5, 2, 1}, pluss::ScalarType::Int64).intersection({
    pluss::make_series({2, 5}, pluss::ScalarType::Int64),
    pluss::make_series({7, 8}, pluss::ScalarType::Int64),
}, false, false);
```

### Output

```text
pluss::Series(shape=[0], strides=[1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

## Gotchas And Failure Modes

- Empty `others` is not an error.
- `sorted=false` preserves order inherited from `self`, not from the later
  inputs.
- `assume_unique=true` can preserve duplicates through the chain.
- All inputs must share dtype and platform.
- This root method always returns a flat 1D result.

## Summary

Use `intersection()` when the same value must survive across many series.

- pass additional inputs naturally as `{b, c, d}`
- keep `assume_unique=false` unless you truly know each input is already unique
- choose `sorted=false` when left-side encounter order is meaningful
- expect the output to shrink monotonically as more inputs are added

## See Also

- [`Series::intersect1d`](intersect1d.md): the two-input form.
- [`Series::unionize`](unionize.md): many-input distinct union.
- [`Series::setdiff1d`](setdiff1d.md): subtract membership instead of keeping it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersect1d.html">
    <span class="label">Previous</span>
    <span class="title">intersect1d</span>
  </a>
  <a class="next-link" href="intersection_along.html">
    <span class="label">Next</span>
    <span class="title">intersection_along</span>
  </a>
</div>
