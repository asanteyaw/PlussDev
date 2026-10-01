<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d02572c1e87c -->

# CatalogTimelineAccessor::weekend

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::weekend() const
```

## What It Does

Flags whether each bound key's timestamp falls on a Saturday or Sunday,
replacing each `Timeline`-kind key with a `Bool` `Series` key.

```{important} CatalogTimelineAccessor requires every bound key to be Timeline-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key replaced by a `Bool` `Series` key: `true`
    where the row falls on a weekend, `false` otherwise.

## Examples

Example 1. Flag a Saturday and a Wednesday.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"})),
});
auto out = ct.tl().weekend();
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
ts                       | Series       | Series[bool]     | 2       | [true, false]                                    
========================================================================================================================
```

`"2024-07-06"` is a Saturday; `"2023-03-01"` is a Wednesday.

## Notes

- Saturday and Sunday only — there's no configurable weekend definition on this
  method.
- Reads local calendar fields relative to whatever timezone the key's `Timeline`
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::weekday`](weekday.md): the underlying weekday
  number this method is derived from.
- [`CatalogTimelineAccessor::weekday_name`](weekday_name.md): the weekday
  spelled out as a full English name.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::weekend`](../registry-timeline/weekend.md): the
  scribble-model equivalent.

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
