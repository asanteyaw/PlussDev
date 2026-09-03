<!-- pluss-hand-authored -->

# SeriesStatAccessor::skewness

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::skewness() const
Series SeriesStatAccessor::skewness(std::int64_t dim) const
```

## What It Does

`skewness()` measures standardized third-moment asymmetry. Positive values
indicate a longer or heavier right tail, negative values a longer or heavier
left tail, and zero a symmetric lane. Zero does not mean low variance.

## Examples

```cpp
auto symmetric = pluss::make_series({1, 2, 3, 4, 5, 6}, {2, 3});
auto lopsided = pluss::make_series({1, 1, 4, 2, 2, 8}, {2, 3});
std::cout << symmetric.stat.skewness(1) << "\n";
std::cout << lopsided.stat().skewness(1) << "\n";
std::cout << lopsided.stat.skewness() << "\n";
```

### Output

```text
data=[0, 0]
data=[0.70710678, 0.70710678]
data=[1.2247449]
```

Pooling the rows changes the population being described, so global skewness is
not an average of row skewness. Here it is larger than either row result.

## Reading The Statistic

Skewness is dimensionless because deviations are normalized by standard
deviation. Multiplying every value by a positive constant therefore changes
spread but not skewness. Treat estimates from very short lanes cautiously;
higher moments are sensitive to a few extreme observations.

## See Also

- [`Series::skewness`](../series/skewness.md): expanded interpretation examples.
- [`kurtosis`](kurtosis.md): fourth-moment shape, not asymmetry.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pdist2.html"><span class="label">Previous</span><span class="title">pdist2</span></a>
  <a class="next-link" href="standardize.html"><span class="label">Next</span><span class="title">standardize</span></a>
</div>
