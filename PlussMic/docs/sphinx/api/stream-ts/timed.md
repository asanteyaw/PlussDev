<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5ad43ca84601 -->

# StreamTimeSeriesAccessor::timed

[Back to StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)

## Signature

```cpp
Stream StreamTimeSeriesAccessor::timed(
    const Stream& axis, std::int64_t duration_nanos,
    pluss::table::TimeWindowStatistic statistic = pluss::table::TimeWindowStatistic::Mean,
    const pluss::table::StatisticOptions& options = {},
    pluss::table::WindowClosure closure = pluss::table::WindowClosure::Right) const
```

## What It Does

Computes a **calendar-duration trailing window statistic**: each position's
window is every position whose `axis` tick falls within `duration_nanos`
nanoseconds of that position's own tick (per `closure`), rather than a fixed
row count. `axis` can be a `Timeline`, a `TimeIndex`, or a plain `Int64`
`Series` of nanosecond ticks; it must be the same length as the bound stream,
contain no missing entries, and be sorted ascending.

`closure` controls which end of each `[tick - duration_nanos, tick]` span is
included: `Right`/`Both` always include the position's own tick; `Left`/
`Neither` exclude it.

Only `Sum`, `Mean`, `Var`, and `Std` are supported.

```{important} StreamTimeSeriesAccessor requires the bound stream to be numeric Series-kind
Same precondition as every method on this accessor — see
[`Stream::ts`](../stream/ts.md)'s own important callout for the exact throw
message pattern.
```

## Parameters

**axis** (`const Stream&`)
    A `Timeline`, `TimeIndex`, or `Int64`-nanoseconds `Series`, the same
    length as the bound stream, non-missing and sorted ascending.

**duration_nanos** (`std::int64_t`)
    The window width, in nanoseconds. Must be positive.

**statistic** (`pluss::table::TimeWindowStatistic`, default `Mean`)
    `Sum`, `Mean`, `Var`, or `Std`.

**options** (`const pluss::table::StatisticOptions&`)
    `ddof` (default `1`), `min_count` (default `1`).

**closure** (`pluss::table::WindowClosure`, default `Right`)
    Which end(s) of each duration window are inclusive.

## Returns

`Stream`
    A new stream, same length as the bound source, holding the
    duration-window statistic at each position.

## Examples

Example 1. An irregular tick axis — the window naturally covers a different
number of positions at each row, since it's a genuine span of time rather
than a fixed count.

```cpp
auto axis = pluss::kindle::Stream(pluss::make_series(
    std::vector<std::int64_t>{0, 1000000000, 2000000000, 4000000000, 7000000000}));
auto readings = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto out = readings.ts().timed(axis, 3000000000, pluss::table::TimeWindowStatistic::Mean);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=5
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 10                                                                                                       
1            | 15                                                                                                       
2            | 20                                                                                                       
3            | 35                                                                                                       
4            | 50                                                                                                       
========================================================================================================================
```

With a 3-second window and the default right-closed boundary, position `2`
(tick `2s`) looks back to everything within `3s` — that's ticks `0s`, `1s`,
and `2s` itself, so its window is `(10, 20, 30)` → mean `20`.

## Notes

- With `closure = Right` (the default) or `Both`, a position's own tick is
  always part of its own window. With `Left` or `Neither`, it never is.
- `axis` must be non-missing and sorted ascending, or it throws.
- `axis` must be `Timeline`, `TimeIndex`, or an `Int64` nanoseconds `Series`,
  or it throws `"Slate::timed: axis must be Timeline, TimeIndex, or Int64
  nanoseconds"`.
- Only `Sum`, `Mean`, `Var`, and `Std` are supported.
- A pre-existing missing entry in the bound stream (not the axis) is skipped
  within any window that contains it, the same as [`rolling()`](rolling.md).
- See the important callout above.

## See Also

- [`StreamTimeSeriesAccessor::rolling`](rolling.md): the calendar-duration
  overload of `rolling()` delegates straight to this method.
- [`StreamTimeSeriesAccessor::grouped`](grouped.md): a fixed-row-count window
  computed independently per group, rather than a duration-based window.
- [`Stream::ts`](../stream/ts.md): the entry point this accessor is reached from.
- [`TimeSeriesAccessor::timed`](../slate-ts/timed.md): the underlying
  single-slate engine, with additional edge-case examples (a non-monotonic
  axis throw).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="zscore_rolling.html">
    <span class="label">Next</span>
    <span class="title">zscore_rolling</span>
  </a>
</div>
