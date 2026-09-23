<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 643d79d3e0fb -->

# DataFrameTimelineAccessor::yearday

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::yearday() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature with its 1-based
day-of-year (`1`-`366`), as a numeric `Series` — same feature name, same row
order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by its `Int64`
    day-of-year.

## Examples

Example 1. Extract the day-of-year from every row of a `Timeline` feature.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-02-29T23:45:00Z",
          "2024-07-06T14:00:00Z",
          "2023-03-01T00:00:00Z",
      }))}});
std::cout << events;

auto out = events.tl().yearday();
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
0    | 31  
1    | 60  
2    | 188 
3    | 60  
-----------
[4 cs x 1 feat]
```

`"2024-02-29"` (a leap day) and `"2023-03-01"` (a non-leap year) both land on
day `60` — 2024's leap day shifts everything from March onward by one day
relative to a non-leap year, so early-March dates coincide numerically.

## Notes

- 1-based: January 1st is day `1`, never `0`.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Thread`) displays as
  `"NULL"`.

## See Also

- [`DataFrameTimelineAccessor::leap`](leap.md): whether the row's year is a
  leap year, which is exactly what shifts `yearday()` by one day after
  February.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="year.html">
    <span class="label">Previous</span>
    <span class="title">year</span>
  </a>
</div>
