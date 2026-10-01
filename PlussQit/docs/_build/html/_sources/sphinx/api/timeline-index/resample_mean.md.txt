# TimelineIndexAccessor::resample_mean

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signatures

```cpp
pluss::Series TimelineIndexAccessor::resample_mean(const pluss::Series& values, const ResampleOptions& options) const
pluss::Series TimelineIndexAccessor::resample_mean(const pluss::Series& values, std::string_view freq, std::int64_t step = 1) const
```

## What It Does

Aggregates a 1D numeric `Series` by averaging values inside each resample bin.

Use this when you want interval-level means rather than totals.

This is the level-style counterpart to [`resample_sum`](resample_sum.md).
Where a sum asks "how much accumulated?", a mean asks "what was the typical
level inside that interval?"

## Parameters

**values**

The 1D numeric payload aligned with the time index.

**freq**

The resampling frequency code.

**step**

The number of base frequency units per bin.

**options**

The full resampling control block.

## Returns

`pluss::Series`
    A 1D numeric `Series` of per-bin means.

## Examples

Example 1. Compute the mean value inside each hourly bucket.

```cpp
auto values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0});
auto means = idx.resample_mean(values, "H");
std::cout << pluss::series::print(means) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2.0000, 4.5000]
```

Example 2. Increase `step` when you want wider bins while keeping the same
mean-style aggregation.

```cpp
auto means = idx.resample_mean(values, "H", 2);
std::cout << pluss::series::print(means) << "\n";
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[3.0000]
```

## Notes

- `resample_mean()` and [`resample_sum`](resample_sum.md) share the same binning
  semantics. Only the aggregation rule changes.
- Empty bins become floating `NaN` in the current implementation once the mean
  surface is formed.
- If you need a dispatch-by-name surface, use [`resample_agg`](resample_agg.md).

## See Also

- [`TimelineIndexAccessor::resample_sum`](resample_sum.md)
- [`TimelineIndexAccessor::resample_agg`](resample_agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="resample_max.html">
    <span class="label">Previous</span>
    <span class="title">resample_max</span>
  </a>
  <a class="next-link" href="resample_min.html">
    <span class="label">Next</span>
    <span class="title">resample_min</span>
  </a>
</div>
