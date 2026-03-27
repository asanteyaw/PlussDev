# Time-Series Accessor Guide

This page documents the `.ts()` surface for `DataFrame` and `Thread`.

## API Signature

- `DataFrame::ts(feature: string_view) -> DataFrameTsAccessor`
- `DataFrame::ts(features: {string_view...}) -> DataFrameTsAccessor`
- `Thread::ts() -> TimeSeriesAccessor`

## Notes

Use `DataFrame::ts(...)` when you want transforms returned as a `DataFrame` (single or multi-feature).  
Use `Thread::ts()` when you want transforms returned as a `Thread` for one column.

Inputs:
- Numeric `Series`-kind features.
- For time-axis methods (`resample`, `asfreq`, `align_to`, `reindex_time`), a timeline/timeindex column is required.

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
- `rolling_mean(window: int64, min_periods: int64 = 1) -> DataFrame`
- `rolling_std(window: int64, min_periods: int64 = 1) -> DataFrame`
- `rolling_sum(window: int64, min_periods: int64 = 1) -> DataFrame`
- `rolling_var(window: int64, min_periods: int64 = 1) -> DataFrame`
- `rolling_min(window: int64, min_periods: int64 = 1) -> DataFrame`
- `rolling_max(window: int64, min_periods: int64 = 1) -> DataFrame`
- `ema(alpha: double) -> DataFrame`
- `ewm_mean(alpha: double) -> DataFrame`
- `ewm_std(alpha: double, epsilon: double = 1e-12) -> DataFrame`
- `expanding_mean(min_periods: int64 = 1) -> DataFrame`
- `expanding_std(min_periods: int64 = 1, epsilon: double = 1e-12) -> DataFrame`
- `zscore_rolling(window: int64, min_periods: int64 = 1, epsilon: double = 1e-12) -> DataFrame`
- `fill_gaps(mode: GapFillMode = Forward, constant: optional<double> = nullopt) -> DataFrame`
- `resample(time_feature: string_view, freq: string_view, op: ResampleOp = Mean, step: int64 = 1) -> DataFrame`
- `asfreq(time_feature: string_view, freq: string_view, step: int64 = 1, fill_mode: GapFillMode = Forward, constant: optional<double> = nullopt, op: ResampleOp = Last) -> DataFrame`
- `insert_gaps(time_feature: string_view, freq: string_view, step: int64 = 1) -> DataFrame`
- `align_to(time_feature: string_view, target_axis: Thread, mode: TsAlignMode = Exact) -> DataFrame`
- `reindex_time(time_feature: string_view, target_axis: Thread, mode: TsAlignMode = Exact, fill: TsReindexFill = None, constant: optional<double> = nullopt) -> DataFrame`
- `calendar(time_feature: string_view) -> DataFrame`
- `lag_block(lags: {int64...}) -> DataFrame`
- `rolling_block(windows: {int64...}, options: TsRollingBlockOptions = {}) -> DataFrame`

### Example

```cpp
#include <pluss/mic.h>

using pluss::table::DataFrame;

DataFrame df = DataFrame::load_csv("prices.csv")
                 .cast_timeline({"date"});

auto lr = df.ts("Adj Close").log_returns();
auto lag = df.ts({"Adj Close", "Volume"}).lag_block({1, 2, 5});
auto roll = df.ts({"Adj Close"}).rolling_mean(20, 5);
auto daily = df.ts({"Adj Close", "Volume"}).resample(
    "date", "D", pluss::subsp::stats::ts::ResampleOp::Mean, 1);
```

### Expected output

- `lr`, `lag`, `roll`, and `daily` are all `DataFrame` objects.
- Selected features are transformed and returned with aligned cases/time axis.

## Thread Surface

### API Signature

- `returns(periods: int64 = 1) -> Thread`
- `log_returns(periods: int64 = 1) -> Thread`
- `diff(periods: int64 = 1) -> Thread`
- `lag(periods: int64 = 1) -> Thread`
- `lead(periods: int64 = 1) -> Thread`
- `rolling_mean(window: int64, min_periods: int64 = 1) -> Thread`
- `rolling_std(window: int64, min_periods: int64 = 1) -> Thread`
- `rolling_sum(window: int64, min_periods: int64 = 1) -> Thread`
- `rolling_var(window: int64, min_periods: int64 = 1) -> Thread`
- `rolling_min(window: int64, min_periods: int64 = 1) -> Thread`
- `rolling_max(window: int64, min_periods: int64 = 1) -> Thread`
- `ema(alpha: double) -> Thread`
- `ewm_mean(alpha: double) -> Thread`
- `ewm_std(alpha: double, epsilon: double = 1e-12) -> Thread`
- `expanding_mean(min_periods: int64 = 1) -> Thread`
- `expanding_std(min_periods: int64 = 1, epsilon: double = 1e-12) -> Thread`
- `zscore_rolling(window: int64, min_periods: int64 = 1, epsilon: double = 1e-12) -> Thread`
- `fill_gaps(mode: GapFillMode = Forward, constant: optional<double> = nullopt) -> Thread`
- `resample(axis: Thread, freq: string_view, op: ResampleOp = Mean, step: int64 = 1) -> TimeSeriesResample`

### Example

```cpp
#include <pluss/mic.h>

using pluss::table::DataFrame;

DataFrame df = DataFrame::load_csv("prices.csv").cast_timeline({"date"});

auto s = df.features("Adj Close");
auto lr = s.ts().log_returns();
auto z = s.ts().zscore_rolling(30, 10, 1e-12);
auto rs = s.ts().resample(df.features("date"), "W", pluss::subsp::stats::ts::ResampleOp::Last, 1);
```

### Expected output

- `lr` and `z` are `Thread`.
- `rs.labels` is timeline/timeindex-like labels and `rs.values` is resampled values.

## Common Pitfall

If you want a transformed **DataFrame**, use `df.ts(...)...`.  
If you call `df.features(\"col\").ts()...`, you are on the **Thread** surface and the return type is `Thread`.

