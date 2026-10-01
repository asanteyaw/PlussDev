<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 24c2992b46f8 -->

# ArchiveTimelineAccessor::date

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::date() const
```

## What It Does

Strips the time-of-day from each bound key's timestamp (dropped to midnight),
replacing each bound key's values in place. Equivalent to
[`floor("D")`](floor.md). Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::date`](../catalog-timeline/date.md).

```{important} Binding and kind-check rules are inherited from Archive::tl()
This method's key-binding (from `catalog(0)`) and lazy Timeline-kind check
follow exactly the rules documented on [`Archive::tl`](../archive/tl.md) — see that
page's two important callouts for the full explanation. `ArchiveTimelineAccessor`
requires every bound key to be `Timeline`-kind, checked the moment any
method (including this one) is called.
```

## Parameters

`None`

## Returns

`Archive`
    A new archive of the same depth, with every bound key's time-of-day portion zeroed out, in every catalog.

## Examples

Example 1. Strip the time-of-day from two timestamps. Delegation to `CatalogTimelineAccessor::date` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::date`](../catalog-timeline/date.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().date();
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.catalog(0).keys("ts").as_note().at(0) << " | " << fmted.catalog(0).keys("ts").as_note().at(1);
```

### Output

```text
2024-01-31 00:00:00 | 2024-02-29 00:00:00
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::date`](../catalog-timeline/date.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::date`](../catalog-timeline/date.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ceil.html">
    <span class="label">Previous</span>
    <span class="title">ceil</span>
  </a>
  <a class="next-link" href="day.html">
    <span class="label">Next</span>
    <span class="title">day</span>
  </a>
</div>
