# Part VIII: Time-Series Workflows

Time-series work in PlussMic is built from two ideas:

1. A time axis is data, usually a `Timeline` or `TimeIndex` stored in a
   `Slate`.
2. Time-series calculations are ordinary table transformations that return
   either a `Slate` or a `DataFrame`.

Use the Slate accessor when you are working on one numeric value stream:

```cpp
auto returns = df.features("close").ts().returns(1);
```

Use the DataFrame accessor when you want to transform one or more named
features and keep a table-shaped result:

```cpp
auto returns = df.features({"close", "volume"}).ts().returns(1);
```

Use DataFrame resampling and reindexing when the operation needs a time axis:

```cpp
auto values = df.labels().set("date", false).features({"close", "volume"});
auto daily = values.ts().resample("D").mean();
```

The accessors do not guess that a feature named `"date"` is time-like. For
cases-based resampling, the frame cases must actually be a `Timeline` or
`TimeIndex`; use `.tl()` for datetime columns and `.ts()` for numeric value
streams.

If a date column starts as text, parse it through explicit `cast(...)` first:

```cpp
auto parsed = df.cast({
    pluss::table::DataFrame::CastSpec{
        "date_text",
        pluss::table::SlateCastTarget::Timeline,
        pluss::table::SlateCastOptions{
            .format = "%Y-%m-%d",
            .invalid = pluss::table::ParseInvalid::Null}},
});
```

## The time data model

PlussMic represents time in three common ways:

| Representation | Meaning |
| --- | --- |
| `Timeline` | A sequence of timestamps. Use it for date/time columns. |
| `TimeIndex` | A timeline with index semantics. Use it when alignment and as-of operations matter. |
| Numeric `Series` | A numeric value stream measured over time, such as close price, volume, demand, or latency. |

Time-series value transforms require numeric `Series` slates. Time-axis
methods require a `Timeline` or `TimeIndex` slate.

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
auto with_time = df.cast({
    pluss::table::DataFrame::CastSpec{
        "date", pluss::table::SlateCastTarget::Timeline, {}},
});
```

The current source supports conversion of integer `YYYYMMDD` values through
`cast(..., SlateCastTarget::Timeline)`, which is useful for market-data files that store dates as
`19900102` instead of ISO strings.

## Sorting is your responsibility

Time-series calculations use row order unless the method explicitly resamples or
aligns by a time axis. PlussMic will not sort a frame for you before computing a
return, lag, rolling mean, or line plot.

For ordered calculations, sort first:

```cpp
auto sorted = df.arrange({DataFrame::ArrangeSpec{"date", true, true}});
auto returns = sorted.features({"close"}).ts().returns(1);
```

This matters in production. A return computed on unsorted prices is not a
slightly messy return; it is a different calculation.

## Slate time-series accessor

`Slate::ts()` creates a `TimeSeriesAccessor` for one numeric `Series` payload:

```cpp
auto close = df.features("close");
auto r = close.ts().returns(1);
```

The source slate must be defined, must be `SlateKind::Series`, and must be
numeric. If you call `ts()` on a `Note`, `Timeline`, or non-numeric payload, the
operation throws when you call the accessor method.

### Slate accessor methods

| Method | Return | Meaning |
| --- | --- | --- |
| `returns(periods)` | `Slate` | Fractional return over `periods`. |
| `log_returns(periods)` | `Slate` | Log return over `periods`. |
| `diff(periods)` | `Slate` | Difference from the value `periods` rows earlier. |
| `lag(periods)` | `Slate` | Shift values backward by `periods`; early rows become undefined numeric positions. |
| `lead(periods)` | `Slate` | Shift values forward by `periods`; trailing rows become undefined numeric positions. |
| `rolling(window, statistic, options)` | `Slate` | Observation-count rolling/window statistic. |
| `expanding(statistic, options)` | `Slate` | Expanding statistic. |
| `ewm(alpha, statistic, ...)` | `Slate` | Exponentially weighted statistic. |
| `zscore_rolling(window, min_periods, epsilon)` | `Slate` | Rolling z-score. |
| `fill_gaps(mode, constant)` | `Slate` | Fill numeric gaps according to a gap-fill mode. |
| `resample(axis, freq, op, step)` | `TimeSeriesResample` | Resample one value stream against a time axis. |

The return type is important. `Slate::ts()` methods return `Slate`, except
for `resample`, which returns a `TimeSeriesResample` with two fields:

| Field | Meaning |
|---|---|
| `labels` | The resampled time labels as a `Slate`. |
| `values` | The resampled values as a `Slate`. |

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
auto rolling_mean = close.ts().rolling(20, pluss::table::WindowStatistic::Mean, {.min_count = 10});
auto rolling_std = close.ts().rolling(20, pluss::table::WindowStatistic::Std, {.min_count = 10});
auto rolling_sum = close.ts().rolling(20, pluss::table::WindowStatistic::Sum, {.min_count = 10});
auto rolling_var = close.ts().rolling(20, pluss::table::WindowStatistic::Var, {.min_count = 10});
auto rolling_min = close.ts().rolling(20, pluss::table::WindowStatistic::Minimum, {.min_count = 10});
auto rolling_max = close.ts().rolling(20, pluss::table::WindowStatistic::Maximum, {.min_count = 10});
```

