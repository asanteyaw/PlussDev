<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4de1cfdd6c46 -->

# AtlasTimelineAccessor::iso

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::iso() const -> Atlas
```

## What It Does

Replaces every `Timeline`/`TimeIndex` feature in **every frame of the stack** with a
fixed ISO-8601-with-offset text representation, as a `Note` `Series` — the same
transform [`DataFrame::tl().iso()`](../dataframe-timeline/iso.md) applies to a single
frame, broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).
Equivalent to [`fmt("%Y-%m-%dT%H:%M:%S.%f%z")`](fmt.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by a `Note` Series holding each row as
    `"YYYY-MM-DDTHH:MM:SS.fffffffff+HH:MM"`.

## Examples

Example 1. Render a `Timeline` feature as ISO-8601 text, stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().iso();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
=============================
     | ts                    
-----------------------------
0    | 2024-01-31T09:15:30.0~
1    | 2024-02-29T23:45:00.0~
-----------------------------
[2 cs x 1 feat]
=============================
     | ts                    
-----------------------------
0    | 2024-07-06T14:00:00.0~
1    | 2023-03-01T00:00:00.0~
-----------------------------
[2 cs x 1 feat]
```

Each cell's full value is 35 characters long (e.g. `"2024-01-31T09:15:30.000000000+00:00"`),
but the box render clips any cell wider than 22 characters with a trailing `~`.

## Notes

- Always includes the full 9-digit nanosecond fraction and a signed `%z` offset — use
  [`fmt()`](fmt.md) with your own pattern for a more compact string.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`/`TimeIndex`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output, same as on a single frame — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::iso`](../dataframe-timeline/iso.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::fmt`](fmt.md): the general form this method is a fixed
  pattern of.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnat.html">
    <span class="label">Previous</span>
    <span class="title">isnat</span>
  </a>
  <a class="next-link" href="leap.html">
    <span class="label">Next</span>
    <span class="title">leap</span>
  </a>
</div>
