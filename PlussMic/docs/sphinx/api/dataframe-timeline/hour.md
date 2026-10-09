<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2af73d3dc4c1 -->

# DataFrameTimelineAccessor::hour

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::hour() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with its hour-of-day
(`0`-`23`), as a numeric `Series` — same feature name, same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by its `Int64`
    hour-of-day.

## Examples

Example 1. Extract the hour from every row of a `Timeline` feature.

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

auto out = events.tl().hour();
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
0    | 9   
1    | 23  
2    | 14  
3    | 0   
-----------
[4 cs x 1 feat]
```

The `"ts"` column keeps its name but is now `Int64`, holding each row's
hour-of-day.

## Notes

- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Slate`) displays as
  `"NULL"`.
- Reads the local hour relative to the `Timeline`'s current timezone; changing
  timezone with [`tz()`](tz.md) changes what `hour()` reports for the same
  underlying instant.

## See Also

- [`DataFrameTimelineAccessor::minute`](minute.md),
  [`DataFrameTimelineAccessor::second`](second.md): the finer clock-time
  components.
- [`DataFrameTimelineAccessor::time`](time.md): the full clock-time portion as
  a `Timeline`, instead of one numeric field at a time.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmt.html">
    <span class="label">Previous</span>
    <span class="title">fmt</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
