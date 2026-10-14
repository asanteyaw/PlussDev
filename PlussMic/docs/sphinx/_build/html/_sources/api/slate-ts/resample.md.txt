<!-- pluss-hand-authored -->

# TimeSeriesAccessor::resample

[Back to TimeSeriesAccessor API Reference](../../slate_ts_accessor_api_reference)

## Signatures

```cpp
TimeSeriesAccessor::resample(const Slate& axis, std::string_view freq, ResampleOp op = ResampleOp::Mean, std::int64_t step = 1) const -> TimeSeriesResample
TimeSeriesAccessor::resample(const Slate& axis, const pluss::TimeIndex::ResampleOptions& options, ResampleOp op = ResampleOp::Mean) const -> TimeSeriesResample
```

## What It Does

Buckets the bound slate's values into regular calendar/duration periods (like
every day, every 6 hours, every week), keyed by an explicit `axis`, and
aggregates each bucket with `op`. `freq` supports fixed units (`S`/`min`/`H`/`D`,
optionally prefixed with a multiplier like `"6H"`), calendar-aware `W`
(weekly), `M` (monthly), and `Y` (yearly) buckets. Every bucket in the regular
grid between the data's minimum and maximum tick appears in the result —
including ones with zero source observations, which come back as `NULL` (for
`Mean`/`Min`/`Max`/`First`/`Last`) or `0` (for `Sum`/`Count`).

Unlike [`DataFrameTsAccessor::resample`](../dataframe-ts/resample.md), this
method has no lazy/chainable form and no implicit case-labels-as-axis
shorthand — `axis` and `op` are both always explicit arguments, and the result
is a plain `{labels, values}` pair rather than a rendered table.

```{important} shares rolling_block()'s missing-value-poisons-the-bucket behavior
Like [`zscore_rolling()`](zscore_rolling.md) and
[`rolling_block()`](rolling_block.md), `Sum`/`Mean` here accumulate every
position in a bucket with plain floating-point arithmetic — one missing entry
poisons its entire bucket to `NULL`, rather than being skipped and computed
over the remaining real observations in that bucket.
```

## Parameters

**axis** (`const Slate&`)
    The time axis (`Timeline`/`TimeIndex`), same length as the bound slate,
    supplying each position's bucket membership.

**freq** (`std::string_view`) / **options** (`const pluss::TimeIndex::ResampleOptions&`)
    The bucket frequency. `options` additionally exposes `step`, `closed`
    (`Left`/`Right`), `label` (`Left`/`Right`), `origin`
    (`StartDay`/`Start`/`Epoch`), and `offset_nanos` for finer control than a
    bare frequency string.

**op** (`ResampleOp`, default `Mean`)
    The aggregation applied to each bucket: `Mean`, `Sum`, `Min`, `Max`,
    `First`, `Last`, or `Count`.

**step** (`std::int64_t`, default `1`)
    A frequency multiplier — for example `step = 6` with `freq = "H"` produces
    6-hour buckets.

## Returns

`TimeSeriesResample`
    A `{labels, values}` pair: `labels` is a `Timeline`/`TimeIndex` slate of
    bucket boundaries, `values` is a `Series` slate of the aggregated result,
    one entry per bucket, in the same order.

## Examples

Example 1. Daily buckets, summed — including an empty bucket for a day with no
observations at all.

```cpp
using namespace pluss::table;

auto axis = Slate(pluss::make_timeline({
    "2024-01-01T08:00:00Z", "2024-01-01T20:00:00Z",
    "2024-01-03T10:00:00Z", "2024-01-03T15:00:00Z"}));
auto readings = Slate(pluss::make_series({10.0, 20.0, 30.0, 5.0}));

auto summed = readings.ts().resample(axis, "D", pluss::subsp::stats::ts::ResampleOp::Sum);
std::cout << summed.labels;
std::cout << summed.values;
```

### Output

```text
0  2024-01-01
1  2024-01-02
2  2024-01-03

ptype: Timeline
0  30
1  0
2  35

ptype: Series
```

`2024-01-01`'s bucket sums its two observations (`10 + 20 = 30`); `2024-01-02`
has no observations at all, but still appears in the regular daily grid with a
sum of `0`; `2024-01-03` sums `30 + 5 = 35`.

Example 2. The same axis, aggregated with `Mean` instead — an empty bucket is
`NULL`, not `0`.

```cpp
auto mean = readings.ts().resample(axis, "D", pluss::subsp::stats::ts::ResampleOp::Mean);
std::cout << mean.values;
```

### Output

```text
0  15
1  NULL
2  17.5

ptype: Series
```

Unlike `Sum`, an empty bucket's `Mean` is `NULL` — there's no meaningful
average of zero observations.

## Notes

- Every bucket in the contiguous grid from the data's minimum to maximum tick
  appears in the result, even ones with zero observations — `resample()` never
  silently skips empty periods.
- `Sum`/`Count` on an empty bucket produce `0`; `Mean`/`Min`/`Max`/`First`/
  `Last` produce `NULL`.
- Bucket boundaries default to `closed = Left`, `label = Left`, `origin =
  StartDay` — the first bucket starts at midnight (in the axis's own time
  zone) on the day of the earliest observation, and each period is
  `[start, start + freq)`.
- See the important callout above: a missing value inside a bucket poisons
  that bucket's `Sum`/`Mean` entirely, the same way it does for
  [`rolling_block()`](rolling_block.md).

## See Also

- [`TimeSeriesAccessor::align_to`](align_to.md): a 1:1 reindex onto an explicit
  axis, rather than a many-to-one aggregation into buckets.
- [`TimeSeriesAccessor::fill_gaps`](fill_gaps.md): filling gaps within an
  already-regular axis, rather than building one via bucketing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ne.html">
    <span class="label">Previous</span>
    <span class="title">ne</span>
  </a>
  <a class="next-link" href="returns.html">
    <span class="label">Next</span>
    <span class="title">returns</span>
  </a>
</div>
