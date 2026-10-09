<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 717d741ca30d -->

# DataFrameTimelineAccessor::weekday

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::weekday() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with its day-of-week,
as a numeric `Series` — same feature name, same row order. Encoding is
`0 = Monday` through `6 = Sunday`.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by its `Int64`
    day-of-week (`0`-`6`, Monday-first).

## Examples

Example 1. Extract the day-of-week from every row of a `Timeline` feature.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-02-29T23:45:00Z",
          "2024-07-06T14:00:00Z",
          "2023-03-01T00:00:00Z",
      }))}});
std::cout << events;

auto out = events.tl().weekday();
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:15:30
1    | 2024-02-29 23:45:00
2    | 2024-07-06 14:00:00
3    | 2023-03-01 00:00:00
--------------------------
[4 cs x 1 feat]
```

### Output (after)

```text
===========
     | ts  
-----------
0    | 2   
1    | 3   
2    | 5   
3    | 2   
-----------
[4 cs x 1 feat]
```

`"2024-01-31"` and `"2023-03-01"` are both Wednesdays (`2`); `"2024-02-29"` is
a Thursday (`3`); `"2024-07-06"` is a Saturday (`5`).

## Notes

- The encoding is Monday `0` through Sunday `6` — **not** the C/Python
  `tm_wday`-style Sunday-first convention some other libraries use.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Slate`) displays as
  `"NULL"`.

## See Also

- [`DataFrameTimelineAccessor::weekday_name`](weekday_name.md): the same
  information as a text name instead of a number.
- [`DataFrameTimelineAccessor::weekend`](weekend.md): whether the day falls on
  Saturday/Sunday.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="week.html">
    <span class="label">Previous</span>
    <span class="title">week</span>
  </a>
  <a class="next-link" href="weekday_name.html">
    <span class="label">Next</span>
    <span class="title">weekday_name</span>
  </a>
</div>
