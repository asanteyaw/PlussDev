<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8ac4e8cea853 -->

# ArchiveTimelineAccessor::epoch

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::epoch(pluss::table::TimeUnit unit = pluss::table::TimeUnit::Nanos) const
```

## What It Does

Extracts each bound key's timestamp as a Unix epoch value in `unit`, replacing
each `Timeline`-kind key with an `Int64` `Series` key. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::epoch`](../catalog-timeline/epoch.md).

```{important} Binding and kind-check rules are inherited from Archive::tl()
This method's key-binding (from `catalog(0)`) and lazy Timeline-kind check
follow exactly the rules documented on [`Archive::tl`](../archive/tl.md) — see that
page's two important callouts for the full explanation. `ArchiveTimelineAccessor`
requires every bound key to be `Timeline`-kind, checked the moment any
method (including this one) is called.
```

## Parameters

**unit** (`pluss::table::TimeUnit`, defaults to `pluss::table::TimeUnit::Nanos`)
    One of the fixed-duration units: `Nanos`, `Micros`, `Millis`, `Seconds`, or
    `Days`. Passing a span-only unit (`Weeks`, `Months`, `Years`,
    `BusinessDays`) throws.

## Returns

`Archive`
    A new archive of the same depth, with every bound key replaced by an `Int64` `Series` key of
    epoch counts in `unit`, in every catalog.

## Examples

Example 1. Default call reports nanoseconds — see the important callout above. Delegation to `CatalogTimelineAccessor::epoch` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::epoch`](../catalog-timeline/epoch.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().epoch();
std::cout << out.catalog(0).keys("ts").as_series().to_vec<std::int64_t>()[0];
```

### Output

```text
1706692530000000000
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::epoch`](../catalog-timeline/epoch.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::epoch`](../catalog-timeline/epoch.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="day.html">
    <span class="label">Previous</span>
    <span class="title">day</span>
  </a>
  <a class="next-link" href="floor.html">
    <span class="label">Next</span>
    <span class="title">floor</span>
  </a>
</div>
