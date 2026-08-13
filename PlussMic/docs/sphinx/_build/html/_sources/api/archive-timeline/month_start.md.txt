<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 314c120b9e10 -->

# ArchiveTimelineAccessor::month_start

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::month_start() const
```

## What It Does

Flags whether each bound key's timestamp falls on the first day of its calendar
month, replacing each `Timeline`- or `TimeIndex`-kind key with a `Bool` `Series`
key. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::month_start`](../catalog-timeline/month_start.md).

```{important} Binding and kind-check rules are inherited from Archive::tl()
This method's key-binding (from `catalog(0)`) and lazy Timeline/TimeIndex-kind check
follow exactly the rules documented on [`Archive::tl`](../archive/tl.md) — see that
page's two important callouts for the full explanation. `ArchiveTimelineAccessor`
requires every bound key to be `Timeline`- or `TimeIndex`-kind, checked the moment any
method (including this one) is called.
```

## Parameters

`None`

## Returns

`Archive`
    A new archive of the same depth, with every bound key replaced by a `Bool` `Series` key: `true`
    where the row's day-of-month is `1`, `false` otherwise, in every catalog.

## Examples

Example 1. Neither sample timestamp falls on the 1st of its month. Delegation to `CatalogTimelineAccessor::month_start` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::month_start`](../catalog-timeline/month_start.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().month_start();
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [ts]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
ts                       | Series       | Series[bool]     | 2       | [false, false]                                   
========================================================================================================================
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::month_start`](../catalog-timeline/month_start.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::month_start`](../catalog-timeline/month_start.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_name.html">
    <span class="label">Previous</span>
    <span class="title">month_name</span>
  </a>
  <a class="next-link" href="period.html">
    <span class="label">Next</span>
    <span class="title">period</span>
  </a>
</div>
