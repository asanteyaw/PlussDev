<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bd5c99fbf600 -->

# AtlasTimelineAccessor::time

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::time() const -> Atlas
```

## What It Does

Replaces every `Timeline`/`TimeIndex` feature in **every frame of the stack** with just
its time-of-day portion — the date is reset to the Unix epoch day (`1970-01-01`), and
the clock time is kept, in local wall time — the same transform
[`DataFrame::tl().time()`](../dataframe-timeline/time.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature's date portion reset to `1970-01-01`, keeping only the time-of-day.

## Examples

Example 1. Keep only the time-of-day from a `Timeline` feature stacked across two
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

auto out = atlas.tl().time();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==========================
     | ts                 
--------------------------
0    | 1970-01-01 09:15:30
1    | 1970-01-01 23:45:00
--------------------------
[2 cs x 1 feat]
==========================
     | ts                 
--------------------------
0    | 1970-01-01 14:00:00
1    | 1970-01-01 00:00:00
--------------------------
[2 cs x 1 feat]
```

Every row's date is now `1970-01-01`; each original time-of-day is preserved exactly.

## Notes

- The result is still `Timeline`/`TimeIndex`-kind with a fixed date component — it's
  meant for comparing or grouping by time-of-day, not as a display format. Use
  [`fmt("%H:%M:%S")`](fmt.md) for a bare text time.
- Operates in **local wall time**, per each frame's `Timeline`'s current timezone.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`/`TimeIndex`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row stays `NaT` (displays as `"NULL"`).

## See Also

- [`DataFrameTimelineAccessor::time`](../dataframe-timeline/time.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::date`](date.md): the mirror-image operation, keeping the
  date and zeroing the time-of-day.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="span.html">
    <span class="label">Previous</span>
    <span class="title">span</span>
  </a>
  <a class="next-link" href="tz.html">
    <span class="label">Next</span>
    <span class="title">tz</span>
  </a>
</div>
