# Part VIII: Time-Series Workflows

Time-series work in PlussMic is built from two ideas:

1. A time axis is data, usually a `Timeline` or `TimeIndex` stored in a
   `Thread`.
2. Time-series calculations are ordinary table transformations that return
   either a `Thread` or a `DataFrame`.

Use the Thread accessor when you are working on one numeric value stream:

```cpp
auto returns = df.features("close").ts().returns(1);
```

Use the DataFrame accessor when you want to transform one or more named
features and keep a table-shaped result:

```cpp
auto returns = df.ts({"close", "volume"}).returns(1);
```

Use DataFrame resampling and reindexing when the operation needs a time axis:

```cpp
auto daily = df.ts({"close", "volume"}).resample(
    "date", "D", pluss::subsp::stats::ts::ResampleOp::Mean);
```

The accessors do not guess that a feature named `"date"` is time-like. The
feature must actually be a `Timeline` or `TimeIndex` for time-axis methods.

## The time data model

PlussMic represents time in three common ways:

| Representation | Meaning |
| --- | --- |
| `Timeline` | A sequence of timestamps. Use it for date/time columns. |
| `TimeIndex` | A timeline with index semantics. Use it when alignment and as-of operations matter. |
| Numeric `Series` | A numeric value stream measured over time, such as close price, volume, demand, or latency. |

Time-series value transforms require numeric `Series` threads. Time-axis
methods require a `Timeline` or `TimeIndex` thread.

This split is deliberate. A price column and a timestamp column have different
semantics even though both can be represented internally as arrays.

## Creating time features

When you construct a frame in C++, use `pluss::make_timeline`:

```cpp
DataFrame df({
    {"date", pluss::make_timeline({
        "2024-01-01T00:00:00Z",
        "2024-01-01T12:00:00Z",
        "2024-01-02T00:00:00Z"})},
    {"close", pluss::make_series({100.0, 105.0, 103.0})},
});
```

When you load from CSV, parse or cast time columns early:

```cpp
pluss::loader::ReadCsvOptions options;
options.parse_dates = {"date"};

auto df = DataFrame::load_csv("prices.csv", options);
```

If a column loaded as text or as a numeric `YYYYMMDD` series, cast it:

```cpp
auto with_time = df.cast_timeline({"date"});
```

The current source supports conversion of integer `YYYYMMDD` values through
`cast_timeline`, which is useful for market-data files that store dates as
`19900102` instead of ISO strings.

## Sorting is your responsibility

Time-series calculations use row order unless the method explicitly resamples or
aligns by a time axis. PlussMic will not sort a frame for you before computing a
return, lag, rolling mean, or line plot.

For ordered calculations, sort first:

```cpp
auto sorted = df.sort_values({DataFrame::ArrangeSpec{"date", true, true}});
auto returns = sorted.ts("close").returns(1);
```

This matters in production. A return computed on unsorted prices is not a
slightly messy return; it is a different calculation.

## Thread time-series accessor

`Thread::ts()` creates a `TimeSeriesAccessor` for one numeric `Series` payload:

```cpp
auto close = df.features("close");
auto r = close.ts().returns(1);
```

The source thread must be defined, must be `ThreadKind::Series`, and must be
numeric. If you call `ts()` on a `Note`, `Timeline`, or non-numeric payload, the
operation throws when you call the accessor method.

### Thread accessor methods

