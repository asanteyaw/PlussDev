# Time-Series Accessor Guide

This page documents the `.ts()` surface for `DataFrame`, `Thread`, and `Atlas`.
Use `.ts()` for numeric time-series values. Use `.tl()` for datetime/timeline
values.

## API Signature

- `DataFrame::ts() -> DataFrameTsAccessor`
- `Thread::ts() -> TimeSeriesAccessor`
- `Atlas::ts() -> AtlasTsAccessor`

Compatibility overloads still accept feature names, but preferred public code is
selection-first: select the value feature(s), then call `.ts()` with no feature
names.

## Notes

Use `DataFrame::ts()` after selecting numeric value features when you want transforms returned as a `DataFrame`.  
Use `Thread::ts()` when you want transforms returned as a `Thread` for one column.
Use `Atlas::ts()` when you want the same transform delegated to every frame in an `Atlas`.

Preferred selection-first style:

```cpp
auto values = df.label().set("date", false).features({"close", "volume"});
auto daily = values.ts().resample("D").mean();
auto roll = values.ts().rolling(20, pluss::table::WindowStatistic::Mean);
auto seven_day = values.ts().rolling("7D", pluss::table::WindowStatistic::Mean);
```

Text-to-datetime parsing is an explicit `cast(...)`, then timeline operations
belong on the `Timeline` surface:

```cpp
auto dates = df.cast({
    pluss::table::DataFrame::CastSpec{
        "date_text",
        pluss::table::ThreadCastTarget::Timeline,
        pluss::table::ThreadCastOptions{
            .format = "%Y-%m-%d",
            .invalid = pluss::table::ParseInvalid::Null}},
}).features({"date_text"});
auto years = dates.tl().year();
```

If `DataFrame::ts()` is called with no feature arguments, every feature in the
current frame must be a numeric `Series`. A mixed frame throws a Pluss error.
This keeps surface application intentional:

```cpp
auto ok = df.features({"close", "volume"}).ts().rolling(5);
// df.ts().rolling(5) throws if df also contains text or timeline columns.
```

Inputs:
- Numeric `Series`-kind features.
- For cases-based time-axis methods (`resample`, `asfreq`, `align_to`,
  `reindex_time`), the frame cases must be `Timeline`/`TimeIndex`. Use
  `label().set("date")` first.
- Overloads that accept `time_feature` are retained for compatibility.

Output:
- `DataFrameTsAccessor` methods return `DataFrame`.
- `TimeSeriesAccessor` methods return `Thread` (or `{labels, values}` for `resample`).

## DataFrame Surface

### API Signature

- `returns(periods: int64 = 1) -> DataFrame`
- `log_returns(periods: int64 = 1) -> DataFrame`
- `diff(periods: int64 = 1) -> DataFrame`
- `lag(periods: int64 = 1) -> DataFrame`
- `lead(periods: int64 = 1) -> DataFrame`
- `rolling(window: int64, statistic: WindowStatistic = Mean, options: WindowOptions = {}) -> DataFrame`
- `rolling(duration: string_view, statistic: WindowStatistic = Mean, options: StatisticOptions = {}, closure: WindowClosure = Right) -> DataFrame`
- `expanding(statistic: WindowStatistic = Mean, options: StatisticOptions = {}) -> DataFrame`
- `ewm(alpha: double, statistic: WindowStatistic = Mean, adjusted: bool = true, bias_corrected: bool = true, min_count: int64 = 1, ignore_nulls: bool = false) -> DataFrame`
- `zscore_rolling(window: int64, min_periods: int64 = 1, epsilon: double = 1e-12) -> DataFrame`
- `fill_gaps(mode: GapFillMode = Forward, constant: optional<double> = nullopt) -> DataFrame`
- `sort(ascending: bool = true, nulls_last: bool = true) -> DataFrame`
- `resample(freq: string_view, step: int64 = 1) -> DataFrameTsResampler`
- `resample(freq: string_view, op: ResampleOp, step: int64 = 1) -> DataFrame`
- `resample(time_feature: string_view, freq: string_view, step: int64 = 1) -> DataFrameTsResampler`
- `resample(time_feature: string_view, freq: string_view, op: ResampleOp, step: int64 = 1) -> DataFrame`
- `asfreq(freq: string_view, step: int64 = 1, fill_mode: GapFillMode = Forward, constant: optional<double> = nullopt, op: ResampleOp = Last) -> DataFrame`
- `asfreq(time_feature: string_view, freq: string_view, step: int64 = 1, fill_mode: GapFillMode = Forward, constant: optional<double> = nullopt, op: ResampleOp = Last) -> DataFrame`
- `insert_gaps(freq: string_view, step: int64 = 1) -> DataFrame`
- `insert_gaps(time_feature: string_view, freq: string_view, step: int64 = 1) -> DataFrame`
- `align_to(target_axis: Thread, mode: TsAlignMode = Exact) -> DataFrame`
- `align_to(time_feature: string_view, target_axis: Thread, mode: TsAlignMode = Exact) -> DataFrame`
- `reindex_time(target_axis: Thread, mode: TsAlignMode = Exact, fill: TsReindexFill = None, constant: optional<double> = nullopt) -> DataFrame`
- `reindex_time(time_feature: string_view, target_axis: Thread, mode: TsAlignMode = Exact, fill: TsReindexFill = None, constant: optional<double> = nullopt) -> DataFrame`
- `calendar(time_feature: string_view) -> DataFrame`
- `lag_block(lags: {int64...}) -> DataFrame`
- `rolling_block(windows: {int64...}, options: TsRollingBlockOptions = {}) -> DataFrame`

