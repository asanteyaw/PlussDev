<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f42b9eda30d6 -->

# ArchiveTimelineAccessor::fmt

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::fmt(std::string_view pattern) const
```

## What It Does

Renders each bound key's timestamp as custom text built from `pattern`,
replacing each `Timeline`-kind key with a `Note` (text) key. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::fmt`](../catalog-timeline/fmt.md).

```{important} Binding and kind-check rules are inherited from Archive::tl()
This method's key-binding (from `catalog(0)`) and lazy Timeline-kind check
follow exactly the rules documented on [`Archive::tl`](../archive/tl.md) — see that
page's two important callouts for the full explanation. `ArchiveTimelineAccessor`
requires every bound key to be `Timeline`-kind, checked the moment any
method (including this one) is called.
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

`Archive`
    A new archive of the same depth, with every bound key replaced by a `Note` key holding each
    row's text rendered per `pattern`, in every catalog.

## Examples

Example 1. Format two timestamps with a custom day/month-name/year pattern. Delegation to `CatalogTimelineAccessor::fmt` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::fmt`](../catalog-timeline/fmt.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().fmt("%d %b %Y");
std::cout << out.catalog(0);
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

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::fmt`](../catalog-timeline/fmt.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::fmt`](../catalog-timeline/fmt.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

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
