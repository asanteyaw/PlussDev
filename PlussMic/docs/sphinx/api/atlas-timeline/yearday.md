<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 88116d4b2eac -->

# AtlasTimelineAccessor::yearday

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
Atlas AtlasTimelineAccessor::yearday() const
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with its
day-of-year (`1`-`366`), as a numeric `Series` — the same transform
[`DataFrame::tl().yearday()`](../dataframe-timeline/yearday.md) applies to a single
frame, broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by its `Int64` day-of-year.

## Examples

Example 1. Extract the day-of-year from a `Timeline` feature stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().yearday();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | ts  
-----------
0    | 31  
1    | 60  
-----------
[2 cs x 1 feat]
===========
     | ts  
-----------
0    | 188 
1    | 60  
-----------
[2 cs x 1 feat]
```

`frame(0)`'s `"2024-02-29"` is day `60` of the 2024 leap year; `frame(1)`'s
`"2023-03-01"` is also day `60`, but of a non-leap year — see
[`leap()`](leap.md) for the flag that distinguishes them.

## Notes

- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output, same as on a single frame — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- Leap years shift every day-of-year number from March 1st onward by one, relative to a
  non-leap year — see the example above.
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::yearday`](../dataframe-timeline/yearday.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::leap`](leap.md): where leap-year status changes this
  numbering.
- [`AtlasTimelineAccessor::week`](week.md): the coarser week-of-year grouping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="year.html">
    <span class="label">Previous</span>
    <span class="title">year</span>
  </a>
</div>
