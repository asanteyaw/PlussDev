<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 562d4cf39f93 -->

# AtlasTimelineAccessor::leap

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
Atlas AtlasTimelineAccessor::leap() const
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with a
boolean `Series` flagging whether the row's calendar year is a leap year — the same
transform [`DataFrame::tl().leap()`](../dataframe-timeline/leap.md) applies to a single
frame, broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by a `Bool` Series: `true` when the row's year is a leap year,
    `false` otherwise.

## Examples

Example 1. Flag leap-year rows across a `Timeline` feature stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().leap();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | ts  
-----------
0    | true
1    | true
-----------
[2 cs x 1 feat]
============
     | ts   
------------
0    | true 
1    | false
------------
[2 cs x 1 feat]
```

Every `2024`-dated row flags `true`; `frame(1)`'s `"2023-03-01"` row flags `false`.

## Notes

- Reflects the *row's year*, not whether the row's exact date is February 29th — any
  `2024`-dated row flags `true`, since 2024 as a whole is a leap year.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output, same as on a single frame — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::leap`](../dataframe-timeline/leap.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::month_end`](month_end.md): where leap-year status actually
  changes February's last valid day.
- [`AtlasTimelineAccessor::yearday`](yearday.md): where leap years shift every
  day-of-year number from March onward.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="iso.html">
    <span class="label">Previous</span>
    <span class="title">iso</span>
  </a>
  <a class="next-link" href="localize.html">
    <span class="label">Next</span>
    <span class="title">localize</span>
  </a>
</div>
