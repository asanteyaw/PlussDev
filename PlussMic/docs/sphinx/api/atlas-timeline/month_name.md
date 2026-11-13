<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f0e77687cf7e -->

# AtlasTimelineAccessor::month_name

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
Atlas AtlasTimelineAccessor::month_name() const
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with its
full English month name, as a `Note` (text) `Series` — the same transform
[`DataFrame::tl().month_name()`](../dataframe-timeline/month_name.md) applies to a
single frame, broadcast across the whole `Atlas` via
[`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by a `Note` Series holding each row's full month name
    (`"January"`-`"December"`).

## Examples

Example 1. Extract the month name from a `Timeline` feature stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().month_name();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===============
     | ts      
---------------
0    | January 
1    | February
---------------
[2 cs x 1 feat]
============
     | ts   
------------
0    | July 
1    | March
------------
[2 cs x 1 feat]
```

## Notes

- Always the full English name (`"January"`, not `"Jan"`); use [`fmt("%b")`](fmt.md) for
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

- [`DataFrameTimelineAccessor::month_name`](../dataframe-timeline/month_name.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::month`](month.md): the same information as a `1`-`12` number
  instead of a name.
- [`AtlasTimelineAccessor::weekday_name`](weekday_name.md): the equivalent for weekdays.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_end.html">
    <span class="label">Previous</span>
    <span class="title">month_end</span>
  </a>
  <a class="next-link" href="month_start.html">
    <span class="label">Next</span>
    <span class="title">month_start</span>
  </a>
</div>
