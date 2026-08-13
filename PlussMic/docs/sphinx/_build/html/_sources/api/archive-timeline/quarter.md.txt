<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8cc5bc3f3d45 -->

# ArchiveTimelineAccessor::quarter

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::quarter() const
```

## What It Does

Extracts the calendar quarter (`1`-`4`) of each bound key's timestamp, replacing
each `Timeline`- or `TimeIndex`-kind key with an `Int32` `Series` key. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::quarter`](../catalog-timeline/quarter.md).

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
    A new archive of the same depth, with every bound key replaced by an `Int32` `Series` key of
    quarter numbers, in every catalog.

## Examples

Example 1. Extract the quarter from a July and a March timestamp. Delegation to `CatalogTimelineAccessor::quarter` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::quarter`](../catalog-timeline/quarter.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().quarter();
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
ts                       | Series       | Series[int32]    | 2       | [3, 1]                                           
========================================================================================================================
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::quarter`](../catalog-timeline/quarter.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::quarter`](../catalog-timeline/quarter.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="period.html">
    <span class="label">Previous</span>
    <span class="title">period</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
