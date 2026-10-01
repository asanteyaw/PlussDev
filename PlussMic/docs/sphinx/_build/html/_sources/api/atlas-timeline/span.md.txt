<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e7dae7b5b44c -->

# AtlasTimelineAccessor::span

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signatures

```cpp
AtlasTimelineAccessor::span(const Thread& other, const TimelineSpanOptions& options = {}) const -> Atlas
AtlasTimelineAccessor::span(const Thread& other, TimeUnit unit) const -> Atlas
AtlasTimelineAccessor::span(const pluss::Timeline& other, const TimelineSpanOptions& options = {}) const -> Atlas
AtlasTimelineAccessor::span(const pluss::Timeline& other, TimeUnit unit) const -> Atlas
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with the
elapsed distance between it and `other`, in `options.unit` (or plain `unit`) — the same
transform [`DataFrame::tl().span()`](../dataframe-timeline/span.md) applies to a single
frame, broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

```{important} No Atlas overload — the same `other` is applied to every frame
Unlike [`DataFrameTimelineAccessor::span`](../dataframe-timeline/span.md), which has a
`const DataFrame&` overload pairing each frame against its own matching `other` frame,
this accessor has **no `Atlas` overload**. Only a single `Thread` or `pluss::Timeline`
is accepted as `other`, and that *same* value is used as the right-hand side for
**every** frame in the stack — there is no way to pair each frame against a different
per-frame `other` in one call.
```

## Parameters

**other** (`const Thread&` / `const pluss::Timeline&`)
    The right-hand timestamp(s), applied identically to every frame in the stack — see
    the important callout above.

**options** (`const TimelineSpanOptions&`)
    `unit` (default `TimeUnit::Days`), `mode`, `anchor`, `signed_result`, and
    `calendar` — the same fields as
    [`DataFrameTimelineAccessor::span`](../dataframe-timeline/span.md)'s `options`; see
    that page for the full breakdown and worked examples of each.

**unit** (`TimeUnit`, convenience overload)
    Shorthand for `TimelineSpanOptions{.unit = unit}` with every other option left at
    its default.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by an `Int64` Series of elapsed distances to `other`.

## Examples

Example 1. Span two frames' single-row `Timeline` features against the same `other`
value.

```cpp
using namespace pluss::table;

auto s1 = DataFrame({{"ts", Thread(pluss::make_timeline({"2024-01-01T00:00:00Z"}))}});
auto s2 = DataFrame({{"ts", Thread(pluss::make_timeline({"2024-02-01T00:00:00Z"}))}});
auto atlas = Atlas({s1, s2});

auto other = Thread(pluss::make_timeline({"2024-02-01T00:00:00Z"}));
auto out = atlas.tl().span(other, TimeUnit::Days);
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | ts  
-----------
0    | 31  
-----------
[1 cs x 1 feat]
===========
     | ts  
-----------
0    | 0   
-----------
[1 cs x 1 feat]
```

`frame(0)`'s `"2024-01-01"` is 31 days before `other` (`"2024-02-01"`); `frame(1)`'s own
value is already exactly `other`, so its span is `0` — the same `other` value was
measured against both frames.

## Notes

- See the important callout above before reaching for this method to compare two
  atlases frame-by-frame — there is no such overload; `other` is one fixed value (or
  column) applied to every frame.
- Fixed-duration units (`Nanos`/`Micros`/`Millis`/`Seconds`/`Days`/`Weeks`) are plain
  elapsed-time differences. Calendar units (`Months`/`Years`) count completed calendar
  periods or crossed calendar boundaries, per `options.mode`. `BusinessDays` counts
  business days per `options.calendar`.
- Calendar units and `BusinessDays` require `*this` and `other` to share the same
  timezone — mismatched timezones throw; convert with [`tz()`](tz.md) first.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- If either side is `NaT` for a row, that row's span displays as `"NULL"`.
- Each frame is transformed independently against the same `other` — see the important
  callout above.

## See Also

- [`DataFrameTimelineAccessor::span`](../dataframe-timeline/span.md): the single-frame
  operation this method broadcasts across the stack — including the `DataFrame`
  overload this accessor lacks, and the full set of `mode`/`anchor`/`signed_result`/
  `calendar` worked examples.
- [`AtlasTimelineAccessor::epoch`](epoch.md): a fixed-unit distance from the Unix epoch
  to a single timeline, rather than between two timelines.
- [`AtlasTimelineAccessor::shift`](shift.md): move a timeline by a fixed amount,
  rather than measuring the distance to another one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shift.html">
    <span class="label">Previous</span>
    <span class="title">shift</span>
  </a>
  <a class="next-link" href="time.html">
    <span class="label">Next</span>
    <span class="title">time</span>
  </a>
</div>
