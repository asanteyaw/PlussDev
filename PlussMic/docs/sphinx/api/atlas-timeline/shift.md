<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 634dbd255727 -->

# AtlasTimelineAccessor::shift

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const -> Atlas
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with each
row shifted forward (or backward, if `periods` is negative) by `periods * freq` — the
same transform [`DataFrame::tl().shift()`](../dataframe-timeline/shift.md) applies to a
single frame, broadcast across the whole `Atlas` via
[`map_frames()`](../atlas/map_frames.md). Month/year shifts are calendar-aware: a
day-of-month that doesn't exist in the target month is clamped down to that month's
last valid day.

## Parameters

**periods** (`std::int64_t`)
    How many `freq` units to shift by. Positive moves forward in time, negative moves
    backward.

**freq** (`std::string_view`)
    One of `"y"`/`"yr"`/`"year"`/`"years"`/`"annual"`, `"mo"`/`"mth"`/`"month"`/`"months"`,
    `"d"`/`"day"`/`"days"`, `"h"`/`"hr"`/`"hour"`/`"hours"`,
    `"t"`/`"min"`/`"minute"`/`"minutes"`, or `"s"`/`"sec"`/`"second"`/`"seconds"`
    (case-insensitive). Cannot be empty.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature shifted by `periods * freq`.

## Examples

Example 1. Shift a `Timeline` feature forward one calendar month, stacked across two
frames, showing day-of-month clamping in `frame(0)`.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().shift(1, "months");
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==========================
     | ts                 
--------------------------
0    | 2024-02-29 09:15:30
1    | 2024-03-29 23:45:00
--------------------------
[2 cs x 1 feat]
==========================
     | ts                 
--------------------------
0    | 2024-08-06 14:00:00
1    | 2023-04-01 00:00:00
--------------------------
[2 cs x 1 feat]
```

`frame(0)`'s January 31st shifted one month forward lands on February 29th (2024 is a
leap year) — day `31` is clamped down to February's last valid day rather than
overflowing into March.

## Notes

- `freq` accepts calendar units (`years`/`months`, day-of-month-clamping) and fixed
  units (`days`/`hours`/`minutes`/`seconds`, exact elapsed-time arithmetic with no
  clamping) — there's no `"week"` frequency.
- `freq` cannot be empty, and an unrecognized string throws
  `"Timeline::shift: unsupported frequency"`.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row stays `NaT` regardless of `periods`/`freq`.
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::shift`](../dataframe-timeline/shift.md): the
  single-frame operation this method broadcasts across the stack, with the full set of
  calendar-clamping and fixed-arithmetic worked examples.
- [`AtlasTimelineAccessor::span`](span.md): the elapsed distance between two
  timelines, rather than shifting one by a fixed amount.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="second.html">
    <span class="label">Previous</span>
    <span class="title">second</span>
  </a>
  <a class="next-link" href="span.html">
    <span class="label">Next</span>
    <span class="title">span</span>
  </a>
</div>
