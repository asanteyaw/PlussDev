<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ae6acd6bbb3a -->

# AtlasTimelineAccessor::month_start

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
Atlas AtlasTimelineAccessor::month_start() const
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with a
boolean `Series` flagging whether the row falls on the first day of its month — the same
transform [`DataFrame::tl().month_start()`](../dataframe-timeline/month_start.md)
applies to a single frame, broadcast across the whole `Atlas` via
[`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by a `Bool` Series: `true` when the day-of-month is `1`, `false`
    otherwise.

## Examples

Example 1. Flag month-start rows across a `Timeline` feature stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().month_start();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
============
     | ts   
------------
0    | false
1    | false
------------
[2 cs x 1 feat]
============
     | ts   
------------
0    | false
1    | true 
------------
[2 cs x 1 feat]
```

Only `frame(1)`'s `"2023-03-01"` falls on the first of its month.

## Notes

- Only checks day-of-month `== 1` — the time-of-day portion (if any) doesn't affect the
  result.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output, same as on a single frame — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::month_start`](../dataframe-timeline/month_start.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::month_end`](month_end.md): the mirror-image check for the
  last day of the month.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_name.html">
    <span class="label">Previous</span>
    <span class="title">month_name</span>
  </a>
  <a class="next-link" href="quarter.html">
    <span class="label">Next</span>
    <span class="title">quarter</span>
  </a>
</div>
