<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4e6855e8043b -->

# DataFrameTimelineAccessor::year

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::year() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with its calendar year,
as a numeric `Series` — same feature name, same row order, only the payload
kind changes.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by its `Int64` year.
    Any feature not selected by `.tl()` (there won't be one — see Notes) is
    left untouched.

## Examples

Example 1. Extract the year from every row of a `Timeline` feature.

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

auto out = events.tl().year();
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
0    | 2024
1    | 2024
2    | 2024
3    | 2023
-----------
[4 cs x 1 feat]
```

The `"ts"` column keeps its name but is now `Int64`, holding each row's
calendar year.

## Notes

- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind, so in practice every feature ends up
  selected — narrow with [`features()`](../dataframe/features.md) first
  (`df.features({"ts"}).tl().year()`) if the source frame also has non-time
  columns.
- A row whose underlying timestamp is `NaT` (or whose `Slate` already carries
  a null mask) displays as `"NULL"` in the year output, not some sentinel
  number.
- `year()` reads local calendar fields — i.e. relative to whatever timezone
  the `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).

## See Also

- [`DataFrameTimelineAccessor::month`](month.md),
  [`DataFrameTimelineAccessor::day`](day.md): the other calendar-date
  components, extracted the same way.
- [`DataFrameTimelineAccessor::leap`](leap.md): whether the row's year is a
  leap year.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekend.html">
    <span class="label">Previous</span>
    <span class="title">weekend</span>
  </a>
  <a class="next-link" href="yearday.html">
    <span class="label">Next</span>
    <span class="title">yearday</span>
  </a>
</div>