`window` is the number of rows in the rolling window. `min_periods` is the
minimum number of observations required before a value is emitted.

Use `min_periods = window` when you want strict full-window results:

```cpp
auto strict = close.ts().rolling(20, pluss::table::WindowStatistic::Mean, {.min_count = 20});
```

Use a smaller `min_periods` when early estimates are acceptable:

```cpp
auto warmup = close.ts().rolling(20, pluss::table::WindowStatistic::Mean, {.min_count = 5});
```

Rolling windows are observation-based, not calendar-based. If you need calendar
windows, resample first or construct the exact frame you want before rolling.

## Expanding and exponentially weighted metrics

Expanding metrics grow from the start of the series:

```cpp
auto expanding_mean = close.ts().expanding(pluss::table::WindowStatistic::Mean, {.min_count = 5});
auto expanding_std = close.ts().expanding(pluss::table::WindowStatistic::Std, {.min_count = 5});
```

Exponentially weighted metrics give more weight to recent values:

```cpp
auto ewm_mean = close.ts().ewm(0.1, pluss::table::WindowStatistic::Mean);
auto ewm_std = close.ts().ewm(0.1, pluss::table::WindowStatistic::Std);
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

The Slate and DataFrame accessors expose `fill_gaps`:

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
auto one = df.features({"close"}).ts();
auto many = df.features({"close", "volume"}).ts();
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
| `rolling(window, statistic, options)` | `DataFrame` | Observation-count rolling/window statistics for selected features. |
| `rolling(duration, statistic, options, closure)` | `DataFrame` | Time-window statistics over timeline cases, e.g. `"7D"`. |
| `expanding(statistic, options)` | `DataFrame` | Expanding statistics for selected features. |
| `ewm(alpha, statistic, ...)` | `DataFrame` | Exponentially weighted statistics for selected features. |
| `zscore_rolling(window, min_periods, epsilon)` | `DataFrame` | Rolling z-scores. |
| `fill_gaps(mode, constant)` | `DataFrame` | Fill numeric gaps in selected features. |
| `sort(ascending, nulls_last)` | `DataFrame` | Sort selected numeric features by timeline cases. |
| `resample(freq, step)` | `DataFrameTsResampler` | Build a cases-based resampler object, then choose `.mean()`, `.max()`, `.asfreq()`, `.ffill()`, and related actions. |
| `resample(freq, op, step)` | `DataFrame` | Aggregate selected features into cases-based time bins immediately. |
| `asfreq(freq, step, fill_mode, constant, op)` | `DataFrame` | Resample against timeline cases, then fill gaps. |
| `insert_gaps(freq, step)` | `DataFrame` | Align to regular timeline cases and insert gaps where exact observations are missing. |
| `align_to(target_axis, mode)` | `DataFrame` | Align selected features to a target time axis using timeline cases. |
| `reindex_time(target_axis, mode, fill, constant)` | `DataFrame` | Align to a target axis and optionally fill gaps. |
| `.tl()` calendar methods | `DataFrame` | Extract calendar features from datetime columns. |
| `lag_block(lags)` | `DataFrame` | Create multiple lagged features. |
| `rolling_block(windows, options)` | `DataFrame` | Create multiple rolling features. |

## Resampling

Resampling is selection-first. Put the timeline column on the case axis, then
select numeric value features and call `.ts()`.

Use the resampler object when you want time-series semantics closer to a
grouped pipeline:

```cpp
auto values = df.labels().set("date", false).features({"close", "volume"});
auto ordered = values.ts().sort();
auto seven_day = values.ts().rolling("7D", pluss::table::WindowStatistic::Mean);
auto daily = values.ts().resample("D").mean();
auto weekly_max = values.features({"close"}).ts().resample("W").max();
auto hourly = values.features({"close"}).ts().resample("H").asfreq();
auto hourly_filled = values.features({"close"}).ts().resample("H").ffill();
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
auto daily = df.features({"close", "volume"})
    .ts({.axis = df.features("date")})
    .resample(
    "D",
    pluss::subsp::stats::ts::ResampleOp::Mean,
    1);
