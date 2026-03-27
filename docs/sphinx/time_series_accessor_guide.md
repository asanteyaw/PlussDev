# Time-Series Accessor Guide

This guide documents the general time-series surface built on top of `Series`, `Thread`, and `DataFrame`.

## Quick Start

```cpp
#include <pluss/mic.h>

using pluss::table::DataFrame;

DataFrame df = /* load or build */;

// Single-column TS transform.
auto lr = df.ts("Adj Close").log_returns();

// One-liner assignment.
df.set("Log Returns") = df.ts("Adj Close").log_returns();
```

## Surfaces

## `Thread` surface

`df.features("Adj Close").ts()` returns a `TimeSeriesAccessor` with:

- `returns`, `log_returns`, `diff`, `lag`, `lead`
- `rolling_mean`, `rolling_std`, `rolling_sum`, `rolling_var`, `rolling_min`, `rolling_max`
- `ema`, `ewm_mean`, `ewm_std`
- `expanding_mean`, `expanding_std`
- `zscore_rolling`
- `fill_gaps`
- `resample(axis, freq, op, step)` returning `{labels, values}`

## `DataFrame` surface

`df.ts("feature")` or `df.ts({"f1","f2"})` returns `DataFrameTsAccessor`.

Transform methods return a `DataFrame` containing transformed selected features:

- `returns`, `log_returns`, `diff`, `lag`, `lead`
- `rolling_mean`, `rolling_std`, `rolling_sum`, `rolling_var`, `rolling_min`, `rolling_max`
- `ema`, `ewm_mean`, `ewm_std`
- `expanding_mean`, `expanding_std`
- `zscore_rolling`, `fill_gaps`

Time-axis utilities:

- `resample(time_feature, freq, op, step)`
- `asfreq(time_feature, freq, step, fill_mode, constant, op)`
- `insert_gaps(time_feature, freq, step)`
- `align_to(time_feature, target_axis, mode)`
- `reindex_time(time_feature, target_axis, mode, fill, constant)`

Feature builders:

- `calendar(time_feature)`
- `lag_block({lags...})`
- `rolling_block({windows...}, options)`

## Example: End-to-End Feature Prep

```cpp
auto lr = df.ts("Adj Close").log_returns();
auto lag = df.ts({"Adj Close", "Volume"}).lag_block({1, 2, 3});
auto roll = df.ts({"Adj Close", "Volume"}).rolling_block({5, 20});
auto cal = df.ts("Adj Close").calendar("date");

auto daily = df.ts({"Adj Close", "Volume"}).resample(
    "date", "D", pluss::subsp::stats::ts::ResampleOp::Mean, 1);
```

## Notes

- Computation follows `Series` platform semantics.
- Missing alignment/reindex positions are represented as `NaN` in numeric outputs.
- For assignment to a new feature, `DataFrame::set(name)=DataFrame` requires the right-hand side to have one feature.
