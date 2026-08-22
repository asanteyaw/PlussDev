<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eabe0b5688e7 -->

# CatalogTimelineAccessor::weekday_name

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::weekday_name() const
```

## What It Does

Renders each bound key's timestamp as its full English weekday name, replacing
each `Timeline`- or `TimeIndex`-kind key with a `Note` (text) key.

```{important} CatalogTimelineAccessor requires every bound key to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key replaced by a `Note` key holding each
    row's full weekday name (`"Monday"`-`"Sunday"`).

## Examples

Example 1. Render the weekday name for a Wednesday and a Thursday.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().weekday_name();
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
ts                       | Note         | Note             | 2       | [Wednesday, Thursday]                            
========================================================================================================================
```

## Notes

- Always the full English name (`"Monday"`, not `"Mon"`); use [`fmt("%a")`](fmt.md)
  for the abbreviated three-letter form.
- Changes each bound key's kind from `Timeline`/`TimeIndex` to `Note`.
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::weekday`](weekday.md): the same information as a
  `0`-`6` number instead of a name.
- [`CatalogTimelineAccessor::month_name`](month_name.md): the equivalent for
  months.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::weekday_name`](../registry-timeline/weekday_name.md):
  the scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday.html">
    <span class="label">Previous</span>
    <span class="title">weekday</span>
  </a>
  <a class="next-link" href="weekend.html">
    <span class="label">Next</span>
    <span class="title">weekend</span>
  </a>
</div>
