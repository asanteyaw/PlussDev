<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c495b1a2b48d -->

# RegistryTimelineAccessor::localize

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::localize(const pluss::table::TimelineLocalizeOptions& options = {}) const
```

## What It Does

Reinterprets each bound field's *current* clock reading as a naive local
wall-clock time in `options.tz`, and converts it to the true UTC instant
that local reading corresponds to. **This changes the underlying instant**
(unlike [`tz()`](tz.md), which only relabels display).

```{danger} The instant shift persists; the resulting display label does not
Verified: the underlying epoch instant genuinely moves — reading
[`epoch()`](epoch.md) before and after `localize()` confirms the correct
UTC-offset delta (e.g. 18,000 seconds for `"America/New_York"` in January).
But the same round-trip limitation documented on [`tz()`](tz.md) applies
here too: the timezone label attached by `localize()` does not survive being
read back through any further `.tl()` call. The practical effect is
surprising in the *opposite* direction from `tz()`'s no-op: instead of the
display staying unchanged, a follow-up `fmt()`/calendar read shows the
*shifted* instant rendered as if it, too, were plain UTC — see Example 1,
where the displayed wall-clock text visibly changes by the timezone offset,
which is not what `localize()` is supposed to look like from the outside.
```

## Parameters

**options** (`const pluss::table::TimelineLocalizeOptions&`)
    `tz` (default `"UTC"`; must be one of the four supported names — see
    [`tz()`](tz.md) — and cannot be empty), `fold` (`0` or `1`; disambiguates
    the daylight-saving fall-back overlap), and `nonexistent` (only
    `"shift_forward"` is a recognized handling mode for a
    spring-forward-gap reading; anything else, including the default
    `"raise"`, throws).

## Returns

`Registry`
    A new registry with every bound field's underlying instant recomputed
    as if its prior reading were local time in `options.tz`.

## Examples

Example 1. Localize a timestamp to `"America/New_York"`. `epoch()` confirms
the instant actually moved; a follow-up `fmt()` shows the shifted instant
rendered without the expected NY relabeling — see the danger callout above.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})},
});

pluss::table::TimelineLocalizeOptions options;
options.tz = "America/New_York";
auto localized = rg.fields({"ts"}).tl().localize(options);

auto before = rg.fields({"ts"}).tl().epoch(pluss::table::TimeUnit::Seconds);
auto after = localized.fields({"ts"}).tl().epoch(pluss::table::TimeUnit::Seconds);
std::cout << "epoch before: " << std::get<pluss::Series>(before.fields("ts").at(0)).to_vec<std::int64_t>()[0] << "\n";
std::cout << "epoch after: " << std::get<pluss::Series>(after.fields("ts").at(0)).to_vec<std::int64_t>()[0] << "\n";

auto fmted = localized.fields("ts").tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << "displayed: " << std::get<pluss::Note>(fmted.at(0)).at(0);
```

### Output

```text
epoch before: 1706692530
epoch after: 1706710530
displayed: 2024-01-31 14:15:30
```

The 18,000-second delta (5 hours) confirms the instant genuinely moved — the
correct behavior for `"America/New_York"` in January (Eastern Standard
Time, `-5:00`). But the displayed text, `14:15:30`, is the *shifted* instant
rendered as bare UTC, not `09:15:30` relabeled as NY-local — see the danger
callout above.

## Notes

- **`localize()` changes the underlying instant; [`tz()`](tz.md) never
  does.** That part works correctly and persists.
- See the danger callout above for the display-label limitation that
  applies on top of the correct instant shift.
- `options.tz` only accepts the same four names as `tz()`, and cannot be
  empty.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::tz`](tz.md): relabels display without
  changing the instant — and documents the same underlying round-trip
  limitation.
- [`RegistryTimelineAccessor::epoch`](epoch.md): used above to confirm the
  instant shift independent of display.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

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