```

The call returns a `DataFrame` whose cases are the resample labels. In the
verified example later in this chapter, four half-day observations become two
daily rows. Select value features first, then call `.ts()` with either time
case labels or `TsOptions{.axis = ...}` as the main time-series resampler
surface.

## As-frequency conversion

`asfreq` resamples and then fills gaps:

```cpp
auto hourly = df.features({"close"}).ts().asfreq(
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
auto with_gaps = df.labels().set("date", false).features({"close"}).ts().insert_gaps("H", 1);
```

Rows with no exact source observation are emitted as undefined numeric
positions. This is useful before plotting because it makes data outages visible
instead of connecting across missing intervals.

## Aligning and reindexing to a target axis

Use `align_to` when you already have the target timeline:

```cpp
auto aligned = df.labels().set("date", false).features({"close", "volume"}).ts().align_to(
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
auto reindexed = df.labels().set("date", false).features({"close"}).ts().reindex_time(
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
auto filled = df.labels().set("date", false).features({"close"}).ts().reindex_time(
    target_axis,
    pluss::table::TsAlignMode::Exact,
    pluss::table::TsReindexFill::Constant,
    100.0);
```

## Calendar features

`calendar(time_feature)` extracts useful calendar columns from a time axis:

```cpp
auto cal = df.features({"date"}).tl().year();
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

## Timeline spans

Use `.tl().span(...)` when you need elapsed calendar distances between timeline
values:

```cpp
auto start = pluss::table::Slate(pluss::make_timeline({
    "2024-01-31T00:00:00Z",
    "2024-02-29T00:00:00Z",
    "2024-01-01T00:00:00Z"}));

auto end = pluss::table::Slate(pluss::make_timeline({
    "2024-02-01T00:00:00Z",
    "2025-02-28T00:00:00Z",
    "2024-01-05T00:00:00Z"}));

auto days = start.tl().span(end, pluss::table::TimeUnit::Days);
```

`TimeUnit::Days` and smaller fixed units are elapsed UTC units. Calendar units
such as `Months`, `Years`, and `BusinessDays` require matching time zones,
because the answer depends on local calendar labels.

For month and year spans, choose the rule explicitly:

```cpp
pluss::table::TimelineSpanOptions months;
months.unit = pluss::table::TimeUnit::Months;
months.mode = pluss::table::TimelineSpanMode::Complete;

auto full_months = start.tl().span(end, months);

months.mode = pluss::table::TimelineSpanMode::Boundary;
auto crossed_month_labels = start.tl().span(end, months);
```

`TimelineSpanMode::Complete` counts fully completed calendar units. January 31
to February 1 is `0` complete months. `TimelineSpanMode::Boundary` counts crossed
calendar labels, so January 31 to February 1 crosses one month boundary.

The default completion anchor is strict day-of-month matching. If your domain
uses end-of-month accounting rules, opt in explicitly:

```cpp
months.mode = pluss::table::TimelineSpanMode::Complete;
months.anchor = pluss::table::TimelineSpanAnchor::MonthEnd;

auto accounting_months = start.tl().span(end, months);
```

With `MonthEnd`, January 31 to February 29 counts as one complete month, and
February 29 to February 28 in a non-leap year counts as one complete year when
the unit is `Years`.

Business-day spans use an explicit calendar:

```cpp
pluss::table::TimelineSpanOptions business;
business.unit = pluss::table::TimeUnit::BusinessDays;
business.closed = pluss::table::TimelineSpanClosed::Right; // (start, end]
business.calendar.name = "ExampleCorp";
business.calendar.region = "CA-AB";
business.calendar.holidays = {
    pluss::make_timeline("2024-01-03T00:00:00Z")
        .epoch_days()
        .host<std::int64_t>().array().data.at(static_cast<std::size_t>(0))};

auto business_days = start.tl().span(end, business);
```

Use a built-in preset when your domain matches one of the supplied calendars:

```cpp
business.calendar = pluss::table::BusinessCalendar::monfri();
business.calendar = pluss::table::BusinessCalendar::usa_federal(2026, 2026);
business.calendar = pluss::table::BusinessCalendar::canada_federal(2026, 2026);
business.calendar = pluss::table::BusinessCalendar::uk_bank(2026, 2026);
business.calendar = pluss::table::BusinessCalendar::target2(2026, 2026);
business.calendar = pluss::table::BusinessCalendar::nyse(2026, 2026);
```

Built-in presets intentionally stop at broad federal/national/market calendars.
Provincial, territorial, state, municipal, and employer-specific calendars can
be expressed by constructing a `BusinessCalendar` with custom holidays. This
keeps the public API from growing a preset for every local jurisdiction.
`uk_bank` currently means England/Wales bank holidays. `target2` means Euro
TARGET closing days. `nyse` means New York Stock Exchange full-day closures; it
is not the same as U.S. federal holidays.

The default business calendar is Monday through Friday, with no holidays. The
default interval is `(start, end]`: exclude the start date, include the end date.
Set `closed` to `Left`, `Both`, or `Neither` when your domain needs a different
convention. Holidays are local epoch-day labels, so generate them in the same
timezone as the timeline being measured. If `BusinessCalendar::timezone` is
provided, it must match the timeline timezone. Calendar and business spans also
require the left and right timelines to have matching timezones; convert first
when necessary.

## Lag blocks

`lag_block` creates several lagged features at once:

```cpp
auto lags = df.features({"close", "volume"}).ts().lag_block({1, 2, 5});
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

auto block = df.features({"close", "volume"}).ts().rolling_block({5, 20}, options);
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
          << df.features("close").ts().rolling(2, pluss::table::WindowStatistic::Mean, {.min_count = 2}).to_string()
          << '\n';

auto values = df.labels().set("date", false).features({"close", "volume"});
auto daily = values.ts().resample("D", pluss::subsp::stats::ts::ResampleOp::Mean);
std::cout << "daily mean:\n" << daily.to_string() << '\n';

auto calendar = df.features({"date"}).tl().year();
std::cout << "calendar:\n"
          << calendar.features({"date_year", "date_month",
                                "date_weekday"}).to_string() << '\n';

auto lags = df.features({"close", "volume"}).ts().lag_block({1, 2});
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
ptype: Series
rolling mean window 2:
0  nan
1  102.5
2  104
3  106
ptype: Series
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
| Several transformed value streams | `df.features({"x", "y"}).ts()` |
| Resample one value stream and handle labels manually | `Slate::ts().resample(axis, ...)` |
| Resample several value streams into a new frame | `df.labels().set("date").features({...}).ts().resample(...)` |
| Add calendar model features | `df.features({"date"}).tl().year()` |
| Build lagged model features | `df.features({...}).ts().lag_block(...)` |
| Build rolling model features | `df.features({...}).ts().rolling_block(...)` |
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

This returns a `Slate`:

```cpp
auto out = df.features("close").ts().returns(1);
```

This returns a `DataFrame`:

```cpp
auto out = df.features({"close"}).ts().returns(1);
```

Choose based on the return shape you need.

### Treating row periods as calendar periods

This is a one-row return:

```cpp
auto out = df.features({"close"}).ts().returns(1);
```

It is not necessarily a one-day return. If the frame has hourly rows, it is a
one-hour return. If the frame has irregular rows, it is a one-observation
return. Resample first when the calendar interval matters.

### Resampling a non-time feature

This fails when `"date"` is not a `Timeline` or `TimeIndex`:

```cpp
auto daily = df.labels().set("date", false).features({"close"}).ts().resample("D").mean();
```

Fix the schema first:

```cpp
auto fixed = df.cast({
    pluss::table::DataFrame::CastSpec{
        "date", pluss::table::SlateCastTarget::Timeline, {}},
});
auto daily = fixed.labels().set("date", false).features({"close"}).ts().resample("D").mean();
```

### Rolling unsorted data

This compiles but can be wrong:

```cpp
auto rolling = df.features({"close"}).ts().rolling(20, pluss::table::WindowStatistic::Mean, {.min_count = 20});
```

If `df` is not sorted by time, the rolling window follows row order. Sort first:

```cpp
auto sorted = df.arrange({DataFrame::ArrangeSpec{"date", true, true}});
auto rolling = sorted.features({"close"}).ts().rolling(20, pluss::table::WindowStatistic::Mean, {.min_count = 20});
```
