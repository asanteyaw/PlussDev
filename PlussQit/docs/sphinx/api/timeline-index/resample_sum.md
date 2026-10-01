# TimelineIndexAccessor::resample_sum

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signatures

```cpp
pluss::Series TimelineIndexAccessor::resample_sum(const pluss::Series& values, const ResampleOptions& options) const
pluss::Series TimelineIndexAccessor::resample_sum(const pluss::Series& values, std::string_view freq, std::int64_t step = 1) const
```

## What It Does

Aggregates a 1D numeric `Series` by summing values inside each resample bin.

This is one of the most common time-series reductions: convert irregular or
fine-grained observations into coarser interval totals.

Use it for additive quantities: turnover, volume, cost, energy, event weights,
or any other payload where values should accumulate across time.

## Parameters

**values**

The 1D numeric values aligned case-for-case with the index.

Case `i` in `values` is assumed to belong to case `i` in the index. That
alignment is not inferred; it is required.

**freq**

The base resampling frequency.

**step**

The bin stride in units of `freq`.

**options**

The full resampling control block. Use this overload when boundary ownership,
label edge, origin anchoring, or offset shifts matter.

## Returns

`pluss::Series`
    A 1D numeric `Series` with one summed value per output bin.

## Examples

Example 1. Sum values inside each hourly bucket.

```cpp
auto values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0});
auto totals = idx.resample_sum(values, "H");
std::cout << pluss::series::print(totals) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6.0000, 9.0000]
```

Example 2. Use the options overload when the bin anchor or boundary semantics
must be explicit.

```cpp
pluss::TimelineIndexAccessor::ResampleOptions opts;
opts.freq = "H";
opts.step = 1;
opts.origin = pluss::TimelineIndexAccessor::ResampleOrigin::Epoch;

auto totals = idx.resample_sum(values, opts);
std::cout << pluss::series::print(totals) << "\n";
```

Example 3. Increase `step` when you want a coarser total from the same hourly
family.

```cpp
auto totals = idx.resample_sum(values, "H", 2);
std::cout << pluss::series::print(totals) << "\n";
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[15.0000]
```

## Notes

- `values` must be 1D and the same length as the index.
- This is usually the right reducer for flow-like quantities and usually the
  wrong reducer for point-in-time levels.
- If you only need observation counts, use [`resample_count`](resample_count.md).
- If you want operation-by-name dispatch, use [`resample_agg`](resample_agg.md).

## See Also

- [`TimelineIndexAccessor::resample_mean`](resample_mean.md)
- [`TimelineIndexAccessor::resample_agg`](resample_agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="resample_ohlc.html">
    <span class="label">Previous</span>
    <span class="title">resample_ohlc</span>
  </a>
  <a class="next-link" href="rolling_agg.html">
    <span class="label">Next</span>
    <span class="title">rolling_agg</span>
  </a>
</div>
