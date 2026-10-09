<!-- pluss-hand-authored -->

# Series::StatAccessor::skewness

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::skewness() const
Series Series::StatAccessor::skewness(std::int64_t dim) const
Series SeriesStatProxy::skewness() const
Series SeriesStatProxy::skewness(std::int64_t dim) const
```

## What It Does

`skewness()` measures standardized third-moment asymmetry. Positive values
indicate a longer or heavier right tail, negative values a longer or heavier
left tail, and zero a symmetric lane. Zero does not mean low variance.

The statistic is dimensionless: translating a lane or multiplying it by a
positive scale does not change its skewness. It is a reduction, so global and
axis-wise calls describe different populations rather than transforming the
owner.

## Parameters And Returns

Omit `dim` to pool every value. Supply a valid, possibly negative `dim` to
return one skewness per surviving lane. The global result currently has shape
`[1]`.

## Examples

Example 1. Symmetric rows have zero third-moment asymmetry.

```cpp
auto symmetric = pluss::make_series({1, 2, 3, 4, 5, 6}, {2, 3});
std::cout << symmetric.stat.skewness(1) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
```

Example 2. Repeated low values and one high value create positive skewness.

```cpp
auto lopsided = pluss::make_series({1, 1, 4, 2, 2, 8}, {2, 3});
std::cout << lopsided.stat().skewness(1) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.70710678, 0.70710678]
```

Example 3. Pooling both rows creates a different distribution; global
skewness is not the average of the row statistics.

```cpp
std::cout << lopsided.stat.skewness() << "\n";
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1.2247449]
```

Pooling the rows changes the population being described, so global skewness is
not an average of row skewness. Here it is larger than either row result.

## Interpreting The Statistic

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