| Method | Return | Meaning |
| --- | --- | --- |
| `returns(periods)` | `Thread` | Fractional return over `periods`. |
| `log_returns(periods)` | `Thread` | Log return over `periods`. |
| `diff(periods)` | `Thread` | Difference from the value `periods` rows earlier. |
| `lag(periods)` | `Thread` | Shift values backward by `periods`; early rows become undefined numeric positions. |
| `lead(periods)` | `Thread` | Shift values forward by `periods`; trailing rows become undefined numeric positions. |
| `rolling_mean(window, min_periods)` | `Thread` | Rolling mean. |
| `rolling_std(window, min_periods)` | `Thread` | Rolling standard deviation. |
| `rolling_sum(window, min_periods)` | `Thread` | Rolling sum. |
| `rolling_var(window, min_periods)` | `Thread` | Rolling variance. |
| `rolling_min(window, min_periods)` | `Thread` | Rolling minimum. |
| `rolling_max(window, min_periods)` | `Thread` | Rolling maximum. |
| `ema(alpha)` | `Thread` | Exponential moving average. |
| `ewm_mean(alpha)` | `Thread` | Exponentially weighted mean. |
| `ewm_std(alpha, epsilon)` | `Thread` | Exponentially weighted standard deviation. |
| `expanding_mean(min_periods)` | `Thread` | Expanding mean. |
| `expanding_std(min_periods, epsilon)` | `Thread` | Expanding standard deviation. |
| `zscore_rolling(window, min_periods, epsilon)` | `Thread` | Rolling z-score. |
| `fill_gaps(mode, constant)` | `Thread` | Fill numeric gaps according to a gap-fill mode. |
| `resample(axis, freq, op, step)` | `TimeSeriesResample` | Resample one value stream against a time axis. |

The return type is important. `Thread::ts()` methods return `Thread`, except
for `resample`, which returns:

```cpp
struct TimeSeriesResample {
  Thread labels;
  Thread values;
};
```

Use this form when you do not need a full DataFrame:

```cpp
auto result = df.features("close").ts().resample(
    df.features("date"),
    "D",
    pluss::subsp::stats::ts::ResampleOp::Last);

auto labels = result.labels;
auto values = result.values;
```

## Returns, differences, lag, and lead

The simplest time-series methods operate on row order:

```cpp
auto close = df.features("close");

auto simple_returns = close.ts().returns(1);
auto log_returns = close.ts().log_returns(1);
auto one_row_diff = close.ts().diff(1);
auto lagged = close.ts().lag(1);
auto led = close.ts().lead(1);
```

The `periods` argument is a row count. It is not a calendar duration. For daily
data, `periods = 1` usually means one trading day or one calendar day depending
on your rows. For intraday data, `periods = 1` means one observation.

Current numeric time-series transforms use floating `nan` for positions that
cannot be computed, such as the first one-period return or the first one-period
lag. Do not document or test those positions as table `NA` unless you have
explicitly converted them to a validity-mask representation.

## Rolling windows

Rolling methods use observation windows:

```cpp
auto rolling_mean = close.ts().rolling_mean(20, 10);
auto rolling_std = close.ts().rolling_std(20, 10);
auto rolling_sum = close.ts().rolling_sum(20, 10);
auto rolling_var = close.ts().rolling_var(20, 10);
auto rolling_min = close.ts().rolling_min(20, 10);
auto rolling_max = close.ts().rolling_max(20, 10);
```

`window` is the number of rows in the rolling window. `min_periods` is the
minimum number of observations required before a value is emitted.

Use `min_periods = window` when you want strict full-window results:

```cpp
auto strict = close.ts().rolling_mean(20, 20);
```

Use a smaller `min_periods` when early estimates are acceptable:

```cpp
auto warmup = close.ts().rolling_mean(20, 5);
```

Rolling windows are observation-based, not calendar-based. If you need calendar
windows, resample first or construct the exact frame you want before rolling.

## Expanding and exponentially weighted metrics

Expanding metrics grow from the start of the series:

```cpp
auto expanding_mean = close.ts().expanding_mean(5);
auto expanding_std = close.ts().expanding_std(5);
```

Exponentially weighted metrics give more weight to recent values:

```cpp
auto ema = close.ts().ema(0.1);
auto ewm_mean = close.ts().ewm_mean(0.1);
auto ewm_std = close.ts().ewm_std(0.1);
```

`alpha` controls responsiveness. Values closer to `1.0` react faster. Values
closer to `0.0` are smoother.

