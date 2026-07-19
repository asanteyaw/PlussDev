<!-- pluss-hand-authored -->

# DataFrameTimelineAccessor::floor

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::floor(std::string_view unit) const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature with each row rounded
*down* to the nearest `unit` boundary, in local wall time — same feature
name, still `Timeline`/`TimeIndex`-kind, same row order.

## Parameters

**unit** (`std::string_view`)

One of `"S"`/`"s"`/`"sec"`/`"second"` (second), `"min"`/`"m"`/`"minute"`
(minute), `"H"`/`"h"`/`"hour"` (hour), or `"D"`/`"d"`/`"day"` (day). Cannot be
empty. Sub-day units coarser than these (weeks, months) aren't supported.

## Returns

`DataFrame`
    A new frame with each selected time feature floored to `unit`.

## Examples

Example 1. Floor every row of a `Timeline` feature to the nearest hour.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-06-01T09:47:12Z",
          "2024-06-01T14:05:00Z",
      }))}});
std::cout << events;

auto out = events.tl().floor("H");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 09:47:12
1    | 2024-06-01 14:05:00
--------------------------
[2 cs x 1 feat]
```

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 09:00:00
1    | 2024-06-01 14:00:00
--------------------------
[2 cs x 1 feat]
```

Both rows drop to the start of their hour — `09:47:12` -> `09:00:00`,
`14:05:00` -> `14:00:00`.

Example 2. Floor a sub-second timestamp to the nearest whole second with
`"S"`.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-06-01T14:05:00.750000000Z"}))}});
std::cout << events;

auto out = events.tl().floor("S");
std::cout << out;
```

### Output (before)

```text
=============================
     | ts                    
-----------------------------
0    | 2024-06-01 14:05:00.7~
-----------------------------
[1 cs x 1 feat]
```

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 14:05:00
--------------------------
[1 cs x 1 feat]
```

`.750000000` seconds worth of sub-second precision is dropped entirely — `"S"`
only keeps whole seconds.

Example 3. Floor to the nearest minute with `"min"`.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-06-01T14:05:47Z"}))}});
std::cout << events;

auto out = events.tl().floor("min");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 14:05:47
--------------------------
[1 cs x 1 feat]
```

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 14:05:00
--------------------------
[1 cs x 1 feat]
```

`14:05:47` drops its `:47` seconds, landing on the start of the `14:05`
minute.

Example 4. Floor to the nearest day with `"D"`, collapsing both rows to bare
dates.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-06-01T14:05:00Z",
          "2024-06-02T23:59:59Z",
      }))}});
std::cout << events;

auto out = events.tl().floor("D");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 14:05:00
1    | 2024-06-02 23:59:59
--------------------------
[2 cs x 1 feat]
```

### Output (after)

```text
=================
     | ts        
-----------------
0    | 2024-06-01
1    | 2024-06-02
-----------------
[2 cs x 1 feat]
```

Both rows land on midnight of their own calendar day; since the whole result
column is now all-midnight, it renders as bare dates instead of full
timestamps.

Example 5. Floor to the nearest day *after* relabeling the display timezone,
to see that `"D"` aligns to **local** midnight, not UTC midnight.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-01-15T03:00:00Z"}))}});

auto local = events.tl().tz("America/New_York");
std::cout << local;

auto out = local.tl().floor("D");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-14 22:00:00
--------------------------
[1 cs x 1 feat]
```

### Output (after)

```text
=================
     | ts        
-----------------
0    | 2024-01-14
-----------------
[1 cs x 1 feat]
```

`2024-01-15T03:00:00Z` is `2024-01-14 22:00:00` in `America/New_York` (UTC-5
in January). Floored in that local wall time, it lands on `2024-01-14`'s
midnight — a full UTC day *earlier* than the UTC calendar date would suggest.
Had `floor("D")` worked in UTC instead, this row would have landed on
`2024-01-15`.

Example 6. Pass an unsupported unit to see the call throw.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-06-01T14:05:00Z"}))}});

try {
  auto out = events.tl().floor("week");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Timeline: unsupported unit (expected S/min/H/D)
```

`"week"` isn't one of the four accepted units, so the call throws instead of
guessing at a coarser boundary.

## Notes

- Flooring happens in **local wall time** (per the `Timeline`'s current
  timezone), then maps back — so `floor("D")` aligns to local midnight, not
  necessarily UTC midnight, if the timezone isn't `"UTC"`.
- `unit` cannot be empty, and only accepts the fixed clock units listed above
  — there's no `"W"` (week) or calendar-month unit here; see
  [`period()`](period.md) for month/year truncation instead.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row stays `NaT` (displays as `"NULL"`) regardless of `unit`.

## See Also

- [`DataFrameTimelineAccessor::ceil`](ceil.md),
  [`DataFrameTimelineAccessor::round`](round.md): round up, or to the nearest
  boundary, instead of always down.
- [`DataFrameTimelineAccessor::date`](date.md): shorthand for
  `floor("D")`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch.html">
    <span class="label">Previous</span>
    <span class="title">epoch</span>
  </a>
  <a class="next-link" href="fmt.html">
    <span class="label">Next</span>
    <span class="title">fmt</span>
  </a>
</div>
