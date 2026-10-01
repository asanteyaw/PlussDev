<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 08173a8abbac -->

# ArchiveTimelineAccessor::time

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::time() const
```

## What It Does

Keeps only the time-of-day portion of each bound key's timestamp — the date is
reset to the Unix epoch day (`1970-01-01`) — replacing each bound key's values
in place. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::time`](../catalog-timeline/time.md).

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
    A new archive of the same depth, with every bound key's date portion reset to `1970-01-01`,
    keeping only the time-of-day, in every catalog.

## Examples

Example 1. Keep only the time-of-day from two timestamps. Delegation to `CatalogTimelineAccessor::time` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::time`](../catalog-timeline/time.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().time();
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.catalog(0).keys("ts").as_note().at(0) << " | " << fmted.catalog(0).keys("ts").as_note().at(1);
```

### Output

```text
1970-01-01 09:15:30 | 1970-01-01 23:45:00
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::time`](../catalog-timeline/time.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::time`](../catalog-timeline/time.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shift.html">
    <span class="label">Previous</span>
    <span class="title">shift</span>
  </a>
  <a class="next-link" href="tz.html">
    <span class="label">Next</span>
    <span class="title">tz</span>
  </a>
</div>
