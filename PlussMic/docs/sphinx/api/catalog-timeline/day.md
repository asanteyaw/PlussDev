<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ad7dce3b053a -->

# CatalogTimelineAccessor::day

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::day() const
```

## What It Does

Extracts the day-of-month (`1`-`31`) of each bound key's timestamp, replacing each
`Timeline`-kind key with an `Int32` `Series` key.

```{important} CatalogTimelineAccessor requires every bound key to be Timeline-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key replaced by an `Int32` `Series` key of
    day-of-month numbers.

## Examples

Example 1. Extract the day-of-month from two end-of-month timestamps.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().day();
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
ts                       | Series       | Series[int32]    | 2       | [31, 29]                                         
========================================================================================================================
```

`2024-02-29` is valid — 2024 is a leap year, so February has 29 days that year; see
[`leap()`](leap.md).

## Notes

- Reads local calendar fields relative to whatever timezone the key's `Timeline`
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::year`](year.md) / [`month()`](month.md): the other
  calendar-date components.
- [`CatalogTimelineAccessor::yearday`](yearday.md): the day-of-*year* number
  instead of day-of-month.
- [`CatalogTimelineAccessor::month_end`](month_end.md): whether this day is the
  last day of its month.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::day`](../registry-timeline/day.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="date.html">
    <span class="label">Previous</span>
    <span class="title">date</span>
  </a>
  <a class="next-link" href="epoch.html">
    <span class="label">Next</span>
    <span class="title">epoch</span>
  </a>
</div>
