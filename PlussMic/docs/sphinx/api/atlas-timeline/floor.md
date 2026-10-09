<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a8da231b4743 -->

# AtlasTimelineAccessor::floor

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::floor(std::string_view unit) const -> Atlas
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with each
row rounded *down* to the nearest `unit` boundary, in local wall time — the same
transform [`DataFrame::tl().floor()`](../dataframe-timeline/floor.md) applies to a
single frame, broadcast across the whole `Atlas` via
[`map_frames()`](../atlas/map_frames.md).

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`, `"H"`/`"h"`/`"hour"`,
    or `"D"`/`"d"`/`"day"`. Cannot be empty. An unsupported unit throws.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature floored to `unit`.

## Examples

Example 1. Floor a `Timeline` feature to the nearest hour, stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().floor("H");
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:00:00
1    | 2024-02-29 23:00:00
--------------------------
[2 cs x 1 feat]
==========================
     | ts                 
--------------------------
0    | 2024-07-06 14:00:00
1    | 2023-03-01 00:00:00
--------------------------
[2 cs x 1 feat]
```

`frame(0)`'s `09:15:30` drops to `09:00:00`; `frame(1)`'s rows were already exactly on
the hour, so they're unchanged.

## Notes

- Flooring happens in **local wall time** (per each frame's `Timeline`'s current
  timezone), then maps back.
- `unit` cannot be empty, and only accepts the fixed clock units listed above — an
  unrecognized unit throws `"Timeline: unsupported unit (expected S/min/H/D)"`.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row stays `NaT` (displays as `"NULL"`) regardless of `unit`.
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::floor`](../dataframe-timeline/floor.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::ceil`](ceil.md), [`AtlasTimelineAccessor::round`](round.md):
  round up, or to the nearest boundary, instead of always down.
- [`AtlasTimelineAccessor::date`](date.md): shorthand for `floor("D")`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch.html">
    <span class="label">Previous</span>
    <span class="title">epoch</span>
  </a>
  <a class="next-link" href="fmt.html">
    <span class="label">Next</span>
    <span class="title">fmt</span>
  </a>
</div>
