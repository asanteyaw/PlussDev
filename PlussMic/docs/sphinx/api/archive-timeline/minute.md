<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4dd2add04899 -->

# ArchiveTimelineAccessor::minute

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::minute() const
```

## What It Does

Extracts the minute-of-hour (`0`-`59`) of each bound key's timestamp, replacing
each `Timeline`-kind key with an `Int32` `Series` key. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::minute`](../catalog-timeline/minute.md).

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
    A new archive of the same depth, with every bound key replaced by an `Int32` `Series` key of
    minute numbers, in every catalog.

## Examples

Example 1. Extract the minute component from two timestamps. Delegation to `CatalogTimelineAccessor::minute` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::minute`](../catalog-timeline/minute.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().minute();
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
ts                       | Series       | Series[int32]    | 2       | [15, 45]                                         
========================================================================================================================
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::minute`](../catalog-timeline/minute.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::minute`](../catalog-timeline/minute.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="localize.html">
    <span class="label">Previous</span>
    <span class="title">localize</span>
  </a>
  <a class="next-link" href="month.html">
    <span class="label">Next</span>
    <span class="title">month</span>
  </a>
</div>
