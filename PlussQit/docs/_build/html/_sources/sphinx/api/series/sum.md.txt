<!-- pluss-hand-authored -->

# Series::sum

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::sum() const -> Series
Series::sum(std::int64_t dim) const -> Series
Series::sum(std::int64_t dim, bool keepdim) const -> Series
```

## What It Does

Adds values together, either across the whole series or along one chosen axis.

`sum()` is the most direct reduction in the `Series` surface, but there are
three details that matter in practice:

- full reduction returns a scalar `Series`
- `sum(dim, keepdim)` removes or preserves the reduced axis depending on
  `keepdim`
- boolean `Series` do not stay boolean under summation; they become counts of
  `true` values

If you want a running prefix total rather than a dimension-collapsing
reduction, the right method is [`cumsum()`](cumsum.md), not `sum()`.

## Parameters

**dim** (`std::int64_t`)

The axis to reduce.

- positive dims count from the front
- negative dims count from the back
- the dim must refer to an existing axis

**keepdim** (`bool`)

Whether the reduced axis stays in the result as size `1`.

- `false`: the axis disappears
- `true`: the axis remains, which is often useful when the result will be
  broadcast back against the original shape

## Returns

`Series`
    A reduced series. Full reduction returns a scalar. Dim reduction returns a
    series whose shape depends on `keepdim`.

## Gradient

`sum()` is differentiable.

The backward rule is the simplest one in the library:

- every element that participated in the sum receives derivative `1`
- with `sum(dim, ...)`, that same incoming derivative is broadcast back across
  the reduced lane

There is no weighting and no tie behavior to think about. `sum()` just routes
the upstream sensitivity equally to every contributing element.

## Examples

The examples on this page build incrementally.

Example 1. Full reduction collapses the entire series to a scalar.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << mat.sum();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=21
```

The result is still a `Series`, but it is a 0-D scalar `Series`, not a raw
`double`.

Example 2. Reducing along `dim = 1` sums row-by-row.

```cpp
std::cout << mat.sum(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 15]
```

Each row becomes one output value because axis `1` is the column axis in this
`2 x 3` example.

Example 3. `keepdim=true` preserves the reduced axis as size `1`.

```cpp
std::cout << mat.sum(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[6],
  [15]]
```

This is often the better shape when the reduction is an intermediate step in a
larger broadcasted expression.

Example 4. Negative dims are normalized from the back.

```cpp
std::cout << mat.sum(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 15]
```

For a rank-2 series, `-1` means "the last axis," so this is the same reduction
as `sum(1)`.

Example 5. Boolean summation becomes a count of `true` values.

```cpp
auto flags = pluss::make_series({
    {true, false, true},
    {false, true, true},
});

std::cout << flags.sum();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=4
```

This is a very useful behavior to know. `sum()` over a boolean mask is the
native "how many positions matched?" reduction.

Example 6. The backward path broadcasts ones back to every contributing element.

```cpp
auto tracked_sum = mat.clone();
tracked_sum.tracks_slope_(true);

auto summed = tracked_sum.sum(1);
summed.sum().reverse();

std::cout << "y=" << summed << "\n";
std::cout << "dx=" << tracked_sum.sensitivity();
```

### Output

```text
y=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 15]
dx=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 1, 1],
  [1, 1, 1]]
```

Each row sum received upstream derivative `1`, so every element in each row
gets `1`.

Example 7. An out-of-range dim fails loudly.

```cpp
try {
  std::cout << mat.sum(2) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
reduce: dim out of range
```

## Notes

- `sum()` is a reduction, not a scan.
- `sum()` over booleans counts `true` values and returns `int64`.
- `keepdim=true` is mostly about downstream shape compatibility.

## See Also

- [`Series::mean`](mean.md): same shape rules, but divided by the lane length.
- [`Series::prod`](prod.md): same reduction shape ideas, but multiplicative.
- [`Series::cumsum`](cumsum.md): running totals without collapsing the axis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minimum.html">
    <span class="label">Previous</span>
    <span class="title">minimum</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
