<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7a6612d8b3c8 -->

# DataFrameTsAccessor::resample

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signatures

```cpp
DataFrameTsAccessor::resample(std::string_view freq, std::int64_t step = 1) const -> DataFrameTsResampler
DataFrameTsAccessor::resample(const pluss::TimeIndex::ResampleOptions& options) const -> DataFrameTsResampler
DataFrameTsAccessor::resample(std::string_view freq, pluss::subsp::stats::ts::ResampleOp op, std::int64_t step = 1) const -> DataFrame
DataFrameTsAccessor::resample(const pluss::TimeIndex::ResampleOptions& options, pluss::subsp::stats::ts::ResampleOp op) const -> DataFrame
DataFrameTsAccessor::resample(std::string_view time_feature, std::string_view freq, std::int64_t step = 1) const -> DataFrameTsResampler
DataFrameTsAccessor::resample(std::string_view time_feature, const pluss::TimeIndex::ResampleOptions& options) const -> DataFrameTsResampler
DataFrameTsAccessor::resample(std::string_view time_feature, std::string_view freq, pluss::subsp::stats::ts::ResampleOp op, std::int64_t step = 1) const -> DataFrame
DataFrameTsAccessor::resample(std::string_view time_feature, const pluss::TimeIndex::ResampleOptions& options, pluss::subsp::stats::ts::ResampleOp op) const -> DataFrame
```

## What It Does

Buckets every bound feature into regular calendar/duration periods (like every
day, every 6 hours, every week) and aggregates each bucket. `freq` supports
fixed units (`S`/`min`/`H`/`D`, optionally prefixed with a multiplier like
`"6H"`), calendar-aware `W` (weekly), `M` (monthly), and `Y` (yearly) buckets.
Every bucket in the regular grid between the data's minimum and maximum tick
appears in the result — including ones with zero source observations, which come
back as `NULL` (for `Mean`/`Min`/`Max`/`First`/`Last`) or `0` (for `Sum`/`Count`).

- **Without an `op`** (returns `DataFrameTsResampler`): a lazy, chainable form —
  call `.mean()`, `.sum()`, `.min()`, `.max()`, `.first()`, `.last()`, or
  `.count()` on the result to actually aggregate, or `.asfreq()`/`.ffill()`/
  `.bfill()`/`.nearest()` to reindex instead of aggregate.
- **With an `op`** (returns `DataFrame` directly): equivalent to calling the
  matching method on the chainable form in one step.

## Parameters

**freq** (`std::string_view`) / **options** (`const pluss::TimeIndex::ResampleOptions&`)

The bucket frequency. `options` additionally exposes `step`, `closed`
(`Left`/`Right`), `label` (`Left`/`Right`), `origin`
(`StartDay`/`Start`/`Epoch`), and `offset_nanos` for finer control than a bare
frequency string.

**step** (`std::int64_t`, default `1`)

A frequency multiplier — for example `step = 6` with `freq = "H"` produces 6-hour
buckets.

**op** (`ResampleOp`, overloads that return `DataFrame` directly)

The aggregation applied to each bucket: `Mean`, `Sum`, `Min`, `Max`, `First`,
`Last`, or `Count`.

**time_feature** (`std::string_view`, four overloads)

An explicit feature name supplying the time axis, instead of this accessor's case
labels or `TsOptions.axis`.

## Returns

`DataFrameTsResampler`
    (No-`op` overloads) A chainable accessor; nothing is aggregated until one of
    its own methods is called.

`DataFrame`
    (`op`-taking overloads) The aggregated result directly, one row per bucket.

## Examples

Example 1. Daily buckets, aggregated directly with an explicit `op`.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Thread(pluss::make_series({10.0, 20.0, 30.0, 5.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T08:00:00Z", "2024-01-01T20:00:00Z",
        "2024-01-03T10:00:00Z", "2024-01-03T15:00:00Z"})));
std::cout << readings;

auto summed = readings.ts().resample("D", pluss::subsp::stats::ts::ResampleOp::Sum);
std::cout << summed;
```

### Output (before)

```text
===========================
                    | value
