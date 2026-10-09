<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 771cce1ee1ba -->

# DataFrameTimelineAccessor::weekend

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::weekend() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with a boolean `Series`
flagging whether the row falls on Saturday or Sunday — same feature name,
same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by a `Bool` Series:
    `true` on Saturday/Sunday, `false` otherwise.

## Examples

Example 1. Flag weekend rows in a `Timeline` feature.

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

auto out = events.tl().weekend();
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
============
     | ts   
------------
0    | false
1    | false
2    | true 
3    | false
------------
[4 cs x 1 feat]
```

Only `"2024-07-06"` (a Saturday) is a weekend row.

## Notes

- `true` covers Saturday and Sunday only — there's no way to include Friday
  or exclude one of the two via this method; build a custom mask from
  [`weekday()`](weekday.md) if you need a different day set.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Slate`) displays as
  `"NULL"`, not `false`.

## See Also

- [`DataFrameTimelineAccessor::weekday`](weekday.md): the underlying
  day-of-week number `weekend()` is derived from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday_name.html">
    <span class="label">Previous</span>
    <span class="title">weekday_name</span>
  </a>
  <a class="next-link" href="year.html">
    <span class="label">Next</span>
    <span class="title">year</span>
  </a>
</div>
