<!-- pluss-hand-authored -->

# TimeSeriesAccessor::timed

[Back to TimeSeriesAccessor API Reference](../../thread_ts_accessor_api_reference)

## Signature

```cpp
TimeSeriesAccessor::timed(const Thread& axis, std::int64_t duration_nanos, TimeWindowStatistic statistic = TimeWindowStatistic::Mean, const StatisticOptions& options = {}, WindowClosure closure = WindowClosure::Right) const -> Thread
```

## What It Does

Computes a **calendar-duration trailing window statistic**: each position's window
is every position whose `axis` tick falls within `duration_nanos` nanoseconds of
that position's own tick (per `closure`), rather than a fixed row count. `axis`
can be a `Timeline`, a `TimeIndex`, or a plain `Int64` `Series` of nanosecond
ticks; it must be the same length as the bound thread, contain no missing
entries, and be sorted ascending (not necessarily evenly spaced — the window
width is a genuine span of time, so it naturally covers a different *number* of
positions at each row when ticks are irregular).

`closure` controls which end of each `[tick - duration_nanos, tick]` span is
included:

- **`Right`** (the default) and **`Both`**: the position's own tick is always
  included in its own window (trivially, since it's exactly `0` away from
  itself).
- **`Left`** and **`Neither`**: the position's own tick is **excluded** from its
  own window entirely — the window only looks strictly before the current
  position.

Only `Sum`, `Mean`, `Var`, and `Std` are supported.

## Parameters

**axis** (`const Thread&`)
    A `Timeline`, `TimeIndex`, or `Int64`-nanoseconds `Series`, the same length
    as the bound thread, non-missing and sorted ascending.

**duration_nanos** (`std::int64_t`)
    The window width, in nanoseconds. Must be positive.

**statistic** (`TimeWindowStatistic`, default `Mean`)
    `Sum`, `Mean`, `Var`, or `Std`.

**options** (`const StatisticOptions&`)
    `ddof` (default `1`): delta degrees of freedom for `Var`/`Std`. `min_count`
    (default `1`): minimum non-missing observations required for a position's
    output to be non-`NULL`.

**closure** (`WindowClosure`, default `Right`)
    Which end(s) of each duration window are inclusive.

## Returns

`Thread`
    A new thread, same length as the bound source, holding the duration-window
    statistic at each position.

## Examples

Example 1. An irregular tick axis — the window naturally covers a different
number of positions at each row, since it's a genuine span of time rather than a
fixed count.

```cpp
using namespace pluss::table;

// Ticks at 0s, 1s, 2s, 4s, 7s (as whole-second nanosecond ticks).
auto axis = Thread(pluss::make_series<std::int64_t>(
    {0, 1'000'000'000, 2'000'000'000, 4'000'000'000, 7'000'000'000}));
auto readings = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = readings.ts().timed(axis, 3'000'000'000, TimeWindowStatistic::Mean);
std::cout << out;
```

### Output

```text
0  10
1  15
2  20
3  35
4  50

ptype: Series
```

With a 3-second window and the default right-closed boundary, position `2`
(tick `2s`) looks back to everything within `3s` — that's ticks `0s`, `1s`, and
`2s` itself, so its window is `(10, 20, 30)` → mean `20`. Position `3` (tick
`4s`) only reaches back to tick `1s` (`4s - 3s = 1s`, and `1s` is excluded since
it's exactly at the boundary under right-closure) — its window is
`(30, 40)` → mean `35`. Position `4` (tick `7s`) only reaches back to tick `4s`
(exactly at the boundary, excluded) — its window is just `(50)` → mean `50`.

Example 2. `closure = Left` excludes each position's own tick from its own
window — the window only looks strictly before the current position.

```cpp
auto out2 = readings.ts().timed(
    axis, 3'000'000'000, TimeWindowStatistic::Mean, {}, WindowClosure::Left);
std::cout << out2;
```

### Output

```text
0  NULL
1  10
2  15
3  25
4  40

ptype: Series
```

Position `0` has nothing before it at all, so its window is empty and the result
is `NULL` (below the default `min_count = 1`). Position `1`'s window is just tick
`0s` (`10`). Position `3`'s window is ticks `1s` and `2s` (`20, 30` → `25`) —
tick `4s` itself (the current position) is excluded this time, unlike Example 1.

Example 3. A non-monotonic axis throws.

```cpp
auto bad_axis = Thread(pluss::make_series<std::int64_t>({0, 2'000'000'000, 1'000'000'000}));
auto values = Thread(pluss::make_series({10.0, 20.0, 30.0}));
try {
  auto bad = values.ts().timed(bad_axis, 1'000'000'000);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::timed: axis must be monotonic
```

## Notes

- With `closure = Right` (the default) or `Both`, a position's own tick is
  always part of its own window. With `Left` or `Neither`, it never is — the
  window only looks strictly before the current position.
- `axis` must be non-missing and sorted ascending, or it throws
  `"Thread::timed: axis cannot contain null values"` /
  `"Thread::timed: axis must be monotonic"` respectively.
- `axis` must be `Timeline`, `TimeIndex`, or an `Int64` nanoseconds `Series`, or
  it throws `"Thread::timed: axis must be Timeline, TimeIndex, or Int64
  nanoseconds"`.
- `duration_nanos` must be `> 0`, or it throws
  `"Thread::timed: duration_nanos must be > 0"`.
- Only `Sum`, `Mean`, `Var`, and `Std` are supported (unlike the fixed-window
  [`rolling()`](rolling.md), which also supports `Minimum`/`Maximum`/`Sem`/
  `Skew`/`Kurtosis`).
- A pre-existing missing entry in the bound thread (not the axis) is skipped
  within any window that contains it, the same as [`rolling()`](rolling.md).

## See Also

- [`TimeSeriesAccessor::rolling`](rolling.md): the calendar-duration overload of
  `rolling()` delegates straight to this method, restricting `axis` to
  `Timeline`/`TimeIndex` and parsing `duration` from a string like `"2D"`.
- [`TimeSeriesAccessor::grouped`](grouped.md): a fixed-row-count window computed
  independently per group, rather than a duration-based window.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sub.html">
    <span class="label">Previous</span>
    <span class="title">sub</span>
  </a>
  <a class="next-link" href="zscore_rolling.html">
    <span class="label">Next</span>
    <span class="title">zscore_rolling</span>
  </a>
</div>
