<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 04a76d00355b -->

# AtlasTimelineAccessor::day

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::day() const -> Atlas
```

## What It Does

Replaces every `Timeline`/`TimeIndex` feature in **every frame of the stack** with its
day-of-month (`1`-`31`), as a numeric `Series` — the same transform
[`DataFrame::tl().day()`](../dataframe-timeline/day.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by its `Int64` day-of-month.

## Examples

Example 1. Extract the day-of-month from a `Timeline` feature stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().day();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | ts  
-----------
0    | 31  
1    | 29  
-----------
[2 cs x 1 feat]
===========
     | ts  
-----------
0    | 6   
1    | 1   
-----------
[2 cs x 1 feat]
```

## Notes

- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`/`TimeIndex`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output, same as on a single frame — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- Reads the local calendar day relative to each frame's `Timeline`'s current timezone.
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::day`](../dataframe-timeline/day.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::year`](year.md), [`AtlasTimelineAccessor::month`](month.md):
  the other calendar-date components.
- [`AtlasTimelineAccessor::month_start`](month_start.md),
  [`AtlasTimelineAccessor::month_end`](month_end.md): whether this day is its month's
  first/last.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="date.html">
    <span class="label">Previous</span>
    <span class="title">date</span>
  </a>
  <a class="next-link" href="epoch.html">
    <span class="label">Next</span>
    <span class="title">epoch</span>
  </a>
</div>
