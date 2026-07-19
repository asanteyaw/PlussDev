<!-- pluss-hand-authored -->

# DataFrameTimelineAccessor::round

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::round(std::string_view unit) const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature with each row rounded
to the *nearest* `unit` boundary, in local wall time — same feature name,
still `Timeline`/`TimeIndex`-kind, same row order.

## Parameters

**unit** (`std::string_view`)

One of `"S"`/`"s"`/`"sec"`/`"second"` (second), `"min"`/`"m"`/`"minute"`
(minute), `"H"`/`"h"`/`"hour"` (hour), or `"D"`/`"d"`/`"day"` (day). Cannot be
empty.

## Returns

`DataFrame`
    A new frame with each selected time feature rounded to the nearest
    `unit`.

## Examples

Example 1. Round every row of a `Timeline` feature to the nearest hour.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-06-01T09:47:12Z",
          "2024-06-01T14:05:00Z",
      }))}});
std::cout << events;

auto out = events.tl().round("H");
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
0    | 2024-06-01 10:00:00
1    | 2024-06-01 14:00:00
--------------------------
[2 cs x 1 feat]
```

`09:47:12` is closer to `10:00:00` than `09:00:00`, so it rounds up; `14:05:00`
is closer to `14:00:00` than `15:00:00`, so it rounds down.

Example 2. Round to the nearest second with `"S"`.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-06-01T14:05:00.300000000Z",
          "2024-06-01T14:05:00.700000000Z",
      }))}});
std::cout << events;

auto out = events.tl().round("S");
std::cout << out;
```

### Output (before)

```text
=============================
     | ts                    
-----------------------------
0    | 2024-06-01 14:05:00.3~
1    | 2024-06-01 14:05:00.7~
-----------------------------
[2 cs x 1 feat]
```

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 14:05:00
1    | 2024-06-01 14:05:01
--------------------------
[2 cs x 1 feat]
```

`.3` seconds rounds down to `:00`; `.7` seconds rounds up to `:01`.

Example 3. Round to the nearest minute with `"min"`.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-06-01T14:05:29Z",
          "2024-06-01T14:05:31Z",
      }))}});
std::cout << events;

auto out = events.tl().round("min");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 14:05:29
1    | 2024-06-01 14:05:31
--------------------------
[2 cs x 1 feat]
```

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 14:05:00
1    | 2024-06-01 14:06:00
--------------------------
[2 cs x 1 feat]
```

`:29` seconds into the minute rounds down; `:31` seconds rounds up to the next
minute.

Example 4. Round to the nearest day with `"D"`, using rows just before and
just after local noon.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-06-01T11:59:00Z",
          "2024-06-01T12:01:00Z",
      }))}});
std::cout << events;

auto out = events.tl().round("D");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 11:59:00
1    | 2024-06-01 12:01:00
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

`11:59` on `06-01` is still closer to that day's own midnight than the next
one, so it rounds down to `06-01`; two minutes later, at `12:01`, the day has
tipped past its midpoint and rounds up to `06-02`.

Example 5. Round a row that sits *exactly* halfway between two hour
boundaries, to confirm the tie-break direction.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-06-01T09:30:00Z"}))}});
std::cout << events;

auto out = events.tl().round("H");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 09:30:00
--------------------------
[1 cs x 1 feat]
```

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 10:00:00
--------------------------
[1 cs x 1 feat]
```

`09:30:00` is exactly halfway between `09:00:00` and `10:00:00`. The tie
resolves **up**, to `10:00:00` — this method never breaks a tie downward. The
same unsupported-unit throw as [`floor()`](floor.md)/[`ceil()`](ceil.md)
(`"Timeline: unsupported unit (expected S/min/H/D)"`) applies here too.

## Notes

- Rounding happens in **local wall time** (per the `Timeline`'s current
  timezone), then maps back.
- `unit` cannot be empty, and only accepts the fixed clock units listed above.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row stays `NaT` (displays as `"NULL"`) regardless of `unit`.

## See Also

- [`DataFrameTimelineAccessor::floor`](floor.md),
  [`DataFrameTimelineAccessor::ceil`](ceil.md): always down, or always up,
  instead of nearest.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quarter.html">
    <span class="label">Previous</span>
    <span class="title">quarter</span>
  </a>
  <a class="next-link" href="second.html">
    <span class="label">Next</span>
    <span class="title">second</span>
  </a>
</div>
