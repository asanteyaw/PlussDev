<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f9d1b11b8104 -->

# SlotTimelineAccessor::localize

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::localize(const pluss::table::TimelineLocalizeOptions& options = {}) const
```

## What It Does

Reinterprets each value's *current* clock reading as a naive local
wall-clock time in `options.tz`, and converts it to the true UTC instant
that local reading corresponds to. **This changes the underlying
instant** (unlike [`tz()`](tz.md), which only relabels display). The
`options.tz` label attached by `localize()` also survives being read
back through any further `.tl()` call — see Example 1.

## Parameters

**options** (`const pluss::table::TimelineLocalizeOptions&`)
    `tz` (default `"UTC"`; must be one of the four supported names — see
    [`tz()`](tz.md) — and cannot be empty), `fold` (`0` or `1`; disambiguates
    the daylight-saving fall-back overlap), and `nonexistent` (only
    `"shift_forward"` is a recognized handling mode for a
    spring-forward-gap reading; anything else, including the default
    `"raise"`, throws).

## Returns

`Slot`
    A new slot with each value's underlying instant recomputed as if its
    prior reading were local time in `options.tz`.

## Examples

Example 1. Localize a timestamp to `"America/New_York"`. `epoch()`
confirms the instant actually moved; a follow-up `fmt()` shows the
shifted instant correctly rendered in NY-local wall-clock time.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")});

pluss::table::TimelineLocalizeOptions options;
options.tz = "America/New_York";
auto localized = slot.tl().localize(options);

auto before = slot.tl().epoch(pluss::table::TimeUnit::Seconds);
auto after = localized.tl().epoch(pluss::table::TimeUnit::Seconds);
std::cout << "epoch before: " << std::get<pluss::Series>(before.at(0)).to_vec<std::int64_t>()[0] << "\n";
std::cout << "epoch after: " << std::get<pluss::Series>(after.at(0)).to_vec<std::int64_t>()[0] << "\n";

auto fmted = localized.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << "displayed: " << std::get<pluss::Note>(fmted.at(0)).at(0);
```

### Output

```text
epoch before: 1706692530
epoch after: 1706710530
displayed: 2024-01-31 09:15:30
```

The 18,000-second delta (5 hours) confirms the instant genuinely moved — the
correct behavior for `"America/New_York"` in January (Eastern Standard
Time, `-5:00`). The displayed text, `09:15:30`, is the shifted instant
correctly rendered as NY-local wall-clock time.

## Notes

- **`localize()` changes the underlying instant; [`tz()`](tz.md) never
  does.**
- `options.tz` only accepts the same four names as `tz()`, and cannot be
  empty.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.
- Internally, a `Slot` stores each row's `Timeline` value independently
  (one scalar `Timeline` per row, not one shared column); the resulting
  per-row tz labels round-trip correctly through the `kindle::Stream`
  boundary every further `.tl()` call goes through, the same as
  [`tz()`](tz.md). If every value does not already share the same display
  timezone going into a `.tl()` call, the call throws rather than picking
  one arbitrarily.

## See Also

- [`SlotTimelineAccessor::tz`](tz.md): relabels display without changing
  the instant.
- [`SlotTimelineAccessor::epoch`](epoch.md): used above to confirm the
  instant shift independent of display.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
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
