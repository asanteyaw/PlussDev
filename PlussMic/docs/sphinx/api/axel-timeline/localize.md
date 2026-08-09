<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d3332381d67c -->

# AxelTimelineAccessor::localize

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::localize(const pluss::table::TimelineLocalizeOptions& options = {}) const
```

## What It Does

Reinterprets each bound field's *current* clock reading as a naive
local wall-clock time in `options.tz`, and converts it to the true UTC
instant that local reading corresponds to, in every registry frame.
**This changes the underlying instant** (unlike [`tz()`](tz.md), which
only relabels display). The `options.tz` label attached by
`localize()` also survives being read back through any further `.tl()`
call — see Example 1. Applies
[`RegistryTimelineAccessor::localize`](../registry-timeline/localize.md)
to every frame via [`Axel::map`](../axel/map.md).

## Parameters

**options** (`const pluss::table::TimelineLocalizeOptions&`)
    `tz` (default `"UTC"`; must be one of the four supported names —
    see [`tz()`](tz.md) — and cannot be empty), `fold` (`0` or `1`;
    disambiguates the daylight-saving fall-back overlap), and
    `nonexistent` (only `"shift_forward"` is a recognized handling mode
    for a spring-forward-gap reading; anything else, including the
    default `"raise"`, throws).

## Returns

`Axel`
    A new axel with every bound field's underlying instant, in every
    frame, recomputed as if its prior reading were local time in
    `options.tz`.

## Examples

Example 1. Localize a timestamp to `"America/New_York"`. `epoch()`
confirms the instant actually moved; a follow-up `fmt()` shows the
shifted instant correctly rendered in NY-local wall-clock time.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})}});
auto axel = pluss::bundle::Axel({r0});

pluss::table::TimelineLocalizeOptions options;
options.tz = "America/New_York";
auto localized = axel.fields({"ts"}).tl().localize(options);

auto before = axel.fields({"ts"}).tl().epoch(pluss::table::TimeUnit::Seconds);
auto after = localized.fields({"ts"}).tl().epoch(pluss::table::TimeUnit::Seconds);
std::cout << "epoch before: " << std::get<pluss::Series>(before.registry(0).fields("ts").at(0)).to_vec<std::int64_t>()[0] << "\n";
std::cout << "epoch after: " << std::get<pluss::Series>(after.registry(0).fields("ts").at(0)).to_vec<std::int64_t>()[0] << "\n";

auto fmted = localized.fields({"ts"}).tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << "displayed: " << std::get<pluss::Note>(fmted.registry(0).fields("ts").at(0)).at(0);
```

### Output

```text
epoch before: 1706692530
epoch after: 1706710530
displayed: 2024-01-31 09:15:30
```

The 18,000-second delta (5 hours) confirms the instant genuinely
moved — the correct behavior for `"America/New_York"` in January
(Eastern Standard Time, `-5:00`). The displayed text, `09:15:30`, is
the shifted instant correctly rendered as NY-local wall-clock time.

## Notes

- **`localize()` changes the underlying instant; [`tz()`](tz.md) never
  does.**
- `options.tz` only accepts the same four names as `tz()`, and cannot
  be empty.
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.
- Internally, each `Registry` frame stores each row's `Timeline` value
  independently (one scalar `Timeline` per row, not one shared column);
  the resulting per-row tz labels round-trip correctly through the
  `kindle::Stream` boundary every further `.tl()` call goes through,
  the same as [`tz()`](tz.md).

## See Also

- [`AxelTimelineAccessor::tz`](tz.md): relabels display without
  changing the instant.
- [`AxelTimelineAccessor::epoch`](epoch.md): used above to confirm the
  instant shift independent of display.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="leap.html">
    <span class="label">Previous</span>
    <span class="title">leap</span>
  </a>
  <a class="next-link" href="minute.html">
    <span class="label">Next</span>
    <span class="title">minute</span>
  </a>
</div>
