<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 715c53ccd4b1 -->

# CatalogTimelineAccessor::ceil

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::ceil(std::string_view unit) const
```

## What It Does

Rounds each bound key's timestamp *up* to the nearest `unit` boundary,
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
    A new catalog with every bound key's timestamps ceiled to `unit`.

## Examples

Example 1. Ceil two timestamps to the nearest hour.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().ceil("H");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.keys("ts").as_note().at(0) << " | " << fmted.keys("ts").as_note().at(1);
```

### Output

```text
2024-01-31 10:00:00 | 2024-03-01 00:00:00
```

Both rows advance to the start of their *next* hour — `09:15:30` to `10:00:00`,
and `23:45:00` rolling over the day boundary into `2024-03-01 00:00:00`.

## Notes

- A row already exactly on a `unit` boundary is left unchanged, not advanced to
  the next boundary.
- `unit` cannot be empty, and only accepts the fixed clock units listed above —
  an unrecognized unit throws `"Timeline: unsupported unit (expected
  S/min/H/D)"` (see [`floor()`](floor.md) for the throw example).
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::floor`](floor.md) / [`round()`](round.md): round
  down, or to the nearest boundary, instead of always up.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::ceil`](../registry-timeline/ceil.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="date.html">
    <span class="label">Next</span>
    <span class="title">date</span>
  </a>
</div>
