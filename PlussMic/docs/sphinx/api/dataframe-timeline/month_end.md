<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 203f3d36a507 -->

# DataFrameTimelineAccessor::month_end

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::month_end() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with a boolean `Series`
flagging whether the row falls on the last day of its month — same feature
name, same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by a `Bool` Series:
    `true` when the row's day-of-month is its month's last valid day, `false`
    otherwise.

## Examples

Example 1. Flag month-end rows in a `Timeline` feature.

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

auto out = events.tl().month_end();
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
0    | true 
1    | true 
2    | false
3    | false
------------
[4 cs x 1 feat]
```

`"2024-01-31"` is January's last day, and `"2024-02-29"` is February's last
day in this leap year — both flag `true`.

## Notes

- Correctly accounts for leap years: `"2024-02-29"` is month-end (2024 is
  leap), but a February 28th in a non-leap year would also flag `true`.
- Only checks the date portion — the time-of-day (if any) doesn't affect the
  result.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Thread`) displays as
  `"NULL"`, not `false`.

## See Also

- [`DataFrameTimelineAccessor::month_start`](month_start.md): the mirror-image
  check for the first day of the month.
- [`DataFrameTimelineAccessor::leap`](leap.md): whether the row's year is a
  leap year.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month.html">
    <span class="label">Previous</span>
    <span class="title">month</span>
  </a>
  <a class="next-link" href="month_name.html">
    <span class="label">Next</span>
    <span class="title">month_name</span>
  </a>
</div>
