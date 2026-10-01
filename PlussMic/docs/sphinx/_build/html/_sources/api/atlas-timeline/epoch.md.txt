<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 279897f57062 -->

# AtlasTimelineAccessor::epoch

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::epoch(TimeUnit unit = TimeUnit::Nanos) const -> Atlas
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with its
Unix epoch value in `unit`, as a numeric `Series` — the same transform
[`DataFrame::tl().epoch()`](../dataframe-timeline/epoch.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

## Parameters

**unit** (`TimeUnit`, default `TimeUnit::Nanos`)
    One of the fixed-duration units: `Nanos`, `Micros`, `Millis`, `Seconds`, or `Days`.
    Passing a span-only unit (`Weeks`, `Months`, `Years`, `BusinessDays` — meant for
    [`span()`](span.md), not a fixed epoch count) throws.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by an `Int64` Series of epoch counts in `unit`.

## Examples

Example 1. Extract the epoch second from a `Timeline` feature stacked across two
frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().epoch(TimeUnit::Seconds);
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
=================
     | ts        
-----------------
0    | 1706692530
1    | 1709250300
-----------------
[2 cs x 1 feat]
=================
     | ts        
-----------------
0    | 1720274400
1    | 1677628800
-----------------
[2 cs x 1 feat]
```

## Notes

- The default unit is `Nanos`, not `Seconds` — see
  [`DataFrameTimelineAccessor::epoch`](../dataframe-timeline/epoch.md) for a worked
  example of the default confirmed against `Seconds`.
- Only fixed-duration units are accepted; passing `Weeks`, `Months`, `Years`, or
  `BusinessDays` throws — use [`span()`](span.md) for calendar-relative distances
  instead.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output, same as on a single frame — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::epoch`](../dataframe-timeline/epoch.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::span`](span.md): elapsed distance *between* two timelines,
  including calendar-relative units `epoch()` doesn't support.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="day.html">
    <span class="label">Previous</span>
    <span class="title">day</span>
  </a>
  <a class="next-link" href="floor.html">
    <span class="label">Next</span>
    <span class="title">floor</span>
  </a>
</div>
