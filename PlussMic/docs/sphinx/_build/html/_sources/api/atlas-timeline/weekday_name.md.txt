<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 28011b0f1377 -->

# AtlasTimelineAccessor::weekday_name

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::weekday_name() const -> Atlas
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with its
full English weekday name, as a `Note` (text) `Series` — the same transform
[`DataFrame::tl().weekday_name()`](../dataframe-timeline/weekday_name.md) applies to a
single frame, broadcast across the whole `Atlas` via
[`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by a `Note` Series holding each row's full weekday name
    (`"Monday"`-`"Sunday"`).

## Examples

Example 1. Extract the weekday name from a `Timeline` feature stacked across two
frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().weekday_name();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
================
     | ts       
----------------
0    | Wednesday
1    | Thursday 
----------------
[2 cs x 1 feat]
================
     | ts       
----------------
0    | Saturday 
1    | Wednesday
----------------
[2 cs x 1 feat]
```

## Notes

- Always the full English name (`"Monday"`, not `"Mon"`); use [`fmt("%a")`](fmt.md) for
  the abbreviated three-letter form.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output, same as on a single frame — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::weekday_name`](../dataframe-timeline/weekday_name.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::weekday`](weekday.md): the same information as a `0`-`6`
  number instead of a name.
- [`AtlasTimelineAccessor::month_name`](month_name.md): the equivalent for months.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday.html">
    <span class="label">Previous</span>
    <span class="title">weekday</span>
  </a>
  <a class="next-link" href="weekend.html">
    <span class="label">Next</span>
    <span class="title">weekend</span>
  </a>
</div>
