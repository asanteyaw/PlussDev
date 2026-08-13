<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 13609c36060e -->

# CatalogTimelineAccessor::fmt

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::fmt(std::string_view pattern) const
```

## What It Does

Renders each bound key's timestamp as custom text built from `pattern`,
replacing each `Timeline`- or `TimeIndex`-kind key with a `Note` (text) key.

```{important} CatalogTimelineAccessor requires every bound key to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**pattern** (`std::string_view`)
    A `strftime`-style format string. Recognized tokens: `%Y` (4-digit year),
    `%m`/`%d` (2-digit month/day), `%H`/`%M`/`%S` (2-digit hour/minute/second),
    `%f` (9-digit zero-padded nanoseconds), `%z` (UTC offset as
    `+HH:MM`/`-HH:MM`, never a bare `Z`), `%a`/`%A` (abbreviated/full weekday
    name), `%b`/`%B` (abbreviated/full month name), and `%%` (a literal `%`).
    Cannot be empty.

## Returns

`Catalog`
    A new catalog with every bound key replaced by a `Note` key holding each
    row's text rendered per `pattern`.

## Examples

Example 1. Format two timestamps with a custom day/month-name/year pattern.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().fmt("%d %b %Y");
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
ts                       | Note         | Note             | 2       | [31 Jan 2024, 29 Feb 2024]                       
========================================================================================================================
```

Example 2. `fmt()` also works on an already-`TimeIndex`-kind key (produced by
[`index()`](index.md)), formatting each row the same way.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
auto ti = ct.tl().index();
auto out = ti.tl().fmt("%Y");
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
ts                       | Note         | Note             | 1       | [2024]                                          
========================================================================================================================
```

## Notes

- `pattern` cannot be empty — an empty pattern throws.
- `%f` always emits all 9 nanosecond digits, zero-padded; `%z` always renders as
  a signed `HH:MM` offset, never a bare `"Z"`.
- Changes each bound key's kind from `Timeline`/`TimeIndex` to `Note`.
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::iso`](iso.md): a fixed ISO-8601-with-offset
  pattern, equivalent to `fmt("%Y-%m-%dT%H:%M:%S.%f%z")`.
- [`CatalogTimelineAccessor::weekday_name`](weekday_name.md) /
  [`month_name()`](month_name.md): dedicated shorthands for the full-name
  tokens.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::fmt`](../registry-timeline/fmt.md): the
  bundle-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floor.html">
    <span class="label">Previous</span>
    <span class="title">floor</span>
  </a>
  <a class="next-link" href="hour.html">
    <span class="label">Next</span>
    <span class="title">hour</span>
  </a>
</div>
