<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 283ba808a768 -->

# CatalogTimelineAccessor::month_end

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::month_end() const
```

## What It Does

Flags whether each bound key's timestamp falls on the last day of its calendar
month, replacing each `Timeline`-kind key with a `Bool` `Series`
key.

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
    where the row's day-of-month is that month's last valid day, `false`
    otherwise.

## Examples

Example 1. Both sample timestamps are the last day of their respective month.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().month_end();
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
ts                       | Series       | Series[bool]     | 2       | [true, true]                                     
========================================================================================================================
```

`"2024-01-31"` is January's last day; `"2024-02-29"` is February's last day in
this leap year — see [`leap()`](leap.md), since the correct "last day" for
February depends on whether the year is a leap year.

## Notes

- Correctly accounts for each month's actual length, including leap-year
  February — see the important callout on [`leap()`](leap.md).
- Only checks the date component; the time-of-day portion doesn't affect the
  result.
- Reads local calendar fields relative to whatever timezone the key's `Timeline`
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::month_start`](month_start.md): the mirror check at
  the beginning of the month.
- [`CatalogTimelineAccessor::leap`](leap.md): where leap-year status actually
  changes February's last valid day.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::month_end`](../registry-timeline/month_end.md):
  the scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month.html">
    <span class="label">Previous</span>
    <span class="title">month</span>
  </a>
  <a class="next-link" href="month_name.html">
    <span class="label">Next</span>
    <span class="title">month_name</span>
  </a>
</div>
