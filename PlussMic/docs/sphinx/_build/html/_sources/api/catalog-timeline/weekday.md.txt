<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9e1e812d1e27 -->

# CatalogTimelineAccessor::weekday

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::weekday() const
```

## What It Does

Extracts the weekday number (`0` = Monday through `6` = Sunday) of each bound
key's timestamp, replacing each `Timeline`- or `TimeIndex`-kind key with an
`Int32` `Series` key.

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
    weekday numbers.

## Examples

Example 1. Extract the weekday number from a Wednesday and a Thursday.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().weekday();
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
ts                       | Series       | Series[int32]    | 2       | [2, 3]                                           
========================================================================================================================
```

`"2024-01-31"` is a Wednesday (`2`), `"2024-02-29"` is a Thursday (`3`) — see
[`weekday_name()`](weekday_name.md) for the same information spelled out.

## Notes

- `0` is Monday, `6` is Sunday (ISO weekday numbering).
- Reads local calendar fields relative to whatever timezone the key's `Timeline`
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::weekday_name`](weekday_name.md): the same
  information as a full English name instead of a number.
- [`CatalogTimelineAccessor::weekend`](weekend.md): whether the weekday falls on
  a weekend.
- [`CatalogTimelineAccessor::week`](week.md): the ISO week-of-year this weekday
  belongs to.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::weekday`](../registry-timeline/weekday.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="week.html">
    <span class="label">Previous</span>
    <span class="title">week</span>
  </a>
  <a class="next-link" href="weekday_name.html">
    <span class="label">Next</span>
    <span class="title">weekday_name</span>
  </a>
</div>
