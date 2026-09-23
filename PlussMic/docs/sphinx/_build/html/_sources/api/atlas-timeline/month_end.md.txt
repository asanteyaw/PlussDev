<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 234f8d30a606 -->

# AtlasTimelineAccessor::month_end

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::month_end() const -> Atlas
```

## What It Does

Replaces every `Timeline`/`TimeIndex` feature in **every frame of the stack** with a
boolean `Series` flagging whether the row falls on the last day of its month — the same
transform [`DataFrame::tl().month_end()`](../dataframe-timeline/month_end.md) applies to
a single frame, broadcast across the whole `Atlas` via
[`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by a `Bool` Series: `true` when the row is its month's last day,
    `false` otherwise.

## Examples

Example 1. Flag month-end rows across a `Timeline` feature stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().month_end();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | ts  
-----------
0    | true
1    | true
-----------
[2 cs x 1 feat]
============
     | ts   
------------
0    | false
1    | false
------------
[2 cs x 1 feat]
```

Both `frame(0)` rows flag `true`: `"2024-01-31"` is January's last day, and
`"2024-02-29"` is February's last day in the 2024 leap year — see
[`leap()`](leap.md).

## Notes

- Correctly accounts for leap years when checking February — `"2024-02-29"` flags
  `true` here because 2024 is a leap year; `"2023-02-28"` would flag `true` in a
  non-leap year instead.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`/`TimeIndex`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output, same as on a single frame — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::month_end`](../dataframe-timeline/month_end.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::month_start`](month_start.md): the mirror-image check for the
  first day of the month.
- [`AtlasTimelineAccessor::leap`](leap.md): the leap-year flag that shifts February's
  last day.

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
