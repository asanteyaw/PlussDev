<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d51dd708076e -->

# ArchiveTimelineAccessor::weekend

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::weekend() const
```

## What It Does

Flags whether each bound key's timestamp falls on a Saturday or Sunday,
replacing each `Timeline`-kind key with a `Bool` `Series` key. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::weekend`](../catalog-timeline/weekend.md).

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
    where the row falls on a weekend, `false` otherwise, in every catalog.

## Examples

Example 1. Flag a Saturday and a Wednesday. Delegation to `CatalogTimelineAccessor::weekend` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::weekend`](../catalog-timeline/weekend.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().weekend();
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
  [`CatalogTimelineAccessor::weekend`](../catalog-timeline/weekend.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::weekend`](../catalog-timeline/weekend.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday_name.html">
    <span class="label">Previous</span>
    <span class="title">weekday_name</span>
  </a>
  <a class="next-link" href="year.html">
    <span class="label">Next</span>
    <span class="title">year</span>
  </a>
</div>
