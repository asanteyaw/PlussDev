<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f4d10f9958b0 -->

# CatalogTimelineAccessor::floor

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::floor(std::string_view unit) const
```

## What It Does

Rounds each bound key's timestamp *down* to the nearest `unit` boundary,
replacing each bound key's values in place (the key stays `Timeline`- or
`TimeIndex`-kind, whichever it already was).

```{important} CatalogTimelineAccessor requires every bound key to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported unit throws.

## Returns

`Catalog`
    A new catalog with every bound key's timestamps floored to `unit`.

## Examples

Example 1. Floor two timestamps to the nearest hour.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().floor("H");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.keys("ts").as_note().at(0) << " | " << fmted.keys("ts").as_note().at(1);
```

### Output

```text
2024-01-31 09:00:00 | 2024-02-29 23:00:00
```

`09:15:30` drops to `09:00:00`; `23:45:00` was already exactly on the hour in
terms of its floor target, so it stays at `23:00:00`.

Example 2. An unsupported unit throws.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
try {
  auto out = ct.tl().floor("bogus");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Timeline: unsupported unit (expected S/min/H/D)
```

## Notes

- `unit` cannot be empty, and only accepts the fixed clock units listed above —
  an unrecognized unit throws, per Example 2.
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::ceil`](ceil.md) / [`round()`](round.md): round up,
  or to the nearest boundary, instead of always down.
- [`CatalogTimelineAccessor::date`](date.md): shorthand for `floor("D")`.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::floor`](../registry-timeline/floor.md): the
  scribble-model equivalent.

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