## Rolling z-scores

Rolling z-scores are useful for anomaly detection and normalized signals:

```cpp
auto z = close.ts().zscore_rolling(20, 10, 1e-12);
```

The `epsilon` argument protects divisions by very small standard deviations.
Use a domain-appropriate value if your data scale is unusual.

## Gap filling

The Thread and DataFrame accessors expose `fill_gaps`:

```cpp
auto forward = close.ts().fill_gaps(
    pluss::subsp::stats::ts::GapFillMode::Forward);

auto zeros = close.ts().fill_gaps(
    pluss::subsp::stats::ts::GapFillMode::Zero);

auto constant = close.ts().fill_gaps(
    pluss::subsp::stats::ts::GapFillMode::Constant, 100.0);
```

Choose the fill mode based on the domain:

- Forward fill can be appropriate for last-observation-carried-forward prices
  or states.
- Backward fill can be useful for planned schedules where the next known value
  should apply.
- Zero fill is appropriate only when missing means zero.
- Constant fill should be a deliberate modeling choice.

## DataFrame time-series accessor

`DataFrame::ts(...)` creates a `DataFrameTsAccessor` for one or more selected
numeric features:

```cpp
auto one = df.ts("close");
auto many = df.ts({"close", "volume"});
```

The selected feature names must exist and must be unique. Most accessor methods
return a DataFrame whose cases come from the source frame. Resampling and
reindexing methods return a DataFrame whose cases come from the time labels or
target axis.

### DataFrame accessor methods

| Method | Return | Meaning |
| --- | --- | --- |
| `returns(periods)` | `DataFrame` | Returns for selected features. |
| `log_returns(periods)` | `DataFrame` | Log returns for selected features. |
| `diff(periods)` | `DataFrame` | Differences for selected features. |
| `lag(periods)` | `DataFrame` | Lag selected features. |
| `lead(periods)` | `DataFrame` | Lead selected features. |
| `rolling_mean(window, min_periods)` | `DataFrame` | Rolling means. |
| `rolling_std(window, min_periods)` | `DataFrame` | Rolling standard deviations. |
| `rolling_sum(window, min_periods)` | `DataFrame` | Rolling sums. |
| `rolling_var(window, min_periods)` | `DataFrame` | Rolling variances. |
| `rolling_min(window, min_periods)` | `DataFrame` | Rolling minimums. |
| `rolling_max(window, min_periods)` | `DataFrame` | Rolling maximums. |
| `ema(alpha)` | `DataFrame` | Exponential moving averages. |
| `ewm_mean(alpha)` | `DataFrame` | Exponentially weighted means. |
| `ewm_std(alpha, epsilon)` | `DataFrame` | Exponentially weighted standard deviations. |
| `expanding_mean(min_periods)` | `DataFrame` | Expanding means. |
| `expanding_std(min_periods, epsilon)` | `DataFrame` | Expanding standard deviations. |
| `zscore_rolling(window, min_periods, epsilon)` | `DataFrame` | Rolling z-scores. |
| `fill_gaps(mode, constant)` | `DataFrame` | Fill numeric gaps in selected features. |
| `resample(time_feature, freq, step)` | `DataFrameTsResampler` | Build a time-series resampler object, then choose `.mean()`, `.max()`, `.asfreq()`, `.ffill()`, and related actions. |
| `resample(time_feature, freq, op, step)` | `DataFrame` | Aggregate selected features into time bins immediately. |
| `asfreq(time_feature, freq, step, fill_mode, constant, op)` | `DataFrame` | Resample, then fill gaps. |
| `insert_gaps(time_feature, freq, step)` | `DataFrame` | Align to regular bins and insert gaps where exact observations are missing. |
| `align_to(time_feature, target_axis, mode)` | `DataFrame` | Align selected features to a target time axis. |
| `reindex_time(time_feature, target_axis, mode, fill, constant)` | `DataFrame` | Align to a target axis and optionally fill gaps. |
| `calendar(time_feature)` | `DataFrame` | Extract calendar features from a time axis. |
| `lag_block(lags)` | `DataFrame` | Create multiple lagged features. |
| `rolling_block(windows, options)` | `DataFrame` | Create multiple rolling features. |

