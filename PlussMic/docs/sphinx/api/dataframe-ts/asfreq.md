<!-- pluss-hand-authored -->

# DataFrameTsAccessor::asfreq

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signatures

```cpp
DataFrameTsAccessor::asfreq(const pluss::TimeIndex::ResampleOptions& options, pluss::subsp::stats::ts::GapFillMode fill_mode = pluss::subsp::stats::ts::GapFillMode::Forward, std::optional<double> constant = std::nullopt, pluss::subsp::stats::ts::ResampleOp op = pluss::subsp::stats::ts::ResampleOp::Last) const -> DataFrame
DataFrameTsAccessor::asfreq(std::string_view freq, std::int64_t step = 1, pluss::subsp::stats::ts::GapFillMode fill_mode = pluss::subsp::stats::ts::GapFillMode::Forward, std::optional<double> constant = std::nullopt, pluss::subsp::stats::ts::ResampleOp op = pluss::subsp::stats::ts::ResampleOp::Last) const -> DataFrame
DataFrameTsAccessor::asfreq(std::string_view time_feature, const pluss::TimeIndex::ResampleOptions& options, pluss::subsp::stats::ts::GapFillMode fill_mode = pluss::subsp::stats::ts::GapFillMode::Forward, std::optional<double> constant = std::nullopt, pluss::subsp::stats::ts::ResampleOp op = pluss::subsp::stats::ts::ResampleOp::Last) const -> DataFrame
DataFrameTsAccessor::asfreq(std::string_view time_feature, std::string_view freq, std::int64_t step = 1, pluss::subsp::stats::ts::GapFillMode fill_mode = pluss::subsp::stats::ts::GapFillMode::Forward, std::optional<double> constant = std::nullopt, pluss::subsp::stats::ts::ResampleOp op = pluss::subsp::stats::ts::ResampleOp::Last) const -> DataFrame
```

## What It Does

[`resample()`](resample.md)s every bound feature onto the regular grid implied by
`freq`/`options` using `op` to combine any bucket that contains more than one
source observation, then immediately runs [`fill_gaps()`](fill_gaps.md) with
`fill_mode`/`constant` over the resampled result. Since
[`resample()`](resample.md) already produces one row per bucket in the regular
grid — including empty ones, which come back `NULL` — the only thing `asfreq()`
adds on top is filling those `NULL`s in a single call.

## Parameters

**freq** / **options**

The bucket frequency, exactly as in [`resample()`](resample.md).

**step** (`std::int64_t`, default `1`)

A frequency multiplier, exactly as in [`resample()`](resample.md).

**fill_mode** (`GapFillMode`, default `Forward`)

The [`fill_gaps()`](fill_gaps.md) strategy applied to empty buckets after
resampling.

**constant** (`std::optional<double>`, default `std::nullopt`)

The fill value when `fill_mode == GapFillMode::Constant`.

**op** (`ResampleOp`, default `Last`)

How a bucket with more than one source observation is combined — see
[`resample()`](resample.md) for the full set (`Mean`/`Sum`/`Min`/`Max`/`First`/
`Last`/`Count`).

**time_feature** (`std::string_view`, two overloads)

An explicit feature name supplying the time axis, instead of using this accessor's
case labels or `TsOptions.axis`.

## Returns

`DataFrame`
    A new frame with one row per bucket in the regular grid, gaps filled according
    to `fill_mode`.

## Examples

Example 1. Resample to a daily grid with `Last`, forward-filling the empty day.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Thread(pluss::make_series({10.0, 20.0, 30.0, 5.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T08:00:00Z", "2024-01-01T20:00:00Z",
        "2024-01-03T10:00:00Z", "2024-01-03T15:00:00Z"})));
std::cout << readings;

auto out = readings.ts().asfreq("D");
std::cout << out;
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

### Output (after)

```text
==================
           | value
------------------
2024-01-01 | 20   
2024-01-02 | 20   
2024-01-03 | 5    
------------------
[3 cs x 1 feat]
```

`2024-01-01`'s bucket has two observations (`10`, `20`); with the default
`op = Last`, it keeps `20`. `2024-01-02` has no observations at all — an empty
bucket — so [`resample()`](resample.md) alone would leave it `NULL`; the default
`fill_mode = Forward` immediately carries `2024-01-01`'s `20` into it. `2024-01-03`
also has two observations (`30`, `5`); `Last` keeps `5`.

Example 2. Pass an explicit non-default `op`, to combine multi-observation
buckets differently.

```cpp
using namespace pluss::table;

auto out2 = readings.ts().asfreq(
    "D", 1, pluss::subsp::stats::ts::GapFillMode::Forward, std::nullopt,
    pluss::subsp::stats::ts::ResampleOp::Sum);
std::cout << out2;
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 30   
2024-01-02 | 30   
2024-01-03 | 35   
------------------
[3 cs x 1 feat]
```

With `op = Sum` instead of the default `Last`, `2024-01-01`'s bucket becomes
`10 + 20 = 30` and `2024-01-03`'s becomes `30 + 5 = 35`. `2024-01-02` still has
no observations of its own, so the default `fill_mode = Forward` carries the
now-summed `30` into it, exactly as before.

## Notes

- `asfreq()` is exactly `resample(freq/options, op)` followed by
  `fill_gaps(fill_mode, constant)` on every bound feature — see
  [`resample()`](resample.md) for the underlying bucketing rules.
- The default `op = Last` differs from [`resample()`](resample.md)'s own default
  behavior of requiring an explicit `op` — `asfreq()` picks `Last` to match the
  common "most recent observation stands for the period" convention.
- `fill_mode = GapFillMode::Zero`/`Constant` fill **every** empty bucket
  unconditionally; `Forward`/`Backward` only fill gaps that have a qualifying
  neighbor in that direction — see [`fill_gaps()`](fill_gaps.md) for the full
  behavior.

## See Also

- [`DataFrameTsAccessor::resample`](resample.md): the bucketing step this method
  runs before filling gaps.
- [`DataFrameTsAccessor::fill_gaps`](fill_gaps.md): the gap-fill step this method
  runs after resampling.
- [`DataFrameTsResampler::asfreq`](../dataframe-ts-resample/asfreq.md): a related
  but distinct operation — a plain reindex (no aggregation) with an optional single
  fill value, reached via [`resample()`](resample.md)'s chainable form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align_to.html">
    <span class="label">Previous</span>
    <span class="title">align_to</span>
  </a>
  <a class="next-link" href="bounded.html">
    <span class="label">Next</span>
    <span class="title">bounded</span>
  </a>
</div>
