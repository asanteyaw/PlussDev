<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: db284ee90434 -->

# ArchiveTimelineAccessor::period

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::period(std::string_view freq) const
```

## What It Does

Replaces each bound key's timestamp with the *start* of the `freq`-sized period
it falls within, replacing each bound key's values in place. For the fixed
clock units, this is equivalent to [`floor(freq)`](floor.md); for the calendar
units it accepts, it snaps to the start of that calendar period (e.g. the
first of the month). Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::period`](../catalog-timeline/period.md).

```{important} Binding and kind-check rules are inherited from Archive::tl()
This method's key-binding (from `catalog(0)`) and lazy Timeline-kind check
follow exactly the rules documented on [`Archive::tl`](../archive/tl.md) — see that
page's two important callouts for the full explanation. `ArchiveTimelineAccessor`
requires every bound key to be `Timeline`-kind, checked the moment any
method (including this one) is called.
```

## Parameters

**freq** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported frequency throws.

## Returns

`Archive`
    A new archive of the same depth, with every bound key's timestamps replaced by their period
    start, in every catalog.

## Examples

Example 1. Snap a mid-day timestamp to its containing day's start. Delegation to `CatalogTimelineAccessor::period` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::period`](../catalog-timeline/period.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().period("D");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.catalog(0).keys("ts").as_note().at(0);
```

### Output

```text
2024-01-31 00:00:00
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::period`](../catalog-timeline/period.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::period`](../catalog-timeline/period.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_start.html">
    <span class="label">Previous</span>
    <span class="title">month_start</span>
  </a>
  <a class="next-link" href="quarter.html">
    <span class="label">Next</span>
    <span class="title">quarter</span>
  </a>
</div>
