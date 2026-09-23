<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e402f7dbdda8 -->

# ArchiveTimelineAccessor::isnat

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::isnat() const
```

## What It Does

Flags whether each bound key's timestamp is `NaT` (missing/"Not a Time"), replacing
each `Timeline`- or `TimeIndex`-kind key with a `Bool` `Series` key. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::isnat`](../catalog-timeline/isnat.md).

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
    where the value is `NaT`, `false` otherwise, in every catalog.

## Examples

Example 1. Flag a `NaT` timestamp alongside a valid one. An empty string passed to
[`pluss::make_timeline`](../catalog/tl.md) produces `NaT` rather than throwing. Delegation to `CatalogTimelineAccessor::isnat` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::isnat`](../catalog-timeline/isnat.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"", "2024-01-01T00:00:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().isnat();
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
ts                       | Series       | Series[bool]     | 2       | [true, false]                                    
========================================================================================================================
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::isnat`](../catalog-timeline/isnat.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::isnat`](../catalog-timeline/isnat.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index.html">
    <span class="label">Previous</span>
    <span class="title">index</span>
  </a>
  <a class="next-link" href="iso.html">
    <span class="label">Next</span>
    <span class="title">iso</span>
  </a>
</div>
