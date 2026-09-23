<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7eabd41c00b2 -->

# DataFrameTsAccessor::insert_gaps

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signatures

```cpp
DataFrameTsAccessor::insert_gaps(std::string_view freq, std::int64_t step = 1) const -> DataFrame
DataFrameTsAccessor::insert_gaps(std::string_view time_feature, std::string_view freq, std::int64_t step = 1) const -> DataFrame
```

## What It Does

Builds a regular grid of ticks (using the same frequency bucketing as
[`resample()`](resample.md)) spanning this accessor's own time range, then
reindexes every bound feature onto that grid with `TsAlignMode::Exact` — a thin
wrapper over [`align_to()`](align_to.md). Any tick in the grid that has no exact
match in the source data comes back `NULL`; unlike [`asfreq()`](asfreq.md), no
aggregation and no gap-filling happen here — this method only ever inserts
missing rows, it never combines or fills them.

## Parameters

**freq** (`std::string_view`)

The bucket frequency defining the regular grid — same calendar/frequency string
convention as [`resample()`](resample.md).

**step** (`std::int64_t`, default `1`)

A frequency multiplier, exactly as in [`resample()`](resample.md).

**time_feature** (`std::string_view`, one overload)

An explicit feature name supplying the time axis, instead of this accessor's case
labels or `TsOptions.axis`.

## Returns

`DataFrame`
    A new frame with one row per tick of the regular grid, `NULL` wherever the
    source data had no exact match.

## Examples

Example 1. Insert a missing calendar day as an explicit `NULL` row.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Thread(pluss::make_series({100.0, 102.0, 110.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-04T00:00:00Z"})));
std::cout << readings;

auto out = readings.ts().insert_gaps("D");
std::cout << out;
```

### Output (before)

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-04 | 110  
------------------
[3 cs x 1 feat]
```

### Output (after)

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-03 | NULL 
2024-01-04 | 110  
------------------
[4 cs x 1 feat]
```

`2024-01-03` never appeared in the source data, but it falls inside the
contiguous daily grid spanning `2024-01-01` through `2024-01-04`, so it's inserted
as an explicit `NULL` row rather than simply being absent.

Example 2. The `time_feature` overload, sourcing the time axis from a named
column instead of case labels.

```cpp
auto readings_named = DataFrame(
    {{"reading_time", Thread(pluss::make_timeline({
          "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-04T00:00:00Z"}))},
     {"value", Thread(pluss::make_series({100.0, 102.0, 110.0}))}});
std::cout << readings_named;

auto out2 = readings_named.ts().insert_gaps("reading_time", "D");
std::cout << out2;
```

### Output (before)

```text
==========================
     | reading_time  value
--------------------------
0    | 2024-01-01    100  
1    | 2024-01-02    102  
2    | 2024-01-04    110  
--------------------------
[3 cs x 2 feat]
```

### Output (after)

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-03 | NULL 
2024-01-04 | 110  
------------------
[4 cs x 1 feat]
```

`.ts()` auto-binds only the numeric `value` feature — `reading_time` is left
alone on the underlying frame so `insert_gaps("reading_time", "D")` can still
find it by name. The result is identical to Example 1: `reading_time` itself
drops out of the output columns, replaced by the regular grid as the new case
labels.

## Notes

- `insert_gaps(freq, step)` is exactly
  `align_to(bins.labels, TsAlignMode::Exact)`, where `bins` comes from the same
  `resample_bins`-style bucketing [`resample()`](resample.md) uses — see that
  method for the full frequency-string grammar.
- No aggregation happens here even if a bucket somehow contained more than one
  source tick at the exact same instant — use [`resample()`](resample.md) if
  values need combining.
- `DataFrame::ts()`/`ts(options)` auto-bind only the numeric `Series` columns on
  the calling frame; a named `Timeline`/`TimeIndex` column meant for
  `time_feature` is simply skipped rather than rejected, so a frame can carry
  both value features and a named time-axis column at once (Example 2).
- The `time_feature` overload consumes that column purely as an alignment key —
  it does not appear in the output, which instead takes the regular grid as its
  new case labels.

## See Also

- [`DataFrameTsAccessor::align_to`](align_to.md): the reindex primitive this
  method wraps with a frequency-derived grid.
- [`DataFrameTsAccessor::asfreq`](asfreq.md): the aggregating, gap-filling
  counterpart — reach for that instead when buckets may contain more than one
  observation or gaps should be filled rather than left `NULL`.
- [`DataFrameTsAccessor::resample`](resample.md): the bucketing convention this
  method's regular grid is built from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gt.html">
    <span class="label">Previous</span>
    <span class="title">gt</span>
  </a>
  <a class="next-link" href="lag.html">
    <span class="label">Next</span>
    <span class="title">lag</span>
  </a>
</div>
