<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 54ba52a58140 -->

# CatalogTimelineAccessor::utc

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::utc() const
```

## What It Does

Shorthand for [`tz("UTC")`](tz.md) — relabels each bound key's display
timezone back to `"UTC"` without changing the underlying instant.

```{important} CatalogTimelineAccessor requires every bound key to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key relabeled to `"UTC"` display. Reading
    the result back through any further `.tl()` call reflects the `"UTC"`
    relabeling.

## Examples

Example 1. `tz("America/New_York")` relabels the display timezone (visible on
the first `fmt()` read below); a follow-up `utc()` relabels it back to
`"UTC"`, and that too is visible on the next read — the instant itself never
changes across either call.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
auto ny = ct.tl().tz("America/New_York");
std::cout << "NY-relabeled: "
          << ny.tl().fmt("%Y-%m-%d %H:%M:%S").keys("ts").as_note().at(0) << "\n";

auto out = ny.tl().utc();
std::cout << "UTC-relabeled: "
          << out.tl().fmt("%Y-%m-%d %H:%M:%S").keys("ts").as_note().at(0);
```

### Output

```text
NY-relabeled: 2024-01-31 04:15:30
UTC-relabeled: 2024-01-31 09:15:30
```

## Notes

- Internally, each bound key stores its `Timeline`/`TimeIndex` values
  independently; the resulting per-row tz labels round-trip correctly through
  the `kindle::Stream` boundary every further `.tl()` call goes through, the
  same as [`tz()`](tz.md).
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::tz`](tz.md): the general form this method is a
  fixed `"UTC"` shorthand for.
- [`CatalogTimelineAccessor::localize`](localize.md): the operation that
  actually changes the underlying instant.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::utc`](../registry-timeline/utc.md): the
  scribble-model equivalent.

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
