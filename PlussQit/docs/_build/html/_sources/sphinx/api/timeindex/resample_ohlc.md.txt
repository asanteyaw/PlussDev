# TimeIndex::resample_ohlc

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signatures

```cpp
TimeIndex::resample_ohlc(const pluss::Series& values, const ResampleOptions& options) const -> pluss::Series
TimeIndex::resample_ohlc(const pluss::Series& values, std::string_view freq, std::int64_t step = 1) const -> pluss::Series
```

## What It Does

Builds OHLC bars from a 1D numeric `Series`.

The returned `Series` is 2D with shape `[nbins, 4]`, ordered as:

```text
[open, high, low, close]
```

This is the natural resampling surface for price-like series.

Use it when each input value is a level observed at a time, and each output row
should summarize an interval as open, high, low, and close.

## Parameters

**values**

The 1D numeric values aligned with the time index.

**freq**

The output bar frequency.

**step**

How many `freq` units each bar spans.

**options**

The full resampling control block when you need explicit closure, labeling,
origin control, or anchor offset.

## Returns

`pluss::Series`
    A 2D numeric `Series` with one row per bar and four columns in OHLC order.

## Examples

Example 1. Build hourly OHLC bars from a 1D numeric stream.

```cpp
auto values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0});
auto ohlc = idx.resample_ohlc(values, "H");
std::cout << pluss::print_series(ohlc) << "\n";
```

### Output

```text
Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=
⎡ 1.0000 3.0000 1.0000 3.0000 ⎤
⎣ 4.0000 5.0000 4.0000 5.0000 ⎦
```

Example 2. Increase `step` when you want fewer, wider bars.

```cpp
auto ohlc = idx.resample_ohlc(values, "H", 2);
std::cout << pluss::print_series(ohlc) << "\n";
```

### Output

```text
Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
[ 1.0000 5.0000 1.0000 5.0000 ]
```

## Notes

- Column order is fixed: open, high, low, close.
- `open` and `close` come from first/last within the bin.
- `high` and `low` come from max/min over the same bin membership.
- This is one of the few `TimeIndex` resampling methods whose output is 2D.
- If you need only one aggregation component, the scalar-output resample methods
  are simpler.

## See Also

- [`TimeIndex::resample_sum`](resample_sum.md)
- [`TimeIndex::resample_agg`](resample_agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="resample_agg.html">
    <span class="label">Previous</span>
    <span class="title">resample_agg</span>
  </a>
  <a class="next-link" href="rolling_count.html">
    <span class="label">Next</span>
    <span class="title">rolling_count</span>
  </a>
</div>
