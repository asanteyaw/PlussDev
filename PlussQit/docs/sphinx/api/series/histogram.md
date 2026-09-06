<!-- pluss-hand-authored -->

# Series::histogram

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
std::pair<Series, Series> Series::histogram(std::int64_t bins = 100,
                                            bool density = false) const
std::pair<Series, Series> Series::histogram(std::int64_t bins,
                                            double lo, double hi,
                                            bool density = false) const
```

## What It Does

Divides a value range into `bins` equal-width buckets and counts how many
elements fall in each, returning **both** the counts and the bucket
boundaries.

The result is a `std::pair<Series, Series>`:

- **`.first`** is `hist` — a length-`bins` `float64` series. Bin `i` holds the
  number of elements in the half-open interval `[edges[i], edges[i + 1])`,
  except the final bin, whose upper edge is **inclusive** so the maximum value
  is not lost.
- **`.second`** is `bin_edges` — a length-`bins + 1` `float64` series, the
  evenly spaced boundaries.

Returning the edges is the whole reason this is a separate call from
[`histc()`](histc.md), which returns only the counts. You almost always need
the edges: to label an axis, to find a bin's centre, to interpret the shape.
Bind them together:

```cpp
auto [hist, edges] = data.histogram(20);
```

The range is either **given** (`histogram(bins, lo, hi)`) or **inferred** — the
default form spans the finite `[min, max]` of the data. Values outside the
range, and any `NaN` / `inf`, are simply not counted.

Set `density = true` to normalize: each count becomes
`count / (total * bin_width)`, so the bars integrate to `1`
(`sum(hist * bin_width) == 1`) and you have a probability density estimate
rather than raw tallies.

## Parameters

**bins** (`std::int64_t`): the number of equal-width buckets. Must be `> 0`.
Default `100`.

**lo**, **hi** (`double`): the inclusive lower and upper bounds of the range.
When omitted, the range is the finite minimum and maximum of the data. A
degenerate range (`hi <= lo`, which also happens when every value is equal) is
widened to `[lo - 0.5, hi + 0.5]`.

**density** (`bool`): when `true`, return a probability density instead of
counts. Default `false`.

## Returns

`std::pair<Series, Series>`
    `{hist, bin_edges}`. `hist` has length `bins`; `bin_edges` has length
    `bins + 1`. Both are `float64` and on the input's platform. `hist` is
    `float64` even for integer input and even without `density`.

## Gradient

`histogram()` is a bucketing operation with locally constant output. It is
**not differentiable**.

## Examples

Example 1. Seven values into four bins over `[0, 3]`.

```cpp
auto [hist, edges] =
    pluss::make_series({0.0, 1.0, 1.0, 2.0, 2.0, 2.0, 3.0}).histogram(4);

std::cout << "hist  = " << hist;
std::cout << "edges = " << edges;
```

### Output

```text
hist  = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 2, 3, 1]edges = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0.75, 1.5, 2.25, 3]
```

Bin `0` is `[0, 0.75)` and catches the single `0`. Bin `1` `[0.75, 1.5)` catches
the two `1`s. Bin `2` `[1.5, 2.25)` catches the three `2`s. Bin `3`
`[2.25, 3]` catches the `3` — the top edge is inclusive.

Example 2. The inclusive top edge, shown directly: `1.0` is the maximum and it
lands in the last bin rather than falling off the end.

```cpp
auto [hist, edges] = pluss::make_series({0.0, 0.5, 1.0}).histogram(2);

std::cout << "hist  = " << hist;
std::cout << "edges = " << edges;
```

### Output

```text
hist  = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]edges = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0.5, 1]
```

Bin `0` `[0, 0.5)` gets `{0}`; bin `1` `[0.5, 1]` gets `{0.5, 1}`.

Example 3. An explicit range clips: values below `lo` or above `hi` are
dropped, not clamped into the edge bins.

```cpp
auto [hist, edges] =
    pluss::make_series({-5.0, 0.0, 1.0, 2.0, 9.0}).histogram(2, 0.0, 2.0);

