<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d9254a35200f -->

# ArchiveTimelineAccessor::floor

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::floor(std::string_view unit) const
```

## What It Does

Rounds each bound key's timestamp *down* to the nearest `unit` boundary,
replacing each bound key's values in place (the key stays `Timeline`-kind). Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::floor`](../catalog-timeline/floor.md).

```{important} Binding and kind-check rules are inherited from Archive::tl()
This method's key-binding (from `catalog(0)`) and lazy Timeline-kind check
follow exactly the rules documented on [`Archive::tl`](../archive/tl.md) — see that
page's two important callouts for the full explanation. `ArchiveTimelineAccessor`
requires every bound key to be `Timeline`-kind, checked the moment any
method (including this one) is called.
```

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported unit throws.

## Returns

`Archive`
    A new archive of the same depth, with every bound key's timestamps floored to `unit`, in every catalog.

## Examples

Example 1. Floor two timestamps to the nearest hour. Delegation to `CatalogTimelineAccessor::floor` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::floor`](../catalog-timeline/floor.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().floor("H");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.catalog(0).keys("ts").as_note().at(0) << " | " << fmted.catalog(0).keys("ts").as_note().at(1);
```

### Output

```text
2024-01-31 09:00:00 | 2024-02-29 23:00:00
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::floor`](../catalog-timeline/floor.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::floor`](../catalog-timeline/floor.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch.html">
    <span class="label">Previous</span>
    <span class="title">epoch</span>
  </a>
  <a class="next-link" href="fmt.html">
    <span class="label">Next</span>
    <span class="title">fmt</span>
  </a>
</div>
