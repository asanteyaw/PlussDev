<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e7a5d9c6a1a7 -->

# ArchiveTimelineAccessor::valid

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::valid() const
```

## What It Does

Flags whether each bound key's timestamp is a real, present timestamp (not `NaT`),
replacing each `Timeline`- or `TimeIndex`-kind key with a `Bool` `Series` key.
Exactly the logical inverse of [`isnat()`](isnat.md). Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::valid`](../catalog-timeline/valid.md).

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
    where the value is a real timestamp, `false` where it's `NaT`, in every catalog.

## Examples

Example 1. Flag a valid timestamp alongside a `NaT` one. Delegation to `CatalogTimelineAccessor::valid` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::valid`](../catalog-timeline/valid.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"", "2024-01-01T00:00:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().valid();
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
ts                       | Series       | Series[bool]     | 2       | [false, true]                                    
========================================================================================================================
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::valid`](../catalog-timeline/valid.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::valid`](../catalog-timeline/valid.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="utc.html">
    <span class="label">Previous</span>
    <span class="title">utc</span>
  </a>
  <a class="next-link" href="week.html">
    <span class="label">Next</span>
    <span class="title">week</span>
  </a>
</div>
