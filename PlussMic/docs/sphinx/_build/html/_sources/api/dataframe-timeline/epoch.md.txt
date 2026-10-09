<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 01ffcfd303be -->

# DataFrameTimelineAccessor::epoch

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::epoch(TimeUnit unit = TimeUnit::Nanos) const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with its Unix epoch
value in `unit`, as a numeric `Series` — same feature name, same row order.

## Parameters

**unit** (`TimeUnit`, default `TimeUnit::Nanos`)

One of the fixed-duration units: `Nanos`, `Micros`, `Millis`, `Seconds`, or
`Days`. Passing a span-only unit (`Weeks`, `Months`, `Years`,
`BusinessDays` — the calendar-relative units meant for
[`span()`](span.md), not a fixed epoch count) throws.

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by an `Int64` Series
    of epoch counts in `unit`.

## Examples

Example 1. Extract the epoch second from every row of a `Timeline` feature.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "1970-01-01T00:00:00Z",
          "2024-01-01T00:00:10Z",
      }))}});
std::cout << events;

auto out = events.tl().epoch(TimeUnit::Seconds);
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 1970-01-01 00:00:00
1    | 2024-01-01 00:00:10
--------------------------
[2 cs x 1 feat]
```

### Output (after)

```text
=================
     | ts        
-----------------
0    | 0         
1    | 1704067210
-----------------
[2 cs x 1 feat]
```

The `"ts"` column keeps its name but is now `Int64`, holding each row's count
of whole seconds since the Unix epoch.

Example 2. Call `epoch()` with no argument to confirm the default unit is
`Nanos`, not `Seconds`.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "1970-01-01T00:00:00Z",
          "2024-01-01T00:00:10Z",
      }))}});

auto out = events.tl().epoch();
std::cout << out;
```

### Output

```text
==========================
     | ts                 
--------------------------
0    | 0                  
1    | 1704067210000000000
--------------------------
[2 cs x 1 feat]
```

With no argument, the same row that showed `1704067210` in `Seconds` above
comes back as `1704067210000000000` — nine zeros further out, confirming the
default is `Nanos`.

Example 3. Use `Millis` on a row with a sub-second component to see the
fractional part truncate rather than round.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "1970-01-01T00:00:00.000000000Z",
          "2024-01-01T00:00:10.123456789Z",
      }))}});
std::cout << events;

auto out = events.tl().epoch(TimeUnit::Millis);
std::cout << out;
```

### Output (before)

```text
=============================
     | ts                    
-----------------------------
0    | 1970-01-01 00:00:00.0~
1    | 2024-01-01 00:00:10.1~
-----------------------------
[2 cs x 1 feat]
```

### Output (after)

```text
====================
     | ts           
--------------------
0    | 0            
1    | 1704067210123
--------------------
[2 cs x 1 feat]
```

`.123456789` seconds is `123.456789` milliseconds; `Millis` keeps only the
whole-millisecond count `123`, discarding the remaining `456789` nanoseconds
rather than rounding up.

Example 4. Use `Days` to see a mid-day timestamp collapse to its whole-day
count.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "1970-01-01T00:00:00Z",
          "2024-01-01T12:00:00Z",
      }))}});
std::cout << events;

auto out = events.tl().epoch(TimeUnit::Days);
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 1970-01-01 00:00:00
1    | 2024-01-01 12:00:00
--------------------------
[2 cs x 1 feat]
```

### Output (after)

```text
============
     | ts   
------------
0    | 0    
1    | 19723
------------
[2 cs x 1 feat]
```

Noon on `2024-01-01` is still inside day `19723` (whole days since epoch); the
half-day remainder is dropped, not rounded to `19724`.

Example 5. Pass a span-only unit to see the call throw instead of returning a
result.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({"2024-01-01T00:00:00Z"}))}});

try {
  auto out = events.tl().epoch(TimeUnit::Months);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::tl().epoch: unit is span-only; use Slate::tl().span
```

`Months` (like `Weeks`, `Years`, and `BusinessDays`) only makes sense as a
calendar-relative distance between two timestamps, so `epoch()` rejects it
outright — see [`span()`](span.md) for the method that does accept it.

## Notes

- The default unit is `Nanos`, not `Seconds` — calling `epoch()` with no
  argument on the same data above would return `[0, 1704067210000000000]`
  instead.
- Only fixed-duration units are accepted (`Nanos`/`Micros`/`Millis`/`Seconds`/
  `Days`); passing `Weeks`, `Months`, `Years`, or `BusinessDays` throws, since
  those units only make sense as a calendar-relative distance between two
  timestamps — see [`span()`](span.md) for that.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Slate`) displays as
  `"NULL"`, not a sentinel epoch value.

## See Also

- [`DataFrameTimelineAccessor::span`](span.md): elapsed distance *between* two
  timelines, including calendar-relative units `epoch()` doesn't support.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="day.html">
    <span class="label">Previous</span>
    <span class="title">day</span>
  </a>
  <a class="next-link" href="floor.html">
    <span class="label">Next</span>
    <span class="title">floor</span>
  </a>
</div>
