<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bb53a235e6ba -->

# DataFrameTimelineAccessor::ceil

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::ceil(std::string_view unit) const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with each row rounded
*up* to the nearest `unit` boundary, in local wall time — same feature name,
still `Timeline`-kind, same row order.

## Parameters

**unit** (`std::string_view`)

One of `"S"`/`"s"`/`"sec"`/`"second"` (second), `"min"`/`"m"`/`"minute"`
(minute), `"H"`/`"h"`/`"hour"` (hour), or `"D"`/`"d"`/`"day"` (day). Cannot be
empty.

## Returns

`DataFrame`
    A new frame with each selected time feature ceiled to `unit`.

## Examples

Example 1. Ceil every row of a `Timeline` feature to the nearest hour.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "2024-06-01T09:47:12Z",
          "2024-06-01T14:05:00Z",
      }))}});
std::cout << events;

auto out = events.tl().ceil("H");
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
1    | 2024-06-01 15:00:00
--------------------------
[2 cs x 1 feat]
```

Both rows advance to the start of the *next* hour — `09:47:12` -> `10:00:00`,
`14:05:00` -> `15:00:00` — even though neither was already on an hour
boundary.

Example 2. Ceil to the nearest second with `"S"`, including one row that's
already exactly on a second boundary.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "2024-06-01T14:05:00.250000000Z",
          "2024-06-01T14:05:01.000000000Z",
      }))}});
std::cout << events;

auto out = events.tl().ceil("S");
std::cout << out;
```

### Output (before)

```text
=============================
     | ts                    
-----------------------------
0    | 2024-06-01 14:05:00.2~
1    | 2024-06-01 14:05:01.0~
-----------------------------
[2 cs x 1 feat]
```

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 14:05:01
1    | 2024-06-01 14:05:01
--------------------------
[2 cs x 1 feat]
```

Row 0 has `.25` seconds of remainder, so it advances to the next whole second,
`14:05:01`. Row 1 is already exactly `14:05:01.000000000` — already on a
second boundary — so it's left unchanged, landing on the same result as row 0
rather than advancing again to `14:05:02`.

Example 3. Ceil to the nearest minute with `"min"`.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({"2024-06-01T14:05:47Z"}))}});
std::cout << events;

auto out = events.tl().ceil("min");
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
0    | 2024-06-01 14:06:00
--------------------------
[1 cs x 1 feat]
```

`:47` seconds into the `14:05` minute advances to the start of `14:06`.

Example 4. Ceil to the nearest day with `"D"`, again mixing a mid-day row with
one already on the boundary.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "2024-06-01T14:05:00Z",
          "2024-06-03T00:00:00Z",
      }))}});
std::cout << events;

auto out = events.tl().ceil("D");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 14:05:00
1    | 2024-06-03 00:00:00
--------------------------
[2 cs x 1 feat]
```

### Output (after)

```text
=================
     | ts        
-----------------
0    | 2024-06-02
1    | 2024-06-03
-----------------
[2 cs x 1 feat]
```

Row 0 advances from mid-day on `06-01` to the next day's midnight, `06-02`.
Row 1 is already exactly `06-03`'s midnight, so it stays `06-03` rather than
advancing to `06-04`. See [`floor()`](floor.md) for the same unsupported-unit
throw behavior (`"Timeline: unsupported unit (expected S/min/H/D)"`), which
applies identically here.

## Notes

- A row that already sits exactly on a `unit` boundary is left unchanged (not
  advanced to the *next* boundary).
- Ceiling happens in **local wall time** (per the `Timeline`'s current
  timezone), then maps back.
- `unit` cannot be empty, and only accepts the fixed clock units listed above.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row stays `NaT` (displays as `"NULL"`) regardless of `unit`.

## See Also

- [`DataFrameTimelineAccessor::floor`](floor.md),
  [`DataFrameTimelineAccessor::round`](round.md): round down, or to the
  nearest boundary, instead of always up.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="date.html">
    <span class="label">Next</span>
    <span class="title">date</span>
  </a>
</div>
