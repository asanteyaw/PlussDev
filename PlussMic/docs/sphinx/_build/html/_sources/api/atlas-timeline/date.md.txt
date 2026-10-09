<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ca74fd263421 -->

# AtlasTimelineAccessor::date

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::date() const -> Atlas
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with just
its date portion (time-of-day dropped to midnight), in local wall time — the same
transform [`DataFrame::tl().date()`](../dataframe-timeline/date.md) applies to a single
frame, broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).
Equivalent to [`floor("D")`](floor.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature's clock-time portion zeroed out.

## Examples

Example 1. Strip the time-of-day from a `Timeline` feature stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().date();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
=================
     | ts        
-----------------
0    | 2024-01-31
1    | 2024-02-29
-----------------
[2 cs x 1 feat]
=================
     | ts        
-----------------
0    | 2024-07-06
1    | 2023-03-01
-----------------
[2 cs x 1 feat]
```

Every row's clock-time portion is now `00:00:00`, so both frames display without any
time-of-day text at all.

## Notes

- Operates in **local wall time**, per each frame's `Timeline`'s current timezone.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row stays `NaT` (displays as `"NULL"`).
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::date`](../dataframe-timeline/date.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::time`](time.md): the mirror-image operation, keeping the
  time-of-day and zeroing the date.
- [`AtlasTimelineAccessor::floor`](floor.md): the general form `date()` is a fixed `"D"`
  unit of.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ceil.html">
    <span class="label">Previous</span>
    <span class="title">ceil</span>
  </a>
  <a class="next-link" href="day.html">
    <span class="label">Next</span>
    <span class="title">day</span>
  </a>
</div>