## Resampling

Resampling now has two surfaces on `df.ts(...)`.

Use the resampler object when you want time-series semantics closer to a
grouped pipeline:

```cpp
auto daily = df.ts({"close", "volume"}).resample("date", "D").mean();
auto weekly_max = df.ts("close").resample("date", "W").max();
auto hourly = df.ts("close").resample("date", "H").asfreq();
auto hourly_filled = df.ts("close").resample("date", "H").ffill();
```

Arguments:

| Argument | Meaning |
| --- | --- |
| `time_feature` | Name of a `Timeline` or `TimeIndex` feature. |
| `freq` | Frequency key, such as `"D"` for daily bins. |
| `step` | Positive frequency step. |

The resampler object supports these materializers today:

- `.mean()`
- `.sum()`
- `.min()`
- `.max()`
- `.first()`
- `.last()`
- `.count()`
- `.asfreq(fill_value = std::nullopt)`
- `.ffill()`
- `.bfill()`
- `.nearest()`

When you already know the aggregation at the call site, the immediate overload
still exists:

```cpp
auto daily = df.ts({"close", "volume"}).resample(
    "date",
    "D",
    pluss::subsp::stats::ts::ResampleOp::Mean,
    1);
```

Both forms produce a `DataFrame` whose cases are the resample labels. In the
verified example later in this chapter, four half-day observations become two
daily rows.

The older `DataFrame::resample(time_feature, value_features, freq, op, step,
render)` API still exists and returns a `DataFrameView`. That older helper is a
frame-level bin-and-aggregate call. Prefer `df.ts(features).resample(...)` when
you want the main time-series resampler surface.

## As-frequency conversion

`asfreq` resamples and then fills gaps:

```cpp
auto hourly = df.ts("close").asfreq(
    "date",
    "H",
    1,
    pluss::subsp::stats::ts::GapFillMode::Forward,
    std::nullopt,
    pluss::subsp::stats::ts::ResampleOp::Last);
```

Use it when your downstream code expects a regular time grid.

## Inserting gaps

`insert_gaps` builds regular bins and aligns exact observations onto them:

```cpp
auto with_gaps = df.ts("close").insert_gaps("date", "H", 1);
```

Rows with no exact source observation are emitted as undefined numeric
positions. This is useful before plotting because it makes data outages visible
instead of connecting across missing intervals.

## Aligning and reindexing to a target axis

Use `align_to` when you already have the target timeline:

```cpp
auto aligned = df.ts({"close", "volume"}).align_to(
    "date",
    target_axis,
    pluss::table::TsAlignMode::Exact);
```

`TsAlignMode` controls how target timestamps map to source timestamps:

| Mode | Meaning |
| --- | --- |
| `Exact` | Only exact timestamp matches are valid. |
| `Backward` | Use the nearest source timestamp at or before the target. |
| `Forward` | Use the nearest source timestamp at or after the target. |
| `Nearest` | Use the nearest source timestamp in either direction. |

Use `reindex_time` when you also want to fill missing aligned values:

```cpp
auto reindexed = df.ts("close").reindex_time(
    "date",
    target_axis,
    pluss::table::TsAlignMode::Exact,
    pluss::table::TsReindexFill::Forward);
```

`TsReindexFill` controls post-alignment filling:

| Fill | Meaning |
| --- | --- |
| `None` | Keep missing aligned values. |
| `Forward` | Forward-fill aligned gaps. |
| `Backward` | Backward-fill aligned gaps. |
| `Zero` | Fill aligned gaps with zero. |
| `Constant` | Fill aligned gaps with the supplied constant. |

If you use `Constant`, pass the constant value:

