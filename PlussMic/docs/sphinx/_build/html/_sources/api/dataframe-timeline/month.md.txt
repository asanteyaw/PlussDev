<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cb65c57fa506 -->

# DataFrameTimelineAccessor::month

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::month() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with its calendar month
(`1`-`12`), as a numeric `Series` — same feature name, same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by its `Int64` month.

## Examples

Example 1. Extract the month from every row of a `Timeline` feature.

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

auto out = events.tl().month();
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
0    | 1   
1    | 2   
2    | 7   
3    | 3   
-----------
[4 cs x 1 feat]
```

The `"ts"` column keeps its name but is now `Int64`, holding each row's
calendar month.

## Notes

- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first
  (`df.features({"ts"}).tl().month()`) if the source frame has other kinds
  mixed in.
- A `NaT` row (or one already null on the underlying `Slate`) displays as
  `"NULL"`, not a sentinel month number.
- Reads the local calendar month relative to the `Timeline`'s current
  timezone.

## See Also

- [`DataFrameTimelineAccessor::year`](year.md),
  [`DataFrameTimelineAccessor::day`](day.md): the other calendar-date
  components.
- [`DataFrameTimelineAccessor::month_start`](month_start.md),
  [`DataFrameTimelineAccessor::month_end`](month_end.md): whether the row
  falls on its month's first/last day.
- [`DataFrameTimelineAccessor::month_name`](month_name.md): the month as a
  text name instead of a number.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minute.html">
    <span class="label">Previous</span>
    <span class="title">minute</span>
  </a>
  <a class="next-link" href="month_end.html">
    <span class="label">Next</span>
    <span class="title">month_end</span>
  </a>
</div>
