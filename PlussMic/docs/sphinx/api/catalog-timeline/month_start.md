<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 90373b0a3211 -->

# CatalogTimelineAccessor::month_start

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::month_start() const
```

## What It Does

Flags whether each bound key's timestamp falls on the first day of its calendar
month, replacing each `Timeline`- or `TimeIndex`-kind key with a `Bool` `Series`
key.

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
    where the row's day-of-month is `1`, `false` otherwise.

## Examples

Example 1. Neither sample timestamp falls on the 1st of its month.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().month_start();
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
ts                       | Series       | Series[bool]     | 2       | [false, false]                                   
========================================================================================================================
```

Both sample dates are the *last* day of their month, not the first — see
[`month_end()`](month_end.md), which flags `true` for exactly these rows.

## Notes

- Only checks the day-of-month component (`day() == 1`); the time-of-day portion
  doesn't affect the result.
- Reads local calendar fields relative to whatever timezone the key's `Timeline`
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::month_end`](month_end.md): the mirror check at the
  end of the month.
- [`CatalogTimelineAccessor::day`](day.md): the underlying day-of-month number
  this method is derived from.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::month_start`](../registry-timeline/month_start.md):
  the bundle-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_name.html">
    <span class="label">Previous</span>
    <span class="title">month_name</span>
  </a>
  <a class="next-link" href="period.html">
    <span class="label">Next</span>
    <span class="title">period</span>
  </a>
</div>
