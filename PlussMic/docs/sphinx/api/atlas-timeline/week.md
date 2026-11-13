<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cfb09e0954d5 -->

# AtlasTimelineAccessor::week

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
Atlas AtlasTimelineAccessor::week() const
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with its
ISO week-of-year (`1`-`53`), as a numeric `Series` — the same transform
[`DataFrame::tl().week()`](../dataframe-timeline/week.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by its `Int64` ISO week-of-year.

## Examples

Example 1. Extract the ISO week-of-year from a `Timeline` feature stacked across two
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

auto out = atlas.tl().week();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | ts  
-----------
0    | 5   
1    | 9   
-----------
[2 cs x 1 feat]
===========
     | ts  
-----------
0    | 27  
1    | 9   
-----------
[2 cs x 1 feat]
```

## Notes

- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output, same as on a single frame — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- Uses ISO week numbering, the same convention as the single-frame
  [`DataFrameTimelineAccessor::week`](../dataframe-timeline/week.md).
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::week`](../dataframe-timeline/week.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::yearday`](yearday.md): the finer-grained day-of-year
  component.
- [`AtlasTimelineAccessor::weekday`](weekday.md): the day-of-week within that week.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="valid.html">
    <span class="label">Previous</span>
    <span class="title">valid</span>
  </a>
  <a class="next-link" href="weekday.html">
    <span class="label">Next</span>
    <span class="title">weekday</span>
  </a>
</div>
