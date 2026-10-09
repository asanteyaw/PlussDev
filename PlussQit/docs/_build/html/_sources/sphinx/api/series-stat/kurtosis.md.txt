<!-- pluss-hand-authored -->

# Series::StatAccessor::kurtosis

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::kurtosis() const
Series Series::StatAccessor::kurtosis(std::int64_t dim) const
Series SeriesStatProxy::kurtosis() const
Series SeriesStatProxy::kurtosis(std::int64_t dim) const
```

## What It Does

`kurtosis()` reports **excess kurtosis**, a standardized fourth-moment summary.
The normal-distribution reference is therefore `0`, not Pearson kurtosis `3`.
Positive values indicate heavier-tail or sharper-peak behavior relative to that
reference; negative values indicate flatter or lighter-tail behavior.

This is a reduction and a shape descriptor, not a spread measurement. Two
lanes can have different scales or asymmetries and still share the same
kurtosis.

## Parameters And Returns

Omitting `dim` pools the complete owner. Supplying `dim`, including a negative
dimension, computes one value per surviving lane. The global result currently
has shape `[1]`.

## Examples

Example 1. Symmetric three-point rows produce negative excess kurtosis.

```cpp
auto symmetric = pluss::make_series({1, 2, 3, 4, 5, 6}, {2, 3});
std::cout << symmetric.stat.kurtosis(1) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1.5, -1.5]
```

Example 2. These lopsided rows have the same fourth-moment summary despite
their nonzero skewness.

```cpp
auto lopsided = pluss::make_series({1, 1, 4, 2, 2, 8}, {2, 3});
std::cout << lopsided.stat().kurtosis(1) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1.5, -1.5]
```

Example 3. Pooling changes the population and therefore the fourth moment.

```cpp
std::cout << lopsided.stat.kurtosis() << "\n";
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.055555556]
```

The symmetric and lopsided rows share kurtosis even though their skewness
differs. Kurtosis and skewness are separate questions, not interchangeable
measures of distribution irregularity. Pooling also changes the result.

## Interpretation Limits

Kurtosis is highly sensitive to rare extremes and unstable on short lanes. Use
it as descriptive evidence alongside counts, quantiles, and skewness rather
than as a complete statement about distribution shape.

## See Also

- [`Series::kurtosis`](../series/kurtosis.md): expanded root-surface lesson.
- [`skewness`](skewness.md): standardized asymmetry.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="center.html"><span class="label">Previous</span><span class="title">center</span></a>
  <a class="next-link" href="logcdf.html"><span class="label">Next</span><span class="title">logcdf</span></a>
</div>
