<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 740236387be6 -->

# AtlasTimelineAccessor::year

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::year() const -> Atlas
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with its
calendar year, as a numeric `Series` — the same transform
[`DataFrame::tl().year()`](../dataframe-timeline/year.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md). The
result is a new `Atlas` with the same `depth()` and the same depth labels as the
source — only each frame's time feature changes, one frame at a time.

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by its `Int64` year.

## Examples

Example 1. Extract the year from a `Timeline` feature stacked across two frames from
different years.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-03-15T09:10:00Z", "2024-07-01T09:40:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2023-11-20T09:10:00Z", "2023-12-31T09:40:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().year();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==========
     | ts  
----------
0    | 2024
1    | 2024
----------
[2 cs x 1 feat]
==========
     | ts  
----------
0    | 2023
1    | 2023
----------
[2 cs x 1 feat]
```

Each frame is converted independently: `frame(0)`'s `2024`-dated timestamps become
`2024`, and `frame(1)`'s `2023`-dated timestamps become `2023` — `year()` never mixes
values across the depth axis, only within each frame.

## Notes

- `Atlas::tl()` (like [`DataFrame::tl()`](../dataframe/tl.md)) requires **every**
  feature on each stacked frame to already be `Timeline`-kind; there is no
  public way to narrow to a subset of features first the way
  [`Atlas::features()`](../atlas/features.md) narrows for other operations. Build the
  atlas from frames that already contain only time features if this matters.
- A row whose underlying timestamp is `NaT` displays as `"NULL"` in the year output, the
  same as on a single frame.
- `year()` reads local calendar fields relative to whatever timezone each frame's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).

## See Also

- [`DataFrameTimelineAccessor::year`](../dataframe-timeline/year.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::month`](month.md), [`AtlasTimelineAccessor::day`](day.md): the
  other calendar-date components, extracted the same way.
- [`Atlas::map_frames`](../atlas/map_frames.md): the general per-frame transform this
  accessor is built on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekend.html">
    <span class="label">Previous</span>
    <span class="title">weekend</span>
  </a>
  <a class="next-link" href="yearday.html">
    <span class="label">Next</span>
    <span class="title">yearday</span>
  </a>
</div>
