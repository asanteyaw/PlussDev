<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 01c29b737644 -->

# ArchiveTimelineAccessor::localize

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::localize(const pluss::table::TimelineLocalizeOptions& options = {}) const
```

## What It Does

Reinterprets each bound key's *current* clock reading as a naive local
wall-clock time in `options.tz`, and converts it to the true UTC instant that
local reading corresponds to. **This changes the underlying instant** (unlike
[`tz()`](tz.md), which only relabels display). The `options.tz` label attached
by `localize()` also survives being read back through any further `.tl()` call
— see Example 1. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::localize`](../catalog-timeline/localize.md).

```{important} Binding and kind-check rules are inherited from Archive::tl()
This method's key-binding (from `catalog(0)`) and lazy Timeline/TimeIndex-kind check
follow exactly the rules documented on [`Archive::tl`](../archive/tl.md) — see that
page's two important callouts for the full explanation. `ArchiveTimelineAccessor`
requires every bound key to be `Timeline`- or `TimeIndex`-kind, checked the moment any
method (including this one) is called.
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

`Archive`
    A new archive of the same depth, with every bound key's underlying instant recomputed as if
    its prior reading were local time in `options.tz`, in every catalog.

## Examples

Example 1. Localize a timestamp to `"America/New_York"`. `epoch()` confirms
the instant actually moved; a follow-up `fmt()` shows the shifted instant
correctly rendered in NY-local wall-clock time. Delegation to `CatalogTimelineAccessor::localize` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::localize`](../catalog-timeline/localize.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
auto archive = pluss::kindle::Archive({c0});

pluss::table::TimelineLocalizeOptions options;
options.tz = "America/New_York";
auto localized = archive.tl().localize(options);

auto before = archive.tl().epoch(pluss::table::TimeUnit::Seconds);
auto after = localized.tl().epoch(pluss::table::TimeUnit::Seconds);
std::cout << "epoch before: " << before.catalog(0).keys("ts").as_series().to_vec<std::int64_t>()[0] << "\n";
std::cout << "epoch after: " << after.catalog(0).keys("ts").as_series().to_vec<std::int64_t>()[0] << "\n";

auto fmted = localized.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << "displayed: " << fmted.catalog(0).keys("ts").as_note().at(0);
```

### Output

```text
epoch before: 1706692530
epoch after: 1706710530
displayed: 2024-01-31 09:15:30
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::localize`](../catalog-timeline/localize.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::localize`](../catalog-timeline/localize.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

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
