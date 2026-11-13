<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6b13db5f0997 -->

# DataFrameTimelineAccessor::day

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameTimelineAccessor::day() const
```

## What It Does

Replaces every selected `Timeline` feature with its day-of-month
(`1`-`31`), as a numeric `Series` — same feature name, same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by its `Int64`
    day-of-month.

## Examples

Example 1. Extract the day-of-month from every row of a `Timeline` feature.

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

auto out = events.tl().day();
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
1    | 29  
2    | 6   
3    | 1   
-----------
[4 cs x 1 feat]
```

The `"ts"` column keeps its name but is now `Int64`, holding each row's
day-of-month.

## Notes

- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if the source frame has
  other kinds mixed in.
- A `NaT` row (or one already null on the underlying `Slate`) displays as
  `"NULL"`.
- Reads the local calendar day relative to the `Timeline`'s current timezone.

## See Also

- [`DataFrameTimelineAccessor::year`](year.md),
  [`DataFrameTimelineAccessor::month`](month.md): the other calendar-date
  components.
- [`DataFrameTimelineAccessor::month_start`](month_start.md),
  [`DataFrameTimelineAccessor::month_end`](month_end.md): whether this day is
  its month's first/last.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="date.html">
    <span class="label">Previous</span>
    <span class="title">date</span>
  </a>
  <a class="next-link" href="epoch.html">
    <span class="label">Next</span>
    <span class="title">epoch</span>
  </a>
</div>
