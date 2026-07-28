<!-- pluss-hand-authored -->

# DataFrameTsAccessor::timed

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signatures

```cpp
DataFrameTsAccessor::timed(std::int64_t duration_nanos, TimeWindowStatistic statistic = TimeWindowStatistic::Mean, const StatisticOptions& options = {}, WindowClosure closure = WindowClosure::Right) const -> DataFrame
DataFrameTsAccessor::timed(const Thread& axis, std::int64_t duration_nanos, TimeWindowStatistic statistic = TimeWindowStatistic::Mean, const StatisticOptions& options = {}, WindowClosure closure = WindowClosure::Right) const -> DataFrame
```

## What It Does

Computes a **calendar-duration trailing window statistic** over every bound feature,
with the window width given as an exact nanosecond count rather than a duration
string — the same trailing-window idea as [`rolling()`](rolling.md)'s
`std::string_view duration` overload, but with exact nanosecond control and a
narrower statistic set (`Sum`/`Mean`/`Var`/`Std` only, via `TimeWindowStatistic`,
versus `rolling()`'s full `WindowStatistic`).

For each row, the window covers every other row whose axis tick falls within
`duration_nanos` of that row's own tick, according to `closure`: under the default
`Right` closure, the window is `(tick - duration_nanos, tick]` — the left boundary
tick itself (exactly `duration_nanos` in the past) is **excluded**, the current row
is **included**. `Left` flips which end is inclusive; `Both` includes both boundary
ticks; `Neither` excludes both.

The no-`axis` overload uses this accessor's own time axis (the frame's case labels,
or an explicit `TsOptions.axis`); the `axis` overload takes an explicit `Thread`
instead, useful when the desired time axis isn't the frame's own case labels. Groups
(`TsOptions.group`) are not supported here — use [`rolling(duration)`](rolling.md)
instead for grouped calendar-duration windows.

## Parameters

**duration_nanos** (`std::int64_t`)

The trailing window width, in nanoseconds. Must be positive.

**axis** (`const Thread&`, explicit-axis overload)

A `Timeline`, `TimeIndex`, or `Int64`-nanoseconds `Thread`, the same length as the
frame's case count, sorted ascending, with no missing values — used as the time axis
instead of this accessor's own.

**statistic** (`TimeWindowStatistic`, default `Mean`)

The statistic computed over each window: `Sum`, `Mean`, `Var`, or `Std`.

**options** (`const StatisticOptions&`)

- **`min_count`** (`std::int64_t`, default `1`): minimum non-null observations
  required for a row's output to be non-`NULL`.
- **`ddof`** (`std::int64_t`, default `1`): delta degrees of freedom for `Var`/`Std`.

**closure** (`WindowClosure`, default `Right`)

Which end(s) of each row's time window are inclusive: `Right` (current row
included, the boundary tick excluded), `Left` (boundary tick included, current row
excluded), `Both`, or `Neither`.

## Returns

`DataFrame`
    A new frame with each bound feature replaced by its trailing duration-window
    statistic, same case labels as this accessor's own (or `axis`'s) time axis.

## Examples

Example 1. A 2-day trailing sum, using the default `Right` closure.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"load", Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0, 25.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z",
        "2024-01-04T00:00:00Z", "2024-01-05T00:00:00Z"})));
std::cout << readings;

constexpr std::int64_t two_days = 2LL * 24 * 3600 * 1'000'000'000LL;
auto out = readings.ts().timed(two_days, TimeWindowStatistic::Sum);
std::cout << out;
```

### Output (before)

```text
==================
           | load
------------------
2024-01-01 | 10  
2024-01-02 | 20  
2024-01-03 | 15  
2024-01-04 | 30  
2024-01-05 | 25  
------------------
[5 cs x 1 feat]
```

### Output (after)

```text
==================
           | load
------------------
2024-01-01 | 10  
2024-01-02 | 30  
2024-01-03 | 35  
2024-01-04 | 45  
2024-01-05 | 55  
------------------
[5 cs x 1 feat]
```

`2024-01-01`'s window is just itself (`10`, nothing 2 days before it exists).
`2024-01-02`'s window is `(2023-12-31, 2024-01-02]`, covering `01-01` and `01-02`
(`10 + 20 = 30`). `2024-01-03`'s window is `(2024-01-01, 2024-01-03]` — `01-01`
itself is *excluded* (it sits exactly on the 2-day boundary, and `Right` closure
excludes the left boundary), leaving `01-02` and `01-03` (`20 + 15 = 35`).
`2024-01-04` covers `01-03`/`01-04` (`15 + 30 = 45`); `2024-01-05` covers
`01-04`/`01-05` (`30 + 25 = 55`).

Example 2. The same window with `closure = Left` — the boundary tick is included
instead of the current row's own tick contributing on the far side.

```cpp
auto out_left = readings.ts().timed(
    two_days, TimeWindowStatistic::Sum, StatisticOptions{}, WindowClosure::Left);
std::cout << out_left;
```

### Output

```text
==================
           | load
------------------
2024-01-01 | NULL
2024-01-02 | 10  
2024-01-03 | 30  
2024-01-04 | 20  
2024-01-05 | 15  
------------------
[5 cs x 1 feat]
```

Under `Left`, `2024-01-03`'s window is `[2024-01-01, 2024-01-03)` — `01-01` is now
*included* (the left boundary), but `01-03` itself is excluded (the current row is
no longer automatically in its own window under `Left`), leaving just `01-01` and
`01-02` (`10 + 20 = 30`). `2024-01-01` has no row strictly before its own boundary
and excludes itself, so with `min_count = 1` still unmet it's `NULL`.

## Notes

- `duration_nanos` must be a positive `std::int64_t`; nanosecond precision means a
  duration like 2 days must be spelled out as `2 * 24 * 3600 * 1'000'000'000LL`
  (or built from whatever duration utilities the calling code already has) — there is
  no duration-string parsing here, unlike [`rolling()`](rolling.md)'s
  `std::string_view duration` overload.
- `TsOptions.group` is not supported: passing a `timed()` call on a grouped accessor
  throws `"DataFrameTsAccessor::timed: TsOptions.group is not supported; use
  rolling(duration) for grouped temporal windows"`.
- The axis (this accessor's own case labels, `TsOptions.axis`, or the explicit
  `axis` argument) must be sorted ascending and free of missing values, or the call
  throws.
- `Var`/`Std` additionally require the window's observation count to exceed `ddof`
  (default `1`), independent of `min_count`.

## See Also

- [`DataFrameTsAccessor::rolling`](rolling.md): the duration-string sibling with a
  broader statistic set and grouped-window support.
- [`DataFrameTsAccessor::expanding`](expanding.md): a window that grows from the
  start of the series rather than a fixed calendar span.
- [`DataFrameTsAccessor::align_to`](align_to.md): reindexing onto an explicit time
  axis, the same axis concept used by this method's `axis` overload.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sub.html">
    <span class="label">Previous</span>
    <span class="title">sub</span>
  </a>
  <a class="next-link" href="weigh.html">
    <span class="label">Next</span>
    <span class="title">weigh</span>
  </a>
</div>