```cpp
auto filled = df.ts("close").reindex_time(
    "date",
    target_axis,
    pluss::table::TsAlignMode::Exact,
    pluss::table::TsReindexFill::Constant,
    100.0);
```

## Calendar features

`calendar(time_feature)` extracts useful calendar columns from a time axis:

```cpp
auto cal = df.ts("close").calendar("date");
```

It returns:

- `date_year`
- `date_month`
- `date_day`
- `date_hour`
- `date_weekday`

The prefix comes from the time feature name. For a feature named `"timestamp"`,
the output columns are `timestamp_year`, `timestamp_month`, and so on.

Calendar features are often used for grouping, joins, or model features:

```cpp
auto enriched = df.combine(cal, {DataFrame::CombineOp::Columns, "_cal", true});
```

## Lag blocks

`lag_block` creates several lagged features at once:

```cpp
auto lags = df.ts({"close", "volume"}).lag_block({1, 2, 5});
```

For features `close` and `volume`, that emits:

- `close_lag1`
- `close_lag2`
- `close_lag5`
- `volume_lag1`
- `volume_lag2`
- `volume_lag5`

Use lag blocks for supervised learning datasets, event studies, or feature
engineering.

## Rolling blocks

`rolling_block` creates many rolling features in one pass:

```cpp
pluss::table::TsRollingBlockOptions options;
options.include_mean = true;
options.include_std = true;
options.include_min = true;
options.include_max = true;
options.min_periods = 5;

auto block = df.ts({"close", "volume"}).rolling_block({5, 20}, options);
```

`TsRollingBlockOptions` contains:

| Option | Default | Meaning |
| --- | --- | --- |
| `include_mean` | `true` | Emit rolling mean features. |
| `include_std` | `true` | Emit rolling standard deviation features. |
| `include_min` | `false` | Emit rolling minimum features. |
| `include_max` | `false` | Emit rolling maximum features. |
| `include_sum` | `false` | Emit rolling sum features. |
| `include_var` | `false` | Emit rolling variance features. |
| `min_periods` | `1` | Minimum observations required for each rolling value. |

The output naming convention is:

```text
<feature>_roll_mean_w<window>
<feature>_roll_std_w<window>
<feature>_roll_min_w<window>
<feature>_roll_max_w<window>
<feature>_roll_sum_w<window>
<feature>_roll_var_w<window>
```

For example, `close` with window `20` can emit
`close_roll_mean_w20` and `close_roll_std_w20`.

## Verified example: returns, resampling, calendar, and lag blocks

This example is compiled and run by the manual example project:

```cpp
DataFrame df({
    {"date", pluss::make_timeline({
        "2024-01-01T00:00:00Z",
        "2024-01-01T12:00:00Z",
        "2024-01-02T00:00:00Z",
        "2024-01-02T12:00:00Z"})},
    {"close", pluss::make_series({100.0, 105.0, 103.0, 109.0})},
    {"volume", pluss::make_series({10.0, 20.0, 30.0, 40.0})},
});

std::cout << "returns:\n"
          << df.features("close").ts().returns(1).to_string() << '\n';
std::cout << "rolling mean window 2:\n"
          << df.features("close").ts().rolling_mean(2, 2).to_string()
          << '\n';

auto daily = df.ts({"close", "volume"}).resample(
    "date", "D", pluss::subsp::stats::ts::ResampleOp::Mean);
std::cout << "daily mean:\n" << daily.to_string() << '\n';

auto calendar = df.ts("close").calendar("date");
std::cout << "calendar:\n"
          << calendar.features({"date_year", "date_month",
                                "date_weekday"}).to_string() << '\n';

auto lags = df.ts({"close", "volume"}).lag_block({1, 2});
std::cout << "lag block:\n" << lags.to_string() << '\n';
```

The verified output is:

