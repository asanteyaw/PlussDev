<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e9747cee96eb -->

# AtlasTimelineAccessor::index

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::index(bool require_sorted = true) const -> Atlas
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with an equivalent
`TimeIndex` — the same transform
[`DataFrame::tl().index()`](../dataframe-timeline/index.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md). A
feature already `TimeIndex`-kind is passed through unchanged.

```{important} Sortedness is checked per frame, independently
With the default `require_sorted = true`, each frame's time feature must be
non-decreasing (and non-`NaT`) **on its own** — the check does not span across frames.
A stack where `frame(0)` is sorted but `frame(1)` isn't still throws, on `frame(1)`'s
own values, the moment `map_frames()` reaches that frame.
```

## Parameters

**require_sorted** (`bool`, default `true`)
    When `true` (the default), every value in each frame's feature must be
    non-decreasing and non-`NaT`, or the call throws for that frame. When `false`, the
    conversion happens without checking order or `NaT` for any frame.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's
    `Timeline` feature converted to `TimeIndex`.

## Examples

Example 1. Convert two independently-sorted `Timeline` features to `TimeIndex`.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2023-01-01T00:00:00Z", "2023-01-02T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().index();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
=================
     | ts        
-----------------
0    | 2024-01-01
1    | 2024-01-02
-----------------
[2 cs x 1 feat]
=================
     | ts        
-----------------
0    | 2023-01-01
1    | 2023-01-02
-----------------
[2 cs x 1 feat]
```

Example 2. One frame's feature is out of order; the call throws on that frame even
though the other frame is fine.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2024-01-02T00:00:00Z", "2024-01-01T00:00:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Thread(pluss::make_timeline({"2023-01-01T00:00:00Z", "2023-01-02T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

try {
    auto out = atlas.tl().index();
} catch (const std::exception& e) {
    std::string msg(e.what());
    std::cout << "threw: " << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
threw: TimeIndex must be sorted (non-decreasing)
```

## Notes

- See the important callout above: the `require_sorted` check runs independently per
  frame — it never compares values across frames.
- `require_sorted = false` skips the check entirely for every frame — use it only when
  the data's order doesn't matter for what comes next, since downstream duration-based
  operations generally assume a monotonic axis.
- A feature that's already `TimeIndex`-kind is returned as-is, regardless of
  `require_sorted`.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`/`TimeIndex`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).

## See Also

- [`DataFrameTimelineAccessor::index`](../dataframe-timeline/index.md): the
  single-frame operation this method broadcasts across the stack, including the
  distinct "unsorted" vs. "contains NaT" throw messages.
- [`Atlas::ts`](../atlas/ts.md): the accessor whose duration-based methods expect a
  `TimeIndex` axis like the one this method produces.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html">
    <span class="label">Previous</span>
    <span class="title">hour</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
