<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ed24e59d039a -->

# DataFrameTimelineAccessor::month_start

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::month_start() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with a boolean `Series`
flagging whether the row falls on the first day of its month — same feature
name, same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by a `Bool` Series:
    `true` when the day-of-month is `1`, `false` otherwise.

## Examples

Example 1. Flag month-start rows in a `Timeline` feature.

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

auto out = events.tl().month_start();
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
2    | false
3    | true 
------------
[4 cs x 1 feat]
```

Only `"2023-03-01"` falls on the first of its month.

## Notes

- Only checks day-of-month `== 1` — the time-of-day portion (if any) doesn't
  affect the result.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Slate`) displays as
  `"NULL"`, not `false`.

## See Also

- [`DataFrameTimelineAccessor::month_end`](month_end.md): the mirror-image
  check for the last day of the month.
- [`DataFrameTimelineAccessor::period`](period.md): truncate every row to its
  month start instead of just flagging it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_name.html">
    <span class="label">Previous</span>
    <span class="title">month_name</span>
  </a>
  <a class="next-link" href="period.html">
    <span class="label">Next</span>
    <span class="title">period</span>
  </a>
</div>
