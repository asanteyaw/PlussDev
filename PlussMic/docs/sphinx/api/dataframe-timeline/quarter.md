<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c7fedf2c634d -->

# DataFrameTimelineAccessor::quarter

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameTimelineAccessor::quarter() const
```

## What It Does

Replaces every selected `Timeline` feature with its calendar
quarter (`1`-`4`), as a numeric `Series` — same feature name, same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by its `Int64`
    quarter.

## Examples

Example 1. Extract the quarter from every row of a `Timeline` feature.

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

auto out = events.tl().quarter();
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
1    | 1   
2    | 3   
3    | 1   
-----------
[4 cs x 1 feat]
```

January, February, and March all fall in Q1; July falls in Q3.

## Notes

- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Slate`) displays as
  `"NULL"`.
- Calendar quarters only (`Jan-Mar`, `Apr-Jun`, `Jul-Sep`, `Oct-Dec`) — there's
  no fiscal-quarter offset parameter.

## See Also

- [`DataFrameTimelineAccessor::month`](month.md): the underlying month
  `quarter()` is derived from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="period.html">
    <span class="label">Previous</span>
    <span class="title">period</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
