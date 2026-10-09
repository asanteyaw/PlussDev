<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 98b02b14c86f -->

# DataFrameTimelineAccessor::month_name

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::month_name() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with its full English
month name, as a `Note` (text) `Series` — same feature name, same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by a `Note` Series
    holding each row's full month name (`"January"`-`"December"`).

## Examples

Example 1. Extract the month name from every row of a `Timeline` feature.

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

auto out = events.tl().month_name();
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
===============
     | ts      
---------------
0    | January 
1    | February
2    | July    
3    | March   
---------------
[4 cs x 1 feat]
```

The `"ts"` column keeps its name but is now `Note`-kind, holding each row's
full month name.

## Notes

- Always the full English name (`"January"`, not `"Jan"`); use
  [`fmt("%b")`](fmt.md) for the abbreviated three-letter form.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Slate`) displays as
  `"NULL"`, not a text placeholder.

## See Also

- [`DataFrameTimelineAccessor::month`](month.md): the same information as a
  `1`-`12` number instead of a name.
- [`DataFrameTimelineAccessor::weekday_name`](weekday_name.md): the equivalent
  for weekdays.
- [`DataFrameTimelineAccessor::fmt`](fmt.md): build any other text
  representation via a custom pattern.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_end.html">
    <span class="label">Previous</span>
    <span class="title">month_end</span>
  </a>
  <a class="next-link" href="month_start.html">
    <span class="label">Next</span>
    <span class="title">month_start</span>
  </a>
</div>
