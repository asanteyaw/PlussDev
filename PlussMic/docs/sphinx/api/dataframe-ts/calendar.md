<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 907c497ff083 -->

# DataFrameTsAccessor::calendar

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signatures

```cpp
DataFrameTsAccessor::calendar() const -> DataFrame
DataFrameTsAccessor::calendar(std::string_view time_feature) const -> DataFrame
```

## What It Does

Extracts calendar components from a time axis into their own numeric/text
columns: year, month, day, hour, and a formatted weekday string. The no-argument
overload uses this accessor's own time axis (`TsOptions.axis` if set, otherwise
its case labels, which must be `Timeline`/`TimeIndex`); the `time_feature` overload
instead reads an explicit `Timeline`/`TimeIndex` feature by name and prefixes each
output column with that feature's name.

## Parameters

**time_feature** (`std::string_view`, one overload)

The name of an existing `Timeline`/`TimeIndex` feature to extract components from,
instead of this accessor's own axis.

## Returns

`DataFrame`
    A new frame with five columns — year, month, day, hour, and a formatted
    weekday string — one row per case on the source axis.

## Examples

Example 1. Extract calendar components from a frame's own `Timeline` case labels.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"value", Slate(pluss::make_series({1.0, 2.0}))}},
    Slate(pluss::make_timeline({
        "2024-03-14T09:30:00Z", "2024-12-25T00:00:00Z"})));

auto out = events.ts().calendar();
std::cout << out;
```

### Output

```text
===================================================================
     | axis_year  axis_month  axis_day  axis_hour  axis_weekday    
-------------------------------------------------------------------
0    | 2024       3           14        9          2024-03-14 (Thu)
1    | 2024       12          25        0          2024-12-25 (Wed)
-------------------------------------------------------------------
[2 cs x 5 feat]
```

Columns are prefixed `axis_` since no explicit `time_feature` was named — this
accessor fell back to `events`'s own case labels. `2024-03-14` is a Thursday and
`2024-12-25` is a Wednesday; `axis_weekday` formats each as
`"YYYY-MM-DD (Abbreviated weekday)"`.

Example 2. Extract calendar components from a named `Timeline` feature, alongside
a separate bound numeric feature.

```cpp
auto events_named = DataFrame(
    {{"event_time", Slate(pluss::make_timeline({
          "2024-03-14T09:30:00Z", "2024-12-25T00:00:00Z"}))},
     {"value", Slate(pluss::make_series({1.0, 2.0}))}});
std::cout << events_named;

auto out2 = events_named.ts().calendar("event_time");
std::cout << out2;
```

### Output (before)

```text
=================================
     | event_time           value
---------------------------------
0    | 2024-03-14 09:30:00  1    
1    | 2024-12-25 00:00:00  2    
---------------------------------
[2 cs x 2 feat]
```

### Output (after)

```text
=============================================================================================
     | event_time_year  event_time_month  event_time_day  event_time_hour  event_time_weekday
---------------------------------------------------------------------------------------------
0    | 2024             3                 14              9                2024-03-14 (Thu)  
1    | 2024             12                25              0                2024-12-25 (Wed)  
---------------------------------------------------------------------------------------------
[2 cs x 5 feat]
```

`.ts()` auto-binds only the numeric `value` feature here — `event_time` is left
untouched on the underlying frame precisely so that `calendar("event_time")` can
still look it up by name. Columns are prefixed `event_time_` instead of `axis_`
since an explicit `time_feature` was named this time.

Example 3. Call the no-argument overload on a frame whose case labels are the
default integer index, to see it throw.

```cpp
using namespace pluss::table;

auto untimed = DataFrame(
    {{"value", Slate(pluss::make_series({1.0, 2.0}))}});

try {
  auto out = untimed.ts().calendar();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrameTsAccessor::calendar: cases must be Timeline/TimeIndex. Use df.label().set("date") first
```

`untimed` never had a `Timeline`/`TimeIndex` axis attached — no `TsOptions.axis`
and plain integer case labels — so there's no time axis to extract components
from.

## Notes

- `axis_weekday`/`{time_feature}_weekday` is a formatted `"YYYY-MM-DD (Abbreviated
  weekday)"` string, not a bare weekday name or numeric code.
- The no-argument overload requires a genuine time axis — either an explicit
  `TsOptions.axis`, or case labels that are already `Timeline`/`TimeIndex` — and
  throws otherwise.
- Unlike most other `ts()` accessor methods, `calendar()`'s output columns are
  unrelated to whichever numeric feature(s) happen to be bound — it only ever
  reads the time axis itself.
- `DataFrame::ts()`/`ts(options)` auto-bind only the numeric `Series` columns on
  the calling frame as this accessor's value features; any `Timeline`/`TimeIndex`
  (or other non-numeric) column is simply skipped rather than rejected, which is
  what makes the `time_feature`-by-name overload reachable in Example 2 above —
  a frame can carry both value features and a named time-axis column at once.

## See Also

- [`DataFrame::tl`](../dataframe/tl.md): the general calendar/timestamp accessor
  for `Timeline`/`TimeIndex` features, offering `.year()`/`.month()` and similar
  individually rather than as one scribbled frame.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bounded.html">
    <span class="label">Previous</span>
    <span class="title">bounded</span>
  </a>
  <a class="next-link" href="diff.html">
    <span class="label">Next</span>
    <span class="title">diff</span>
  </a>
</div>
