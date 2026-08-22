<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 14f75546b619 -->

# CatalogTimelineAccessor::period

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::period(std::string_view freq) const
```

## What It Does

Replaces each bound key's timestamp with the *start* of the `freq`-sized period
it falls within, replacing each bound key's values in place. For the fixed
clock units, this is equivalent to [`floor(freq)`](floor.md); for the calendar
units it accepts, it snaps to the start of that calendar period (e.g. the
first of the month).

```{important} CatalogTimelineAccessor requires every bound key to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**freq** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported frequency throws.

## Returns

`Catalog`
    A new catalog with every bound key's timestamps replaced by their period
    start.

## Examples

Example 1. Snap a mid-day timestamp to its containing day's start.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
auto out = ct.tl().period("D");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.keys("ts").as_note().at(0);
```

### Output

```text
2024-01-31 00:00:00
```

## Notes

- For the fixed units this method accepts, `period(freq)` behaves the same as
  [`floor(freq)`](floor.md) — see that page for the unrecognized-unit throw
  text.
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::floor`](floor.md): the underlying rounding
  behavior this method shares for fixed units.
- [`CatalogTimelineAccessor::shift`](shift.md): move each timestamp by a fixed
  offset instead of snapping to a period boundary.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::period`](../registry-timeline/period.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_start.html">
    <span class="label">Previous</span>
    <span class="title">month_start</span>
  </a>
  <a class="next-link" href="quarter.html">
    <span class="label">Next</span>
    <span class="title">quarter</span>
  </a>
</div>
