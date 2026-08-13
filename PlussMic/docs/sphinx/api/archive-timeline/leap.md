<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1efaae672fe1 -->

# ArchiveTimelineAccessor::leap

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::leap() const
```

## What It Does

Flags whether each bound key's timestamp's calendar year is a leap year,
replacing each `Timeline`- or `TimeIndex`-kind key with a `Bool` `Series` key. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::leap`](../catalog-timeline/leap.md).

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
    when the row's calendar year is a leap year, `false` otherwise, in every catalog.

## Examples

Example 1. Two timestamps from the same leap year both flag `true`, even though
only one falls on February 29th. Delegation to `CatalogTimelineAccessor::leap` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::leap`](../catalog-timeline/leap.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().leap();
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
  [`CatalogTimelineAccessor::leap`](../catalog-timeline/leap.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::leap`](../catalog-timeline/leap.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="iso.html">
    <span class="label">Previous</span>
    <span class="title">iso</span>
  </a>
  <a class="next-link" href="localize.html">
    <span class="label">Next</span>
    <span class="title">localize</span>
  </a>
</div>
