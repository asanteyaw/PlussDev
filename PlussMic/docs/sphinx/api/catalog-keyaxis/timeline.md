<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0ae330b7d3cb -->

# CatalogKeyAxisAccessor::timeline

[Back to CatalogKeyAxisAccessor API Reference](../../catalog_keyaxis_accessor_api_reference)

## Signature

```cpp
Catalog CatalogKeyAxisAccessor::timeline() const
```

## What It Does

Returns a new catalog containing only the keys whose `Stream::kind()` is
`StreamKind::Timeline`.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog containing only the `Timeline`-kind keys.

## Examples

Example 1. Select the timestamp key(s) out of a mixed-kind catalog.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"})),
});
auto out = ct.keyaxis().timeline();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [ts]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
ts                       | Timeline     | Timeline         | 2       | [2024-01-01T00:00:00.000~, 2024-01-02T00:00:00.0~
========================================================================================================================
```

## Notes

- A `Timeline`-kind filter, not a cast — a `TimeIndex` key doesn't count as
  `Timeline` here, per the important callout on [`timeindex()`](timeindex.md).
- An empty result is a valid, defined, empty catalog — not a throw.

## See Also

- [`CatalogKeyAxisAccessor::timeindex`](timeindex.md): the sibling filter for
  `TimeIndex`-kind keys.
- [`CatalogKeyAxisAccessor::series`](series.md) / [`note()`](note.md): the
  shorthands for the other scalar kinds.
- [`Catalog::keyaxis`](../catalog/keyaxis.md): the entry point this accessor
  is reached from.
- [`RegistryFieldAxisAccessor::timeline`](../registry-fieldaxis/timeline.md):
  the scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timeindex.html">
    <span class="label">Previous</span>
    <span class="title">timeindex</span>
  </a>
</div>
