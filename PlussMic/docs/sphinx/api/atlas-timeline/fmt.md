<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ef0819d3efbd -->

# AtlasTimelineAccessor::fmt

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
Atlas AtlasTimelineAccessor::fmt(std::string_view pattern) const
```

## What It Does

Replaces every `Timeline` feature in **every frame of the stack** with a
custom text representation built from `pattern`, as a `Note` `Series` — the same
transform [`DataFrame::tl().fmt()`](../dataframe-timeline/fmt.md) applies to a single
frame, broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

## Parameters

**pattern** (`std::string_view`)
    A `strftime`-style format string. Recognized tokens: `%Y` (4-digit year),
    `%m`/`%d` (2-digit month/day), `%H`/`%M`/`%S` (2-digit hour/minute/second), `%f`
    (9-digit zero-padded nanoseconds), `%z` (UTC offset as `+HH:MM`/`-HH:MM`, never a
    bare `Z`), `%a`/`%A` (abbreviated/full weekday name), `%b`/`%B` (abbreviated/full
    month name), and `%%` (a literal `%`). See
    [`DataFrameTimelineAccessor::fmt`](../dataframe-timeline/fmt.md) for the full token
    reference. Cannot be empty.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by a `Note` Series holding each row's text rendered per `pattern`.

## Examples

Example 1. Format a `Timeline` feature with a custom pattern, stacked across two
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

auto out = atlas.tl().fmt("%d %b %Y");
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==================
     | ts         
------------------
0    | 31 Jan 2024
1    | 29 Feb 2024
------------------
[2 cs x 1 feat]
==================
     | ts         
------------------
0    | 06 Jul 2024
1    | 01 Mar 2023
------------------
[2 cs x 1 feat]
```

## Notes

- `pattern` cannot be empty — an empty pattern throws, same as
  [`DataFrameTimelineAccessor::fmt`](../dataframe-timeline/fmt.md).
- `%f` always emits all 9 nanosecond digits, zero-padded; `%z` always renders as a
  signed `HH:MM` offset, never a bare `"Z"`.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- A `NaT` row displays as `"NULL"` in the output regardless of `pattern` — see
  [`AtlasTimelineAccessor::isnat`](isnat.md).
- Each frame is transformed independently — see
  [`AtlasTimelineAccessor::year`](year.md) for a worked example of this per-frame
  independence.

## See Also

- [`DataFrameTimelineAccessor::fmt`](../dataframe-timeline/fmt.md): the single-frame
  operation this method broadcasts across the stack, and the full token reference.
- [`AtlasTimelineAccessor::iso`](iso.md): a fixed ISO-8601-with-offset pattern,
  equivalent to `fmt("%Y-%m-%dT%H:%M:%S.%f%z")`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floor.html">
    <span class="label">Previous</span>
    <span class="title">floor</span>
  </a>
  <a class="next-link" href="hour.html">
    <span class="label">Next</span>
    <span class="title">hour</span>
  </a>
</div>