### Example

```cpp
#include <pluss/mic.h>

using pluss::table::DataFrame;

DataFrame df = DataFrame::load_csv("prices.csv")
                 .cast({
                     pluss::table::DataFrame::CastSpec{
                         "date", pluss::table::ThreadCastTarget::Timeline, {}},
                 });

auto values = df.label().set("date", false).features({"Adj Close", "Volume"});
auto lr = values.ts().log_returns();
auto lag = values.ts().lag_block({1, 2, 5});
auto roll = values.ts().rolling(20, pluss::table::WindowStatistic::Mean, {.min_count = 5});
auto roll_time = values.ts().rolling("7D", pluss::table::WindowStatistic::Mean);
auto growing = values.ts().expanding(pluss::table::WindowStatistic::Var, {.ddof = 1, .min_count = 2});
auto decay = values.ts().ewm(0.2, pluss::table::WindowStatistic::Mean);
auto sorted = values.ts().sort();
auto daily = values.ts().resample("D").mean();
auto weekly_last = values.ts().resample("W").last();
auto six_hour = values.ts().resample("6H").asfreq();
auto six_hour_ffill = values.ts().resample("6H").ffill();
```

### Expected output

- `lr`, `lag`, `roll`, and `daily` are all `DataFrame` objects.
- `values.ts().resample("D")` uses the timeline/timeindex cases and returns a
  resampler object, then `.mean()`, `.last()`, `.asfreq()`, `.ffill()`, and
  similar methods materialize the final `DataFrame`.
- Selected features are transformed and returned with aligned cases/time axis.

## Thread Surface

### API Signature

- `returns(periods: int64 = 1) -> Thread`
- `log_returns(periods: int64 = 1) -> Thread`
- `diff(periods: int64 = 1) -> Thread`
- `lag(periods: int64 = 1) -> Thread`
- `lead(periods: int64 = 1) -> Thread`
- `rolling(window: int64, statistic: WindowStatistic = Mean, options: WindowOptions = {}) -> Thread`
- `rolling(axis: Thread, duration: string_view, statistic: WindowStatistic = Mean, options: StatisticOptions = {}, closure: WindowClosure = Right) -> Thread`
- `expanding(statistic: WindowStatistic = Mean, options: StatisticOptions = {}) -> Thread`
- `ewm(alpha: double, statistic: WindowStatistic = Mean, adjusted: bool = true, bias_corrected: bool = true, min_count: int64 = 1, ignore_nulls: bool = false) -> Thread`
- `zscore_rolling(window: int64, min_periods: int64 = 1, epsilon: double = 1e-12) -> Thread`
- `fill_gaps(mode: GapFillMode = Forward, constant: optional<double> = nullopt) -> Thread`
- `resample(axis: Thread, freq: string_view, op: ResampleOp = Mean, step: int64 = 1) -> TimeSeriesResample`

### Example

```cpp
#include <pluss/mic.h>

using pluss::table::DataFrame;

DataFrame df = DataFrame::load_csv("prices.csv").cast({
    pluss::table::DataFrame::CastSpec{
        "date", pluss::table::ThreadCastTarget::Timeline, {}},
});

auto s = df.features("Adj Close");
auto lr = s.ts().log_returns();
auto z = s.ts().rolling(30, pluss::table::WindowStatistic::Mean, {.min_count = 10});
auto r7 = s.ts().rolling(df.label().values(), "7D", pluss::table::WindowStatistic::Mean);
auto rs = s.ts().resample(df.features("date"), "W", pluss::subsp::stats::ts::ResampleOp::Last, 1);
```

### Expected output

- `lr` and `z` are `Thread`.
- `rs.labels` is timeline/timeindex-like labels and `rs.values` is resampled values.

## Common Pitfall

If you want a transformed **DataFrame**, use `df.ts(...)...`.  
If you call `df.features(\"col\").ts()...`, you are on the **Thread** surface and the return type is `Thread`.
