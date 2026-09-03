<!-- pluss-hand-authored -->

# SeriesStatAccessor::kurtosis

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::kurtosis() const
Series SeriesStatAccessor::kurtosis(std::int64_t dim) const
```

## What It Does

`kurtosis()` reports **excess kurtosis**, a standardized fourth-moment summary.
The normal-distribution reference is therefore `0`, not Pearson kurtosis `3`.
Positive values indicate heavier-tail or sharper-peak behavior relative to that
reference; negative values indicate flatter or lighter-tail behavior.

## Examples

```cpp
auto symmetric = pluss::make_series({1, 2, 3, 4, 5, 6}, {2, 3});
auto lopsided = pluss::make_series({1, 1, 4, 2, 2, 8}, {2, 3});
std::cout << symmetric.stat.kurtosis(1) << "\n";
std::cout << lopsided.stat().kurtosis(1) << "\n";
std::cout << lopsided.stat.kurtosis() << "\n";
```

### Output

```text
data=[-1.5, -1.5]
data=[-1.5, -1.5]
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
