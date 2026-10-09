<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 881dc98fc4f4 -->

# ArchiveTimelineAccessor::tz

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::tz(std::string_view name) const
```

## What It Does

Relabels each bound key's timestamp *display* timezone to `name`, without
changing the underlying instant — the same operation as
[`Slate::tl().tz()`](../slate-timeline/tz.md)/
[`DataFrame::tl().tz()`](../dataframe-timeline/tz.md) at the table layer.

The relabeling survives being read back through any further `.tl()` call
(`fmt()`, `hour()`, another `tz()`, anything) — see Example 1. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::tz`](../catalog-timeline/tz.md).

```{important} Binding and kind-check rules are inherited from Archive::tl()
This method's key-binding (from `catalog(0)`) and lazy Timeline-kind check
follow exactly the rules documented on [`Archive::tl`](../archive/tl.md) — see that
page's two important callouts for the full explanation. `ArchiveTimelineAccessor`
requires every bound key to be `Timeline`-kind, checked the moment any
method (including this one) is called.
```

## Parameters

**name** (`std::string_view`)
    The target timezone. Only `"UTC"` (also the empty string),
    `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` are
    recognized (case-insensitive); any other name throws.

## Returns

`Archive`
    A new archive of the same depth, with every bound key's timestamps relabeled to `name`.
    Reading the result back through any further `.tl()` call reflects the new
    display timezone, in every catalog.

## Examples

Example 1. `tz()` relabels the display timezone, and the relabeling is still
in effect on the very next `.tl()` read. Delegation to `CatalogTimelineAccessor::tz` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::tz`](../catalog-timeline/tz.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto shifted = archive.tl().tz("America/New_York");
auto fmted = shifted.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.catalog(0).keys("ts").as_note().at(0);
```

### Output

```text
2024-01-31 04:15:30
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::tz`](../catalog-timeline/tz.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::tz`](../catalog-timeline/tz.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="time.html">
    <span class="label">Previous</span>
    <span class="title">time</span>
  </a>
  <a class="next-link" href="utc.html">
    <span class="label">Next</span>
    <span class="title">utc</span>
  </a>
</div>
