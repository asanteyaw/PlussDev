# Time-Series Accessor Guide

This guide documents the general time-series surface built on top of `Series`, `Thread`, and `DataFrame`.

## Quick Start

```cpp
#include <pluss/mic.h>

using pluss::table::DataFrame;
using pluss::table::TsOptions;

DataFrame df = /* load or build */;

// Single-column TS transform.
auto lr = df.features({"Adj Close"}).ts().log_returns();

// One-liner assignment.
df.set("Log Returns") = df.features({"Adj Close"}).ts().log_returns();
```

## Surfaces

## `Thread` surface

`df.features("Adj Close").ts()` returns a `TimeSeriesAccessor` with:

- `returns`, `log_returns`, `diff`, `lag`, `lead`
- `rolling(window, statistic, options)`
- `timed(axis, window, statistic, options)`
- `grouped(group, window, statistic, options)`
- `ewm(alpha, statistic, adjusted, bias_corrected, min_count, ignore_nulls)`
- `expanding(statistic, options)`
- `zscore_rolling`
- `fill_gaps`
- `resample(axis, freq, op, step)` returning `{labels, values}`

## `DataFrame` surface

`df.features({"f1","f2"}).ts()` returns `DataFrameTsAccessor` for selected value features.
Use `TsOptions` when the operation needs a separate time or group axis:

```cpp
auto opts = TsOptions{.axis = df.features("date")};
auto daily = df.features({"Adj Close", "Volume"}).ts(opts).resample("D").mean();
```

Transform methods return a `DataFrame` containing transformed selected features:

- `returns`, `log_returns`, `diff`, `lag`, `lead`
- `rolling(window, statistic, options)`
- `rolling(duration, statistic, options, closure)`
- `grouped(window, statistic, options)` when `TsOptions.group` is provided
- `ewm(alpha, statistic, adjusted, bias_corrected, min_count, ignore_nulls)`
- `expanding(statistic, options)`
- `zscore_rolling`, `fill_gaps`

Time-axis utilities:

- `resample(freq, op, step)` using `TsOptions.axis` or time case labels
- `asfreq(freq, step, fill_mode, constant, op)`
- `insert_gaps(freq, step)`
- `align_to(target_axis, mode)`
- `align_to(target_axis, mode, fill, constant)`

Feature builders:

- `calendar()`
- `lag_block({lags...})`
- `rolling_block({windows...}, options)`

## Example: End-to-End Feature Prep

```cpp
auto opts = TsOptions{.axis = df.features("date")};

auto lr = df.features({"Adj Close"}).ts().log_returns();
auto lag = df.features({"Adj Close", "Volume"}).ts().lag_block({1, 2, 3});
auto roll = df.features({"Adj Close", "Volume"}).ts().rolling_block({5, 20});
auto cal = df.features({"Adj Close"}).ts(opts).calendar();

auto daily = df.features({"Adj Close", "Volume"}).ts(opts).resample(
    "D", pluss::subsp::stats::ts::ResampleOp::Mean, 1);
```

## Notes

- Computation follows `Series` platform semantics.
- Missing alignment/reindex positions are represented as `NaN` in numeric outputs.
- For assignment to a new feature, `DataFrame::set(name)=DataFrame` requires the right-hand side to have one feature.
