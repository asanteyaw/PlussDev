<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4861aec49479 -->

# DataFrameTimelineAccessor::minute

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::minute() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature with its minute-of-hour
(`0`-`59`), as a numeric `Series` — same feature name, same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by its `Int64`
    minute-of-hour.

## Examples

Example 1. Extract the minute from every row of a `Timeline` feature.

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

auto out = events.tl().minute();
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
0    | 15  
1    | 45  
2    | 0   
3    | 0   
-----------
[4 cs x 1 feat]
```

The `"ts"` column keeps its name but is now `Int64`, holding each row's
minute-of-hour.

## Notes

- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Thread`) displays as
  `"NULL"`.
- Reads the local minute relative to the `Timeline`'s current timezone.

## See Also

- [`DataFrameTimelineAccessor::hour`](hour.md),
  [`DataFrameTimelineAccessor::second`](second.md): the neighboring clock-time
  components.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="localize.html">
    <span class="label">Previous</span>
    <span class="title">localize</span>
  </a>
  <a class="next-link" href="month.html">
    <span class="label">Next</span>
    <span class="title">month</span>
  </a>
</div>
