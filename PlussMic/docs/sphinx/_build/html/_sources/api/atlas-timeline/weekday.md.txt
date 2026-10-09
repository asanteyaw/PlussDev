<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7d9cc03a0022 -->

# AtlasTimelineAccessor::weekday

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::weekday() const -> Atlas
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with its
weekday number (`0` = Monday through `6` = Sunday), as a numeric `Series` — the same
transform [`DataFrame::tl().weekday()`](../dataframe-timeline/weekday.md) applies to a
single frame, broadcast across the whole `Atlas` via
[`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by its `Int64` weekday number.

## Examples

Example 1. Extract the weekday number from a `Timeline` feature stacked across two
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

auto out = atlas.tl().weekday();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | ts  
-----------
0    | 2   
1    | 3   
-----------
[2 cs x 1 feat]
===========
     | ts  
-----------
0    | 5   
1    | 2   
-----------
[2 cs x 1 feat]
```

`"2024-01-31"` is a Wednesday (`2`), `"2024-07-06"` is a Saturday (`5`) — see
[`weekday_name()`](weekday_name.md) for the same information spelled out.

## Notes

- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output, same as on a single frame — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- `0` is Monday, `6` is Sunday (ISO weekday numbering).
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::weekday`](../dataframe-timeline/weekday.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::weekday_name`](weekday_name.md): the same information as a
  full English name instead of a number.
- [`AtlasTimelineAccessor::weekend`](weekend.md): whether the weekday falls on a
  weekend.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="week.html">
    <span class="label">Previous</span>
    <span class="title">week</span>
  </a>
  <a class="next-link" href="weekday_name.html">
    <span class="label">Next</span>
    <span class="title">weekday_name</span>
  </a>
</div>
