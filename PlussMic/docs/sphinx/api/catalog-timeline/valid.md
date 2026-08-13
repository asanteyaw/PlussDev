<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 169ba9bcf000 -->

# CatalogTimelineAccessor::valid

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::valid() const
```

## What It Does

Flags whether each bound key's timestamp is a real, present timestamp (not `NaT`),
replacing each `Timeline`- or `TimeIndex`-kind key with a `Bool` `Series` key.
Exactly the logical inverse of [`isnat()`](isnat.md).

```{important} CatalogTimelineAccessor requires every bound key to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key replaced by a `Bool` `Series` key: `true`
    where the value is a real timestamp, `false` where it's `NaT`.

## Examples

Example 1. Flag a valid timestamp alongside a `NaT` one.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"", "2024-01-01T00:00:00Z"})),
});
auto out = ct.tl().valid();
std::cout << out;
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

- Exactly `!isnat()`, provided for readability at call sites that want to filter
  *for* present timestamps rather than *for* missing ones.
- See the important callout above.
- Bound via [`Catalog::tl()`](../catalog/tl.md), which binds every key of the
  catalog unless you project first with [`Catalog::keys(names)`](../catalog/keys.md).

## See Also

- [`CatalogTimelineAccessor::isnat`](isnat.md): the logical inverse.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::valid`](../registry-timeline/valid.md): the
  bundle-model equivalent.

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
