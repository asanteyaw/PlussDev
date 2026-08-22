<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a4b2ed269fe1 -->

# CatalogTimelineAccessor::quarter

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::quarter() const
```

## What It Does

Extracts the calendar quarter (`1`-`4`) of each bound key's timestamp, replacing
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
    quarter numbers.

## Examples

Example 1. Extract the quarter from a July and a March timestamp.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"})),
});
auto out = ct.tl().quarter();
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
ts                       | Series       | Series[int32]    | 2       | [3, 1]                                           
========================================================================================================================
```

July falls in Q3 (months 7-9); March falls in Q1 (months 1-3).

## Notes

- `1` covers January-March, `2` April-June, `3` July-September, `4`
  October-December.
- Reads local calendar fields relative to whatever timezone the key's `Timeline`
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::month`](month.md): the finer-grained component
  `quarter()` is derived from.
- [`CatalogTimelineAccessor::period`](period.md): group timestamps into
  arbitrary frequency-based periods, not just quarters.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::quarter`](../registry-timeline/quarter.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="period.html">
    <span class="label">Previous</span>
    <span class="title">period</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
