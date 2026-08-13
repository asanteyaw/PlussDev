<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7c54319f76c2 -->

# CatalogTimelineAccessor::hour

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::hour() const
```

## What It Does

Extracts the hour-of-day (`0`-`23`) of each bound key's timestamp, replacing each
`Timeline`- or `TimeIndex`-kind key with an `Int32` `Series` key.

```{important} CatalogTimelineAccessor requires every bound key to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key replaced by an `Int32` `Series` key of hour
    numbers.

## Examples

Example 1. Extract the hour from a morning and a late-night timestamp.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().hour();
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
ts                       | Series       | Series[int32]    | 2       | [9, 23]                                          
========================================================================================================================
```

## Notes

- 24-hour clock: `0` is midnight, `23` is 11 PM.
- Reads local calendar fields relative to whatever timezone the key's `Timeline`
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::minute`](minute.md) / [`second()`](second.md): the
  finer-grained time-of-day components.
- [`CatalogTimelineAccessor::time`](time.md): the whole time-of-day portion at
  once.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::hour`](../registry-timeline/hour.md): the
  bundle-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmt.html">
    <span class="label">Previous</span>
    <span class="title">fmt</span>
  </a>
  <a class="next-link" href="index.html">
    <span class="label">Next</span>
    <span class="title">index</span>
  </a>
</div>
