<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: af4de6455bb2 -->

# ArchiveTimelineAccessor::iso

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::iso() const
```

## What It Does

Renders each bound key's timestamp as a fixed ISO-8601-with-offset text
representation, replacing each `Timeline`- or `TimeIndex`-kind key with a `Note`
(text) key. Equivalent to [`fmt("%Y-%m-%dT%H:%M:%S.%f%z")`](fmt.md). Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::iso`](../catalog-timeline/iso.md).

```{important} Binding and kind-check rules are inherited from Archive::tl()
This method's key-binding (from `catalog(0)`) and lazy Timeline/TimeIndex-kind check
follow exactly the rules documented on [`Archive::tl`](../archive/tl.md) — see that
page's two important callouts for the full explanation. `ArchiveTimelineAccessor`
requires every bound key to be `Timeline`- or `TimeIndex`-kind, checked the moment any
method (including this one) is called.
```

## Parameters

`None`

## Returns

`Archive`
    A new archive of the same depth, with every bound key replaced by a `Note` key holding each row
    as `"YYYY-MM-DDTHH:MM:SS.fffffffff+HH:MM"`, in every catalog.

## Examples

Example 1. Render a timestamp as ISO-8601 text. Delegation to `CatalogTimelineAccessor::iso` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::iso`](../catalog-timeline/iso.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().iso();
std::cout << out.catalog(0).keys("ts").as_note().at(0);
```

### Output

```text
2024-01-31T09:15:30.000000000+00:00
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::iso`](../catalog-timeline/iso.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::iso`](../catalog-timeline/iso.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnat.html">
    <span class="label">Previous</span>
    <span class="title">isnat</span>
  </a>
  <a class="next-link" href="leap.html">
    <span class="label">Next</span>
    <span class="title">leap</span>
  </a>
</div>
