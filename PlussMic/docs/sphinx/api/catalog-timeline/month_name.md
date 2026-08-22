<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 465efcca9510 -->

# CatalogTimelineAccessor::month_name

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::month_name() const
```

## What It Does

Renders each bound key's timestamp as its full English month name, replacing
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
    row's full month name (`"January"`-`"December"`).

## Examples

Example 1. Render the month name for a January and a February timestamp.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().month_name();
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
ts                       | Note         | Note             | 2       | [January, February]                              
========================================================================================================================
```

## Notes

- Always the full English name (`"January"`, not `"Jan"`); use [`fmt("%b")`](fmt.md)
  for the abbreviated three-letter form.
- Changes each bound key's kind from `Timeline`/`TimeIndex` to `Note`.
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::month`](month.md): the same information as a
  `1`-`12` number instead of a name.
- [`CatalogTimelineAccessor::weekday_name`](weekday_name.md): the equivalent for
  weekdays.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::month_name`](../registry-timeline/month_name.md):
  the scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_end.html">
    <span class="label">Previous</span>
    <span class="title">month_end</span>
  </a>
  <a class="next-link" href="month_start.html">
    <span class="label">Next</span>
    <span class="title">month_start</span>
  </a>
</div>
