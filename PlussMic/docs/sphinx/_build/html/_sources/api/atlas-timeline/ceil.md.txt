<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3f829fd698b7 -->

# AtlasTimelineAccessor::ceil

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::ceil(std::string_view unit) const -> Atlas
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with each
row rounded *up* to the nearest `unit` boundary, in local wall time — the same
transform [`DataFrame::tl().ceil()`](../dataframe-timeline/ceil.md) applies to a single
frame, broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`, `"H"`/`"h"`/`"hour"`,
    or `"D"`/`"d"`/`"day"`. Cannot be empty. An unsupported unit throws.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature ceiled to `unit`.

## Examples

Example 1. Ceil a `Timeline` feature to the nearest hour, stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().ceil("H");
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 10:00:00
1    | 2024-03-01 00:00:00
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

`frame(0)`'s `09:15:30` and `23:45:00` both advance to the start of their *next* hour
(`10:00:00`, and rolling over the day to `2024-03-01 00:00:00`); `frame(1)`'s rows were
already exactly on the hour, so they're left unchanged rather than advanced further.

## Notes

- A row already exactly on a `unit` boundary is left unchanged, not advanced to the
  *next* boundary — see `frame(1)` above.
- Ceiling happens in **local wall time** (per each frame's `Timeline`'s current
  timezone), then maps back.
- `unit` cannot be empty, and only accepts the fixed clock units listed above — an
  unrecognized unit throws `"Timeline: unsupported unit (expected S/min/H/D)"`.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row stays `NaT` (displays as `"NULL"`) regardless of `unit`.

## See Also

- [`DataFrameTimelineAccessor::ceil`](../dataframe-timeline/ceil.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::floor`](floor.md), [`AtlasTimelineAccessor::round`](round.md):
  round down, or to the nearest boundary, instead of always up.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="date.html">
    <span class="label">Next</span>
    <span class="title">date</span>
  </a>
</div>
