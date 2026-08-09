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
(unlike [`tz()`](tz.md), which only relabels display). The `options.tz`
label attached by `localize()` also survives being read back through any
further `.tl()` call — see Example 1.

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
correctly rendered in NY-local wall-clock time.

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
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.
- Internally, each bound field stores its `Timeline` values independently
  (one scalar `Timeline` per row, not one shared column); the resulting
  per-row tz labels round-trip correctly through the `kindle::Stream`
  boundary every further `.tl()` call goes through. If every row does not
  already share the same display timezone going into a `.tl()` call, the
  call throws rather than picking one arbitrarily.

## See Also

- [`RegistryTimelineAccessor::tz`](tz.md): relabels display without
  changing the instant.
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
