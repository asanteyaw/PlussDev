<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4ceec208a3b7 -->

# DataFrameTimelineAccessor::week

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::week() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature with its ISO-8601 week
number (`1`-`53`), as a numeric `Series` — same feature name, same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by its `Int64`
    ISO week number.

## Examples

Example 1. Extract the ISO week number from every row of a `Timeline`
feature.

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

auto out = events.tl().week();
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
0    | 5   
1    | 9   
2    | 27  
3    | 9   
-----------
[4 cs x 1 feat]
```

## Notes

- Follows the ISO-8601 definition: weeks start on Monday, and week `1` is the
  week containing the year's first Thursday. Because of this, dates in very
  late December or very early January can report a week number that belongs
  to the neighboring calendar year (e.g. December 31st can report week `1` of
  the following ISO year) — `week()` alone doesn't disambiguate which ISO
  year that week number belongs to.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Thread`) displays as
  `"NULL"`.

## See Also

- [`DataFrameTimelineAccessor::quarter`](quarter.md): the coarser calendar
  grouping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="valid.html">
    <span class="label">Previous</span>
    <span class="title">valid</span>
  </a>
  <a class="next-link" href="weekday.html">
    <span class="label">Next</span>
    <span class="title">weekday</span>
  </a>
</div>
