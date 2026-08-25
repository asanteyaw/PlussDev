<!-- pluss-hand-authored -->

# Series::kurtosis

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference)

## Signature

```cpp
Series::stat.kurtosis() -> Series
Series::stat.kurtosis(std::int64_t dim) -> Series
Series::stat().kurtosis() -> Series
Series::stat().kurtosis(std::int64_t dim) -> Series
```

## What It Does

`kurtosis()` measures fourth-moment shape: how heavy-tailed or sharply peaked a
distribution is relative to its spread.

This implementation reports **excess kurtosis**, not raw Pearson kurtosis.

That distinction matters immediately:

- a normal-like reference point is `0`
- not `3`

## Surface Note

Like [`skewness`](skewness.md), the verified callable path on this surface is
through the stats accessor:

- `x.stat.kurtosis(...)`
- `x.stat().kurtosis(...)`

## Parameters

**dim** (`std::int64_t`)

The reduced axis.

Important details:

- omit `dim` to summarize the entire `Series`
- pass `dim` to compute one kurtosis value per surviving lane
- negative dims count from the back

## Returns

`Series`
    A reduction result containing one excess-kurtosis value per surviving lane,
    or one value for the entire `Series` when no `dim` is provided.

## Gradient

Like [`skewness`](skewness.md), this is primarily a descriptive statistic used
to understand distribution shape.

## Verified Examples

The examples below are arranged to teach the excess-kurtosis convention first,
then show that asymmetry and kurtosis are different questions.

Example 1. Symmetric three-point rows produce negative excess kurtosis.

```cpp
auto values = pluss::make_series(
    {1, 2, 3, 4, 5, 6}, {2, 3});

std::cout << values.stat.kurtosis(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1.5, -1.5]
```

This is the clearest "excess kurtosis" tell on the page. If you expected a
normal reference around `3`, you were thinking of the raw Pearson form.

Example 2. The explicit proxy spelling `stat().kurtosis(dim)` reaches the same
row-wise computation.

```cpp
std::cout << values.stat().kurtosis(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1.5, -1.5]
```

This is the same statistic as Example 1, reached through the more explicit
accessor spelling.

Example 3. Lopsided rows can share the same kurtosis as symmetric rows.

```cpp
auto lopsided = pluss::make_series(
    {1, 1, 4, 2, 2, 8}, {2, 3});

std::cout << lopsided.stat.kurtosis(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1.5, -1.5]
```

That is an important teaching point:

- kurtosis is not skewness
- visibly lopsided lanes can still have the same excess-kurtosis summary

Example 4. Global kurtosis can differ materially from per-row kurtosis once the
distribution is pooled.

```cpp
std::cout << lopsided.stat.kurtosis();
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.055555556]
```

The pooled distribution is only slightly above the normal-like zero reference
here, which is very different from the row-wise `-1.5` result.

Example 5. Negative dimensions count from the back, so `-1` means the last
axis.

```cpp
std::cout << lopsided.stat.kurtosis(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1.5, -1.5]
```

On this 2D shape, the last axis is the row axis, so the result matches Example
3.

## Gotchas And Teaching Notes

- This surface reports excess kurtosis.
- Use the stats accessor path on the current public surface.
- Kurtosis and skewness summarize different shape properties and should not be
  mentally collapsed into one "weirdness" metric.
- Very short lanes can still produce a kurtosis value, but higher-moment
  statistics are sensitive and should be interpreted with care on small samples.
- A `dim` outside the valid axis range fails.

## Teaching Summary

Use `kurtosis()` when you care about tail/peakedness shape rather than center or
spread:

- excess kurtosis `0` is the normal-like reference point on this surface
- negative values mean flatter/lighter-tailed behavior relative to that
  reference
- positive values mean heavier-tailed or sharper-peaked behavior relative to
  that reference
- lane-wise and global kurtosis can disagree because they summarize different
  pooled data

## See Also

- [`Series::skewness`](skewness.md): asymmetry rather than tail/peakedness shape.
- [`Series::standardize`](standardize.md): transform the values instead of
  summarizing their fourth moment.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="skewness.html"><span class="label">Previous</span><span class="title">skewness</span></a>
  <a class="next-link" href="../../series_api_reference.html"><span class="label">Next</span><span class="title">Series API Reference</span></a>
</div>