---------------------------
2024-01-01 08:00:00 | 10   
2024-01-01 20:00:00 | 20   
2024-01-03 10:00:00 | 30   
2024-01-03 15:00:00 | 5    
---------------------------
[4 cs x 1 feat]
```

### Output (sum)

```text
==================
           | value
------------------
2024-01-01 | 30   
2024-01-02 | 0    
2024-01-03 | 35   
------------------
[3 cs x 1 feat]
```

`2024-01-01`'s bucket sums its two observations (`10 + 20 = 30`); `2024-01-02` has
no observations at all, but still appears in the regular daily grid with a sum of
`0`; `2024-01-03` sums `30 + 5 = 35`.

Example 2. The equivalent chainable form, plus `.mean()` on the same buckets.

```cpp
auto mean = readings.ts().resample("D").mean();
std::cout << mean;
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 15   
2024-01-02 | NULL 
2024-01-03 | 17.5 
------------------
[3 cs x 1 feat]
```

`.resample("D")` alone returns a `DataFrameTsResampler` with no aggregation
performed yet; `.mean()` triggers it. Unlike `Sum`, an empty bucket's `Mean` is
`NULL` (there's no meaningful average of zero observations), not `0`.

Example 3. The `time_feature` overload, sourcing the time axis from a named
column instead of case labels.

```cpp
auto readings_named = DataFrame(
    {{"reading_time", Thread(pluss::make_timeline({
          "2024-01-01T08:00:00Z", "2024-01-01T20:00:00Z",
          "2024-01-03T10:00:00Z", "2024-01-03T15:00:00Z"}))},
     {"value", Thread(pluss::make_series({10.0, 20.0, 30.0, 5.0}))}});
std::cout << readings_named;

auto summed_named = readings_named.ts().resample(
    "reading_time", "D", pluss::subsp::stats::ts::ResampleOp::Sum);
std::cout << summed_named;
```

### Output (before)

```text
=================================
     | reading_time         value
---------------------------------
0    | 2024-01-01 08:00:00  10   
1    | 2024-01-01 20:00:00  20   
2    | 2024-01-03 10:00:00  30   
3    | 2024-01-03 15:00:00  5    
---------------------------------
[4 cs x 2 feat]
```

### Output (after)

```text
==================
           | value
------------------
2024-01-01 | 30   
2024-01-02 | 0    
2024-01-03 | 35   
------------------
[3 cs x 1 feat]
```

`.ts()` auto-binds only the numeric `value` feature — `reading_time` is left
alone on the underlying frame so `resample("reading_time", ...)` can still find
it by name. The result replaces the original integer case labels with the
bucket labels, and drops `reading_time` itself from the output columns (it only
ever served as the alignment axis); the bucketed values are identical to
Example 1.

## Notes

- Every bucket in the contiguous grid from the data's minimum to maximum tick
  appears in the result, even ones with zero observations — `resample()` never
  silently skips empty periods (that's exactly what makes it useful for finding
  gaps, alongside [`insert_gaps()`](insert_gaps.md)).
- `Sum`/`Count` on an empty bucket produce `0`; `Mean`/`Min`/`Max`/`First`/`Last`
  produce `NULL`.
- Bucket boundaries default to `closed = Left`, `label = Left`, `origin =
  StartDay` — the first bucket starts at midnight (in the axis's own time zone) on
  the day of the earliest observation, and each period is `[start, start + freq)`.
- `DataFrame::ts()`/`ts(options)` auto-bind only the numeric `Series` columns on
  the calling frame; a named `Timeline`/`TimeIndex` column meant for
  `time_feature` is simply skipped rather than rejected, so a frame can carry
  both value features and a named time-axis column at once (Example 3).
- The `time_feature` overloads consume that column purely as an alignment key —
  it does not appear in the output, which instead takes the resample bucket
  labels as its new case labels.

## See Also

- [`DataFrameTsAccessor::asfreq`](asfreq.md): resample-then-fill-gaps in one call.
- [`DataFrameTsAccessor::insert_gaps`](insert_gaps.md): the non-aggregating,
  `NULL`-preserving sibling of this method.
- [`DataFrameTsResampler`](../../dataframe_ts_resampler_api_reference): the
  chainable accessor this method's no-`op` overloads return.

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
