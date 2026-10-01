<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e721d5b934b5 -->

# CatalogTimelineAccessor::tz

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::tz(std::string_view name) const
```

## What It Does

Relabels each bound key's timestamp *display* timezone to `name`, without
changing the underlying instant — the same operation as
[`Thread::tl().tz()`](../thread-timeline/tz.md)/
[`DataFrame::tl().tz()`](../dataframe-timeline/tz.md) at the table layer.

The relabeling survives being read back through any further `.tl()` call
(`fmt()`, `hour()`, another `tz()`, anything) — see Example 1.

```{important} CatalogTimelineAccessor requires every bound key to be Timeline-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**name** (`std::string_view`)
    The target timezone. Only `"UTC"` (also the empty string),
    `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` are
    recognized (case-insensitive); any other name throws.

## Returns

`Catalog`
    A new catalog with every bound key's timestamps relabeled to `name`.
    Reading the result back through any further `.tl()` call reflects the new
    display timezone.

## Examples

Example 1. `tz()` relabels the display timezone, and the relabeling is still
in effect on the very next `.tl()` read.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
auto shifted = ct.tl().tz("America/New_York");
auto fmted = shifted.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.keys("ts").as_note().at(0);
```

### Output

```text
2024-01-31 04:15:30
```

`04:15:30` is the `"America/New_York"`-relative reading of the same instant (a
`-5:00` offset in January) — matching what
[`Thread::tl().tz()`](../thread-timeline/tz.md) produces on the same input.

## Notes

- Only four timezone names are recognized: `"UTC"` (or empty string),
  `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` — any other
  name throws.
- Internally, each bound key stores its `Timeline` values
  independently; `tz()` relabels every row, and that relabeling round-trips
  correctly through the `kindle::Stream` boundary every further `.tl()` call
  goes through. If every row does not already share the same display timezone
  going into a `.tl()` call, the call throws rather than picking one
  arbitrarily — combine to a common zone with `tz()`/`localize()` first if
  needed.
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::utc`](utc.md): shorthand for `tz("UTC")`.
- [`CatalogTimelineAccessor::localize`](localize.md): changes the underlying
  instant (which *does* persist) rather than just the display label.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::tz`](../registry-timeline/tz.md): the
  scribble-model equivalent.

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
