<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: efca9bd8760c -->

# AtlasTimelineAccessor::utc

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::utc() const -> Atlas
```

## What It Does

Replaces every `Timeline` feature's display timezone in **every frame of
the stack** with `"UTC"` — the same transform
[`DataFrame::tl().utc()`](../dataframe-timeline/utc.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).
Exactly equivalent to [`tz("UTC")`](tz.md); the underlying instant never changes, only
the display timezone.

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature's display timezone switched to `"UTC"`.

## Examples

Example 1. Convert a `"America/New_York"`-labeled `Timeline` feature, stacked across
two frames, back to UTC display.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto ny = atlas.tl().tz("America/New_York");
auto out = ny.tl().utc();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:15:30
1    | 2024-02-29 23:45:00
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

`utc()` undoes exactly what the earlier `tz("America/New_York")` call did — same
instants throughout, only the display timezone changes back, matching the atlas's
original values.

## Notes

- Purely a display-timezone relabel — the underlying instant is unchanged, so chaining
  `.tl().tz(name).tl().utc()` always round-trips back to the original displayed values.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row stays `NaT`.
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::utc`](../dataframe-timeline/utc.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::tz`](tz.md): the general form this method is a fixed
  `"UTC"` shorthand for.
- [`AtlasTimelineAccessor::localize`](localize.md): the operation that actually
  changes the underlying instant, unlike this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz.html">
    <span class="label">Previous</span>
    <span class="title">tz</span>
  </a>
  <a class="next-link" href="valid.html">
    <span class="label">Next</span>
    <span class="title">valid</span>
  </a>
</div>
