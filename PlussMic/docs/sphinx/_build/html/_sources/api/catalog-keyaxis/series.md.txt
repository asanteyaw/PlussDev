<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ca2320a990ce -->

# CatalogKeyAxisAccessor::series

[Back to CatalogKeyAxisAccessor API Reference](../../catalog_keyaxis_accessor_api_reference)

## Signature

```cpp
Catalog CatalogKeyAxisAccessor::series() const
```

## What It Does

Returns a new catalog containing only the keys whose `Stream::kind()` is
`StreamKind::Series`.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog containing only the `Series`-kind keys.

## Examples

Example 1. Select the numeric key(s) out of a mixed-kind catalog.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"})),
});
auto out = ct.keyaxis().series();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 2       | [1, 2]                                           
========================================================================================================================
```

## Notes

- A `Series`-kind filter, not a cast — a `Note` key doesn't become `Series`
  here; it's simply excluded. Use [`Catalog::cast().series()`](../catalog-cast/series.md)
  to actually convert kinds.
- An empty result is a valid, defined, empty catalog — not a throw.

## See Also

- [`CatalogKeyAxisAccessor::note`](note.md) / [`timeline()`](timeline.md) /
  : the shorthands for the other scalar kinds.
- [`Catalog::keyaxis`](../catalog/keyaxis.md): the entry point this accessor
  is reached from.
- [`RegistryFieldAxisAccessor::series`](../registry-fieldaxis/series.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prefix.html">
    <span class="label">Previous</span>
    <span class="title">prefix</span>
  </a>
  <a class="next-link" href="suffix.html">
    <span class="label">Next</span>
    <span class="title">suffix</span>
  </a>
</div>
