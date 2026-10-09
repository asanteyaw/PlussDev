<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b144283e785a -->

# AtlasTimelineAccessor::tz

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::tz(std::string_view name) const -> Atlas
```

## What It Does

Replaces every `Timeline` feature's *display* timezone in **every frame of
the stack** with `name` — the same transform
[`DataFrame::tl().tz()`](../dataframe-timeline/tz.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).
**The underlying instant is unchanged** — only the timezone used to compute local
calendar fields (year/month/day/hour, and box-table display) shifts.

## Parameters

**name** (`std::string_view`)
    The target timezone. Only `"UTC"` (also the empty string), `"America/Edmonton"`,
    `"America/New_York"`, and `"Europe/London"` are recognized (case-insensitive); any
    other name throws.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature's display timezone switched to `name`.

## Examples

Example 1. Relabel a UTC `Timeline` feature stacked across two frames to
`"America/New_York"`.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}))},
});
auto q2 = DataFrame({
    {"ts", Slate(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}))},
});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().tz("America/New_York");
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 04:15:30
1    | 2024-02-29 18:45:00
--------------------------
[2 cs x 1 feat]
==========================
     | ts                 
--------------------------
0    | 2024-07-06 10:00:00
1    | 2023-02-28 19:00:00
--------------------------
[2 cs x 1 feat]
```

Every row is the *same instant* as before — only its displayed local wall time changes,
in each frame independently: `-5:00` in January (Eastern Standard Time) and `-4:00` in
July (Eastern Daylight Time).

## Notes

- Only four timezone names are supported: `"UTC"` (or empty string),
  `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` — any other name
  throws. There is no general IANA tzdata lookup.
- **`tz()` never changes the underlying instant** — it only relabels which timezone is
  used to interpret it for display and field extraction. Use
  [`localize()`](localize.md) if the data is naive local wall-clock text that needs to
  be *anchored* to a real instant for the first time.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row stays `NaT` regardless of `name`.
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::tz`](../dataframe-timeline/tz.md): the single-frame
  operation this method broadcasts across the stack, with the full set of timezone
  examples.
- [`AtlasTimelineAccessor::utc`](utc.md): shorthand for `tz("UTC")`.
- [`AtlasTimelineAccessor::localize`](localize.md): reinterprets naive ticks as local
  time in a timezone, shifting the underlying instant — fundamentally different from
  this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="time.html">
    <span class="label">Previous</span>
    <span class="title">time</span>
  </a>
  <a class="next-link" href="utc.html">
    <span class="label">Next</span>
    <span class="title">utc</span>
  </a>
</div>
