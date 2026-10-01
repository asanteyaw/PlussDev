<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c6d67cb37bb6 -->

# CatalogTimelineAccessor::yearday

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::yearday() const
```

## What It Does

Extracts the day-of-year (`1`-`366`) of each bound key's timestamp, replacing
each `Timeline`-kind key with an `Int32` `Series` key.

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
    day-of-year numbers.

## Examples

Example 1. Extract the day-of-year from a late-January and a late-February
timestamp.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().yearday();
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
ts                       | Series       | Series[int32]    | 2       | [31, 60]                                         
========================================================================================================================
```

`"2024-01-31"` is the 31st day of the year; `"2024-02-29"` is the 60th — the extra
leap day pushes every date from March onward one day later than it would fall in
a non-leap year.

## Notes

- `1` is January 1st; the maximum is `366` in a leap year, `365` otherwise — see
  [`leap()`](leap.md).
- Reads local calendar fields relative to whatever timezone the key's `Timeline`
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::day`](day.md): the day-of-*month* number instead of
  day-of-year.
- [`CatalogTimelineAccessor::week`](week.md): the ISO week-of-year, a
  coarser-grained position within the year.
- [`CatalogTimelineAccessor::leap`](leap.md): whether the row's year has the
  extra day that shifts this number.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::yearday`](../registry-timeline/yearday.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="year.html">
    <span class="label">Previous</span>
    <span class="title">year</span>
  </a>
</div>
