<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c1ab5946a4d1 -->

# CatalogTimelineAccessor::leap

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::leap() const
```

## What It Does

Flags whether each bound key's timestamp's calendar year is a leap year,
replacing each `Timeline`-kind key with a `Bool` `Series` key.

```{important} CatalogTimelineAccessor requires every bound key to be Timeline-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

```{important} Reflects the row's YEAR, not whether the date is February 29th
`leap()` answers "is this row's *year* a leap year," not "is this row February
29th." Any row dated anywhere in 2024 flags `true`, since 2024 as a whole is a
leap year — see Example 1, where a January date and a February-29th date both
flag `true` for the same reason.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key replaced by a `Bool` `Series` key: `true`
    when the row's calendar year is a leap year, `false` otherwise.

## Examples

Example 1. Two timestamps from the same leap year both flag `true`, even though
only one falls on February 29th.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().leap();
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

Example 2. A 2024 timestamp flags `true`; a 2023 timestamp flags `false`, since
2023 isn't a leap year.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"})),
});
auto out = ct.tl().leap();
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

## Notes

- See the important callouts above — reflects the row's year, not its exact
  date, and requires every bound key to be `Timeline`-kind.
- Reads local calendar fields relative to whatever timezone the key's `Timeline`
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).

## See Also

- [`CatalogTimelineAccessor::month_end`](month_end.md): where leap-year status
  actually changes February's last valid day.
- [`CatalogTimelineAccessor::yearday`](yearday.md): where leap years shift every
  day-of-year number from March onward.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::leap`](../registry-timeline/leap.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="iso.html">
    <span class="label">Previous</span>
    <span class="title">iso</span>
  </a>
  <a class="next-link" href="localize.html">
    <span class="label">Next</span>
    <span class="title">localize</span>
  </a>
</div>
