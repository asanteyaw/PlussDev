<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 19a6aa09ae5c -->

# ArchiveTimelineAccessor::index

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::index(bool require_sorted = true) const
```

## What It Does

Converts each bound key from `Timeline`-kind to `TimeIndex`-kind. Idempotent:
calling `index()` on a key that's already `TimeIndex`-kind succeeds and leaves
its values unchanged. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::index`](../catalog-timeline/index.md).

```{important} Binding and kind-check rules are inherited from Archive::tl()
This method's key-binding (from `catalog(0)`) and lazy Timeline/TimeIndex-kind check
follow exactly the rules documented on [`Archive::tl`](../archive/tl.md) — see that
page's two important callouts for the full explanation. `ArchiveTimelineAccessor`
requires every bound key to be `Timeline`- or `TimeIndex`-kind, checked the moment any
method (including this one) is called.
```

## Parameters

**require_sorted** (`bool`, defaults to `true`)
    When `true` (the default), every value in the key must be non-decreasing
    (and non-`NaT`), or the call throws. When `false`, the conversion happens
    without checking order or `NaT`.

## Returns

`Archive`
    A new archive of the same depth, with every bound key converted to `TimeIndex`-kind, in every catalog.

## Examples

Example 1. Convert a sorted `Timeline` key to `TimeIndex`. Delegation to `CatalogTimelineAccessor::index` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::index`](../catalog-timeline/index.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().index();
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
ts                       | TimeIndex    | TimeIndex        | 2       | [2024-01-01T00:00:00.000~, 2024-01-02T00:00:00.0~
========================================================================================================================
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::index`](../catalog-timeline/index.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::index`](../catalog-timeline/index.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html">
    <span class="label">Previous</span>
    <span class="title">hour</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
