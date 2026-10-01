<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c0308087dd05 -->

# AtlasTimelineAccessor::localize

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::localize(const TimelineLocalizeOptions& options = {}) const -> Atlas
```

## What It Does

Reinterprets every `Timeline` feature's *current* clock reading, in
**every frame of the stack**, as a naive local wall-clock time in `options.tz`, and
converts it to the true UTC instant that local reading corresponds to — the same
transform [`DataFrame::tl().localize()`](../dataframe-timeline/localize.md) applies to
a single frame, broadcast across the whole `Atlas` via
[`map_frames()`](../atlas/map_frames.md). **This changes the underlying instant**
(unlike [`tz()`](tz.md), which only relabels display).

## Parameters

**options** (`const TimelineLocalizeOptions&`)
    `tz` (default `"UTC"`; must be one of the four supported names — see
    [`tz()`](tz.md) — and cannot be empty), `fold` (`0` or `1`; disambiguates the
    daylight-saving fall-back overlap), and `nonexistent` (only `"shift_forward"` is a
    recognized handling mode for a spring-forward-gap reading; anything else, including
    the default `"raise"`, throws). See
    [`DataFrameTimelineAccessor::localize`](../dataframe-timeline/localize.md) for the
    full field breakdown and DST edge-case examples.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature's underlying instant recomputed as if its prior reading were local time in
    `options.tz`.

## Examples

Example 1. Localize a `Timeline` feature stacked across two frames to
`"America/New_York"`, confirming via `epoch()` that the underlying instant actually
moves.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

TimelineLocalizeOptions options;
options.tz = "America/New_York";
auto localized = atlas.tl().localize(options);

std::cout << localized.frame(0) << "\n" << localized.frame(1) << "\n";
std::cout << "epoch before:\n" << atlas.tl().epoch(TimeUnit::Seconds).frame(0);
std::cout << "epoch after:\n" << localized.tl().epoch(TimeUnit::Seconds).frame(0);
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

epoch before:
=================
     | ts        
-----------------
0    | 1706692530
1    | 1709250300
-----------------
[2 cs x 1 feat]
epoch after:
=================
     | ts        
-----------------
0    | 1706710530
1    | 1709268300
-----------------
[2 cs x 1 feat]
```

The displayed wall-clock text is identical before and after localizing — both frames
still read the same `09:15:30`/`23:45:00`/etc. — because the result is displayed under
the new `"America/New_York"` label, which reintroduces the same offset that was just
subtracted. The `epoch(Seconds)` values on `frame(0)` prove the underlying instant
actually moved: `18,000` seconds later for January (Eastern Standard Time, `-5:00`).

## Notes

- **`localize()` changes the underlying instant; [`tz()`](tz.md) never does.** Reach for
  `localize()` only when the current ticks are naive local readings that have never been
  anchored to a real instant; reach for `tz()` when the ticks are already correct and a
  different display timezone is all that's needed.
- `options.tz` only accepts the same four names as `tz()`, and cannot be empty.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row stays `NaT` regardless of `options`.
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::localize`](../dataframe-timeline/localize.md): the
  single-frame operation this method broadcasts across the stack, with the full set of
  `fold`/`nonexistent` DST edge-case examples.
- [`AtlasTimelineAccessor::tz`](tz.md): relabels display timezone without changing the
  instant — the operation most callers actually want.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="leap.html">
    <span class="label">Previous</span>
    <span class="title">leap</span>
  </a>
  <a class="next-link" href="minute.html">
    <span class="label">Next</span>
    <span class="title">minute</span>
  </a>
</div>
