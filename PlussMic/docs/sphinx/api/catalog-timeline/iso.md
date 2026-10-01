<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 10e3e057677a -->

# CatalogTimelineAccessor::iso

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::iso() const
```

## What It Does

Renders each bound key's timestamp as a fixed ISO-8601-with-offset text
representation, replacing each `Timeline`-kind key with a `Note`
(text) key. Equivalent to [`fmt("%Y-%m-%dT%H:%M:%S.%f%z")`](fmt.md).

```{important} CatalogTimelineAccessor requires every bound key to be Timeline-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key replaced by a `Note` key holding each row
    as `"YYYY-MM-DDTHH:MM:SS.fffffffff+HH:MM"`.

## Examples

Example 1. Render a timestamp as ISO-8601 text.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
auto out = ct.tl().iso();
std::cout << out.keys("ts").as_note().at(0);
```

### Output

```text
2024-01-31T09:15:30.000000000+00:00
```

## Notes

- Always includes the full 9-digit nanosecond fraction and a signed `%z`
  offset — use [`fmt()`](fmt.md) with your own pattern for a more compact
  string.
- Changes each bound key's kind from `Timeline` to `Note`.
- See the important callout above.

## See Also

- [`CatalogTimelineAccessor::fmt`](fmt.md): the general form this method is a
  fixed pattern of.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::iso`](../registry-timeline/iso.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnat.html">
    <span class="label">Previous</span>
    <span class="title">isnat</span>
  </a>
  <a class="next-link" href="leap.html">
    <span class="label">Next</span>
    <span class="title">leap</span>
  </a>
</div>
