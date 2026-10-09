<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 63ec2d3886b5 -->

# AtlasTimelineAccessor::minute

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::minute() const -> Atlas
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with its
minute-of-hour (`0`-`59`), as a numeric `Series` — the same transform
[`DataFrame::tl().minute()`](../dataframe-timeline/minute.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by its `Int64` minute-of-hour.

## Examples

Example 1. Extract the minute-of-hour from a `Timeline` feature stacked across two
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

auto out = atlas.tl().minute();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | ts  
-----------
0    | 15  
1    | 45  
-----------
[2 cs x 1 feat]
===========
     | ts  
-----------
0    | 0   
1    | 0   
-----------
[2 cs x 1 feat]
```

## Notes

- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output, same as on a single frame — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- Reads the local clock minute relative to each frame's `Timeline`'s current timezone.
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::minute`](../dataframe-timeline/minute.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::hour`](hour.md), [`AtlasTimelineAccessor::second`](second.md):
  the neighboring time-of-day components.

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
