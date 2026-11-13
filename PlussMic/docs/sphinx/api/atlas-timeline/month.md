<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d30c5661e8d5 -->

# AtlasTimelineAccessor::month

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
Atlas AtlasTimelineAccessor::month() const
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with its
calendar month (`1`-`12`), as a numeric `Series` — the same transform
[`DataFrame::tl().month()`](../dataframe-timeline/month.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by its `Int64` calendar month.

## Examples

Example 1. Extract the calendar month from a `Timeline` feature stacked across two
frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().month();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | ts  
-----------
0    | 1   
1    | 2   
-----------
[2 cs x 1 feat]
===========
     | ts  
-----------
0    | 7   
1    | 3   
-----------
[2 cs x 1 feat]
```

## Notes

- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output, same as on a single frame — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- Reads the local calendar month relative to each frame's `Timeline`'s current timezone.
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::month`](../dataframe-timeline/month.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::year`](year.md), [`AtlasTimelineAccessor::day`](day.md): the
  other calendar-date components.
- [`AtlasTimelineAccessor::month_start`](month_start.md),
  [`AtlasTimelineAccessor::month_end`](month_end.md): whether the row falls on its
  month's first/last day.
- [`AtlasTimelineAccessor::month_name`](month_name.md): the month as a text name instead
  of a number.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minute.html">
    <span class="label">Previous</span>
    <span class="title">minute</span>
  </a>
  <a class="next-link" href="month_end.html">
    <span class="label">Next</span>
    <span class="title">month_end</span>
  </a>
</div>
