<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 86b4d83d6bac -->

# CatalogTimelineAccessor::minute

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::minute() const
```

## What It Does

Extracts the minute-of-hour (`0`-`59`) of each bound key's timestamp, replacing
each `Timeline`- or `TimeIndex`-kind key with an `Int32` `Series` key.

```{important} CatalogTimelineAccessor requires every bound key to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key replaced by an `Int32` `Series` key of
    minute numbers.

## Examples

Example 1. Extract the minute component from two timestamps.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().minute();
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
ts                       | Series       | Series[int32]    | 2       | [15, 45]                                         
========================================================================================================================
```

## Notes

- Reads local calendar fields relative to whatever timezone the key's `Timeline`
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::hour`](hour.md) / [`second()`](second.md): the
  neighboring time-of-day components.
- [`CatalogTimelineAccessor::time`](time.md): the whole time-of-day portion at
  once.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::minute`](../registry-timeline/minute.md): the
  bundle-model equivalent.

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
