<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fe3a39d5fbea -->

# ArchiveTimelineAccessor::weekday_name

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::weekday_name() const
```

## What It Does

Renders each bound key's timestamp as its full English weekday name, replacing
each `Timeline`- or `TimeIndex`-kind key with a `Note` (text) key. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::weekday_name`](../catalog-timeline/weekday_name.md).

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
    A new archive of the same depth, with every bound key replaced by a `Note` key holding each
    row's full weekday name (`"Monday"`-`"Sunday"`), in every catalog.

## Examples

Example 1. Render the weekday name for a Wednesday and a Thursday. Delegation to `CatalogTimelineAccessor::weekday_name` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::weekday_name`](../catalog-timeline/weekday_name.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().weekday_name();
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
ts                       | Note         | Note             | 2       | [Wednesday, Thursday]                            
========================================================================================================================
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::weekday_name`](../catalog-timeline/weekday_name.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::weekday_name`](../catalog-timeline/weekday_name.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday.html">
    <span class="label">Previous</span>
    <span class="title">weekday</span>
  </a>
  <a class="next-link" href="weekend.html">
    <span class="label">Next</span>
    <span class="title">weekend</span>
  </a>
</div>
