<!-- pluss-hand-authored -->

# Series::skewness

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference)

## Signature

```cpp
Series::stat.skewness() -> Series
Series::stat.skewness(std::int64_t dim) -> Series
Series::stat().skewness() -> Series
Series::stat().skewness(std::int64_t dim) -> Series
```

## What It Does

`skewness()` measures asymmetry.

At a high level:

- positive skewness means a heavier or longer right tail
- negative skewness means a heavier or longer left tail
- zero skewness means the lane is symmetric around its center

Unlike [`center`](center.md) and [`standardize`](standardize.md), this is a
reduction again: one summary value per reduced lane.

## Surface Note

On the current public surface documented here, use the stats accessor spelling:

- `x.stat.skewness(...)`
- `x.stat().skewness(...)`

That is the verified callable path used by the compiled examples below.

## Parameters

**dim** (`std::int64_t`)

The reduced axis.

Important details:

- omit `dim` to summarize the entire `Series`
- pass `dim` to compute one skewness value per remaining lane
- negative dims count from the back

## Returns

`Series`
    A reduction result containing one skewness value per surviving lane, or one
    value for the entire `Series` when no `dim` is provided.

## Gradient

This is primarily a descriptive higher-moment statistic. It is more useful for
diagnostics and distribution understanding than for ordinary model plumbing.

## Examples

The examples below move from symmetric lanes to lopsided ones, then compare
lane-wise and global readings.

Example 1. Symmetric rows have zero skewness.

```cpp
auto values = pluss::make_series(
    {1, 2, 3, 4, 5, 6}, {2, 3});

std::cout << values.stat.skewness(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
```

That is the expected result because `[1, 2, 3]` and `[4, 5, 6]` are symmetric
around their row means.

Example 2. The explicit proxy spelling `stat().skewness(dim)` reaches the same
row-wise computation.

```cpp
std::cout << values.stat().skewness(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
```

That is the same statistic as Example 1, reached through the more explicit
accessor call form.

Example 3. Lopsided rows produce positive skewness.

```cpp
auto lopsided = pluss::make_series(
    {1, 1, 4, 2, 2, 8}, {2, 3});

std::cout << lopsided.stat.skewness(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.70710678, 0.70710678]
```

This is the page's main practical lesson:

- skewness is about shape, not just spread
- two lanes can have similar variance but different asymmetry

Example 4. Global skewness can differ materially from per-row skewness because
pooling changes the distribution you are summarizing.

```cpp
std::cout << lopsided.stat.skewness();
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1.2247449]
```

Here the pooled distribution is more asymmetric than either individual row, so
the global skewness is larger.

Example 5. Negative dimensions count from the back, so `-1` means the last
axis.

```cpp
std::cout << lopsided.stat.skewness(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.70710678, 0.70710678]
```

On this 2D shape, the last axis is the row axis, so the result matches Example
3.

## Gotchas And Notes

- Use the `stat` accessor path on this surface.
- Zero skewness means symmetry, not "low variance."
- Positive skewness does not tell you the spread by itself; it tells you the
  direction of imbalance.
- Very short lanes can still produce a skewness value, but you should interpret
  it carefully because a small number of points makes higher-moment summaries
  fragile.
- A `dim` outside the valid axis range fails.

## Summary

Use `skewness()` when you want to summarize distribution shape rather than
transform values:

- zero means symmetric around the center
- positive means the right side is heavier or longer
- negative means the left side is heavier or longer
- global and lane-wise skewness can differ a lot because they summarize
  different distributions

## See Also

- [`Series::kurtosis`](kurtosis.md): fourth-moment tail/peakedness summary.
- [`Series::var`](var.md): second-moment spread summary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="standardize.html"><span class="label">Previous</span><span class="title">standardize</span></a>
  <a class="next-link" href="kurtosis.html"><span class="label">Next</span><span class="title">kurtosis</span></a>
</div>
