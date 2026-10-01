<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2f78e0630b46 -->

# ArchiveTimelineAccessor::month_end

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::month_end() const
```

## What It Does

Flags whether each bound key's timestamp falls on the last day of its calendar
month, replacing each `Timeline`-kind key with a `Bool` `Series`
key. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::month_end`](../catalog-timeline/month_end.md).

```{important} Binding and kind-check rules are inherited from Archive::tl()
This method's key-binding (from `catalog(0)`) and lazy Timeline-kind check
follow exactly the rules documented on [`Archive::tl`](../archive/tl.md) — see that
page's two important callouts for the full explanation. `ArchiveTimelineAccessor`
requires every bound key to be `Timeline`-kind, checked the moment any
method (including this one) is called.
```

## Parameters

`None`

## Returns

`Archive`
    A new archive of the same depth, with every bound key replaced by a `Bool` `Series` key: `true`
    where the row's day-of-month is that month's last valid day, `false`
    otherwise, in every catalog.

## Examples

Example 1. Both sample timestamps are the last day of their respective month. Delegation to `CatalogTimelineAccessor::month_end` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::month_end`](../catalog-timeline/month_end.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().month_end();
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
ts                       | Series       | Series[bool]     | 2       | [true, true]                                     
========================================================================================================================
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::month_end`](../catalog-timeline/month_end.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::month_end`](../catalog-timeline/month_end.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month.html">
    <span class="label">Previous</span>
    <span class="title">month</span>
  </a>
  <a class="next-link" href="month_name.html">
    <span class="label">Next</span>
    <span class="title">month_name</span>
  </a>
</div>
