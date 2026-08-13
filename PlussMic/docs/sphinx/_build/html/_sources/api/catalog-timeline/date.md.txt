<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: af02799bde2b -->

# CatalogTimelineAccessor::date

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::date() const
```

## What It Does

Strips the time-of-day from each bound key's timestamp (dropped to midnight),
replacing each bound key's values in place. Equivalent to
[`floor("D")`](floor.md).

```{important} CatalogTimelineAccessor requires every bound key to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key's time-of-day portion zeroed out.

## Examples

Example 1. Strip the time-of-day from two timestamps.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().date();
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.keys("ts").as_note().at(0) << " | " << fmted.keys("ts").as_note().at(1);
```

### Output

```text
2024-01-31 00:00:00 | 2024-02-29 00:00:00
```

## Notes

- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::time`](time.md): the mirror-image operation,
  keeping the time-of-day and zeroing the date.
- [`CatalogTimelineAccessor::floor`](floor.md): the general form `date()` is a
  fixed `"D"` unit of.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::date`](../registry-timeline/date.md): the
  bundle-model equivalent.

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
