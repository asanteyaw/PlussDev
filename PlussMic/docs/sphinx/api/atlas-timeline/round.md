<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: be7ae21618bf -->

# AtlasTimelineAccessor::round

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::round(std::string_view unit) const -> Atlas
```

## What It Does

Replaces every `Timeline`/`TimeIndex` feature in **every frame of the stack** with each
row rounded to the *nearest* `unit` boundary, in local wall time — the same transform
[`DataFrame::tl().round()`](../dataframe-timeline/round.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`, `"H"`/`"h"`/`"hour"`,
    or `"D"`/`"d"`/`"day"`. Cannot be empty. An unsupported unit throws.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature rounded to the nearest `unit`.

## Examples

Example 1. Round a `Timeline` feature to the nearest hour, stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().round("H");
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:00:00
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

`09:15:30` is closer to `09:00:00` than `10:00:00`, so it rounds down; `23:45:00` is
closer to the next day's `00:00:00` than back to `23:00:00`, so it rounds up and rolls
into `2024-03-01`.

## Notes

- Rounding happens in **local wall time** (per each frame's `Timeline`'s current
  timezone), then maps back.
- A value exactly halfway between two boundaries resolves **up**, never down — see
  [`DataFrameTimelineAccessor::round`](../dataframe-timeline/round.md) for a worked
  tie-break example.
- `unit` cannot be empty, and only accepts the fixed clock units listed above — an
  unrecognized unit throws `"Timeline: unsupported unit (expected S/min/H/D)"`.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`/`TimeIndex`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row stays `NaT` (displays as `"NULL"`) regardless of `unit`.

## See Also

- [`DataFrameTimelineAccessor::round`](../dataframe-timeline/round.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::floor`](floor.md), [`AtlasTimelineAccessor::ceil`](ceil.md):
  always down, or always up, instead of nearest.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quarter.html">
    <span class="label">Previous</span>
    <span class="title">quarter</span>
  </a>
  <a class="next-link" href="second.html">
    <span class="label">Next</span>
    <span class="title">second</span>
  </a>
</div>
