<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: baf3ed5dae3d -->

# ArchiveTimelineAccessor::utc

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::utc() const
```

## What It Does

Shorthand for [`tz("UTC")`](tz.md) — relabels each bound key's display
timezone back to `"UTC"` without changing the underlying instant. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::utc`](../catalog-timeline/utc.md).

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
    A new archive of the same depth, with every bound key relabeled to `"UTC"` display. Reading
    the result back through any further `.tl()` call reflects the `"UTC"`
    relabeling, in every catalog.

## Examples

Example 1. `tz("America/New_York")` relabels the display timezone (visible on
the first `fmt()` read below); a follow-up `utc()` relabels it back to
`"UTC"`, and that too is visible on the next read — the instant itself never
changes across either call. Delegation to `CatalogTimelineAccessor::utc` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::utc`](../catalog-timeline/utc.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto ny = archive.tl().tz("America/New_York");
std::cout << "NY-relabeled: "
          << ny.tl().fmt("%Y-%m-%d %H:%M:%S").catalog(0).keys("ts").as_note().at(0) << "\n";

auto out = ny.tl().utc();
std::cout << "UTC-relabeled: "
          << out.tl().fmt("%Y-%m-%d %H:%M:%S").catalog(0).keys("ts").as_note().at(0);
```

### Output

```text
NY-relabeled: 2024-01-31 04:15:30
UTC-relabeled: 2024-01-31 09:15:30
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::utc`](../catalog-timeline/utc.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::utc`](../catalog-timeline/utc.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz.html">
    <span class="label">Previous</span>
    <span class="title">tz</span>
  </a>
  <a class="next-link" href="valid.html">
    <span class="label">Next</span>
    <span class="title">valid</span>
  </a>
</div>