```text
=== 16 time series ===
returns:
0  nan
1  0.05
2  -0.01904761905
3  0.05825242718
dtype: Series
rolling mean window 2:
0  nan
1  102.5
2  104
3  106
dtype: Series
daily mean:
==========================
           | close  volume
--------------------------
2024-01-01 | 102.5  15
2024-01-02 | 106    35
--------------------------
[2 cs x 2 feat]
calendar:
==============================================
     | date_year  date_month  date_weekday
----------------------------------------------
0    | 2024       1           2024-01-01 (Mon)
1    | 2024       1           2024-01-01 (Mon)
2    | 2024       1           2024-01-02 (Tue)
3    | 2024       1           2024-01-02 (Tue)
----------------------------------------------
[4 cs x 3 feat]
lag block:
=======================================================
     | close_lag1  close_lag2  volume_lag1  volume_lag2
-------------------------------------------------------
0    | nan         nan         nan          nan
1    | 100         nan         10           nan
2    | 105         100         20           10
3    | 103         105         30           20
-------------------------------------------------------
[4 cs x 4 feat]
```

The output demonstrates several important details:

- The first one-period return is `nan` because no previous value exists.
- The strict two-row rolling mean is `nan` for the first row because
  `min_periods = 2`.
- Daily resampling uses the resample labels as DataFrame cases.
- Calendar extraction keeps the original case count.
- Lag blocks create one output feature for each selected feature and lag.

## Choosing the right API

| Need | Use |
| --- | --- |
| One transformed value stream | `df.features("x").ts()` |
| Several transformed value streams | `df.ts({"x", "y"})` |
| Resample one value stream and handle labels manually | `Thread::ts().resample(axis, ...)` |
| Resample several value streams into a new frame | `DataFrame::ts(features).resample(...)` |
| Add calendar model features | `DataFrame::ts(feature).calendar(time_feature)` |
| Build lagged model features | `DataFrame::ts(features).lag_block(...)` |
| Build rolling model features | `DataFrame::ts(features).rolling_block(...)` |
| Align data to an existing schedule | `align_to` or `reindex_time` |
| Make missing intervals visible | `insert_gaps` |

## Production checklist

Before using a time-series result in a report, model, or downstream pipeline:

- Confirm the time feature is `Timeline` or `TimeIndex`.
- Sort by the time feature before order-dependent calculations.
- Know whether `periods` means rows, not calendar units.
- Choose `min_periods` deliberately.
- Decide whether early undefined numeric positions should remain `nan` or be
  converted to table missingness.
- Use resampling when observations need calendar bins.
- Use reindexing when outputs must match a target schedule.
- Keep the original time feature available unless the resampled labels have
  replaced it as cases.
- Validate output row counts after resampling or reindexing.

## Common mistakes

### Calling the wrong accessor

This returns a `Thread`:

```cpp
auto out = df.features("close").ts().returns(1);
```

This returns a `DataFrame`:

```cpp
auto out = df.ts("close").returns(1);
```

Choose based on the return shape you need.

### Treating row periods as calendar periods

This is a one-row return:

```cpp
auto out = df.ts("close").returns(1);
```

It is not necessarily a one-day return. If the frame has hourly rows, it is a
one-hour return. If the frame has irregular rows, it is a one-observation
return. Resample first when the calendar interval matters.

### Resampling a non-time feature

This fails when `"date"` is not a `Timeline` or `TimeIndex`:

```cpp
auto daily = df.ts("close").resample("date", "D").mean();
```

Fix the schema first:

```cpp
auto fixed = df.cast_timeline({"date"});
auto daily = fixed.ts("close").resample("date", "D").mean();
```

### Rolling unsorted data

This compiles but can be wrong:

```cpp
auto rolling = df.ts("close").rolling_mean(20, 20);
```

If `df` is not sorted by time, the rolling window follows row order. Sort first:

```cpp
auto sorted = df.sort_values({DataFrame::ArrangeSpec{"date", true, true}});
auto rolling = sorted.ts("close").rolling_mean(20, 20);
```
