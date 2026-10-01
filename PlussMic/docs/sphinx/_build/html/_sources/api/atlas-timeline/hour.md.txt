<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5ce25f46fd0b -->

# AtlasTimelineAccessor::hour

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::hour() const -> Atlas
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with its
hour-of-day (`0`-`23`), as a numeric `Series` — the same transform
[`DataFrame::tl().hour()`](../dataframe-timeline/hour.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by its `Int64` hour-of-day.

## Examples

Example 1. Extract the hour-of-day from a `Timeline` feature stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().hour();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | ts  
-----------
0    | 9   
1    | 23  
-----------
[2 cs x 1 feat]
===========
     | ts  
-----------
0    | 14  
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
- Reads the local clock hour relative to each frame's `Timeline`'s current timezone.
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::hour`](../dataframe-timeline/hour.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::minute`](minute.md), [`AtlasTimelineAccessor::second`](second.md):
  the finer-grained time-of-day components.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmt.html">
    <span class="label">Previous</span>
    <span class="title">fmt</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
