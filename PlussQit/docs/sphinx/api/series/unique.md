<!-- pluss-hand-authored -->

# Series::unique

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::unique() const -> Series
Series::unique(bool sorted) const -> Series
```

## What It Does

`unique()` removes repeated values and keeps one representative of each distinct
value.

The main behavior toggle is `sorted`:

- `true`: return the distinct set in sorted order
- `false`: keep the first time each distinct value appears

That one toggle changes the mental model quite a bit. With `sorted=true`,
`unique()` behaves like set extraction. With `sorted=false`, it behaves more
like stable deduplication of a flattened scan.

## Overload Map

**`unique()`**

Equivalent to `unique(true)`.

**`unique(bool sorted)`**

Choose whether distinct values come back sorted or in first-seen order.

## Parameters

**sorted** (`bool`)

Controls output ordering.

- `true`: order by value
- `false`: preserve first-seen encounter order

This does not change which values survive. It changes only which survivor
order you get back.

## Returns

`Series`
    A 1D series containing one copy of each distinct value.

Current root-method behavior is flat: higher-rank input is flattened before the
distinct pass.

## Gradient

`unique()` is not a differentiable numeric transform.

It is a set-like/data-shaping operation whose result depends on equality and
ordering, not on a smooth numeric map.

## Examples

Example 1. The zero-argument form returns the sorted distinct set.

```cpp
auto duplicates = pluss::make_series({4, 1, 4, 2, 1, 3}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, duplicates.unique());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[1, 2, 3, 4]
```

Example 2. `sorted=false` switches from set ordering to first-seen ordering.

```cpp
std::cout << duplicates.unique(false);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[4, 1, 2, 3]
```

That is often the better choice when the original scan order carries meaning.

Example 3. Higher-rank input is flattened before distinct values are extracted.

```cpp
std::cout << pluss::make_series({
    {3, 3, 2},
    {1, 2, 1},
}, pluss::ScalarType::Int64).unique(false);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[3, 2, 1]
```

The surviving order comes from the flattened scan: `3`, then `2`, then `1`.

Example 4. Floating-point distinctness treats repeated `NaN` values as one
distinct value and collapses signed zero together.

```cpp
std::cout << pluss::make_series({
    1.0,
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN(),
    -0.0,
    0.0,
    1.0,
}).unique(false);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, nan, -0]
```

This is one of the most important quirks on the method: repeated `NaN` values
do not keep multiplying the distinct set.

## Gotchas And Failure Modes

- `unique()` currently returns a flat 1D result at this root surface.
- `sorted=false` is stable with respect to first appearance, not last
  appearance.
- Floating-point `NaN` values collapse together as one distinct value.
- `-0.0` and `0.0` are treated as the same distinct zero class.

## Summary

Use `unique()` for distinct extraction, then decide which ordering you need.

- choose `unique()` or `unique(true)` for a sorted distinct set
- choose `unique(false)` for stable first-seen deduplication
- expect a flat 1D result at this root method
- remember the special floating-point rules around `NaN` and zero

## See Also

- [`Series::isin`](isin.md): test membership against a distinct set.
- [`Series::bincount`](bincount.md): count integer occurrences instead of just
  removing duplicates.
- [`Series::histc`](histc.md): bucket numeric values into ranges rather than
  equality classes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transpose.html">
    <span class="label">Previous</span>
    <span class="title">transpose</span>
  </a>
  <a class="next-link" href="unite.html">
    <span class="label">Next</span>
    <span class="title">unite</span>
  </a>
</div>