std::cout << "hist  = " << hist;
std::cout << "edges = " << edges;
```

### Output

```text
hist  = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]edges = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 2]
```

`-5` and `9` are outside `[0, 2]` and contribute nothing. Only `{0}` and
`{1, 2}` are counted.

Example 4. `density = true` rescales so the histogram integrates to `1`.

```cpp
auto [hist, edges] =
    pluss::make_series({0.0, 1.0, 1.0, 2.0, 2.0, 2.0, 3.0}).histogram(4, true);

std::cout << "hist = " << hist;

const double width = 3.0 / 4.0;  // (hi - lo) / bins
pluss::series::print(std::cout, hist.mul(width).sum());
```

### Output

```text
hist = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.19047619, 0.38095238, 0.57142857, 0.19047619]Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1.0000
```

Each bar is `count / (7 * 0.75)`, and `sum(hist * width)` is exactly `1`.

Example 5. `inf` and `NaN` fall outside every finite bin and are ignored.

```cpp
auto [hist, edges] = pluss::make_series({
    0.5, 1.5,
    std::numeric_limits<double>::infinity(),
    std::numeric_limits<double>::quiet_NaN(),
    2.5,
}).histogram(3, 0.0, 3.0);

std::cout << "hist  = " << hist;
std::cout << "edges = " << edges;
```

### Output

```text
hist  = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]edges = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0, 1, 2, 3]
```

Only `0.5`, `1.5`, and `2.5` are counted, one per bin.

Example 6. When every value is identical the inferred range is degenerate, so
it is widened by `0.5` on each side.

```cpp
auto [hist, edges] = pluss::make_series({4.0, 4.0, 4.0}).histogram(2);

std::cout << "hist  = " << hist;
std::cout << "edges = " << edges;
```

### Output

```text
hist  = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 3]edges = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3.5, 4, 4.5]
```

The range becomes `[3.5, 4.5]`; all three `4`s land in the upper bin `[4, 4.5]`.

Example 7. A multi-dimensional input is flattened before binning.

```cpp
auto [hist, edges] =
    pluss::make_series({{0.0, 1.0}, {2.0, 3.0}}).histogram(3, 0.0, 3.0);

std::cout << "hist  = " << hist;
std::cout << "edges = " << edges;
```

### Output

```text
hist  = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 2]edges = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0, 1, 2, 3]
```

Example 8. Contrast with [`histc()`](histc.md), which returns only the counts,
as `int64`, with no edges.

```cpp
std::cout << pluss::make_series({0.0, 1.0, 1.0, 2.0, 2.0, 2.0, 3.0})
                 .histc(4, 0.0, 3.0);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[1, 2, 3, 1]
```

## Gotchas And Failure Modes

- `hist` is `float64`, always — even for integer input and even without
  `density`. Cast it if you need integer counts, or use [`histc()`](histc.md).
- `bin_edges` has `bins + 1` entries, one more than `hist`.
- Values `< lo` or `> hi` are **dropped**, not clamped into the boundary bins.
- `NaN` and `inf` are never counted.
- The inferred range spans the finite data; a lone `inf` does not stretch it.
- `bins` must be `> 0`; `bins = 0` raises.
- Not differentiable.
- For a tally of repeated *integer labels* (not a value range), use
  [`bincount()`](bincount.md).

## Summary

`histogram()` buckets values into equal-width bins and returns both parts:

- `std::pair<Series, Series>` = `{hist (float64, length bins), bin_edges
  (float64, length bins + 1)}` — bind as `auto [h, e] = ...`
- range is given, or the finite `[min, max]`; out-of-range and non-finite
  values are dropped
- the top edge is inclusive
- `density = true` gives a probability density (`sum(hist * width) == 1`)
- not differentiable

## See Also

- [`Series::histc`](histc.md): the same binning, counts only, `int64`, no
  edges.
- [`Series::bincount`](bincount.md): tally non-negative integer *values*
  directly, one bin per integer.
- [`Series::aminmax`](aminmax.md): the `[min, max]` the default range comes
  from.
- [`Series::quantile`](quantile.md): summarize a distribution by percentile
  instead of by bin.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="histc.html">
    <span class="label">Previous</span>
    <span class="title">histc</span>
  </a>
  <a class="next-link" href="hsplit.html">
    <span class="label">Next</span>
    <span class="title">hsplit</span>
  </a>
</div>
