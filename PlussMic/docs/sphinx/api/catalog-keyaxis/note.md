<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eee14176ef52 -->

# CatalogKeyAxisAccessor::note

[Back to CatalogKeyAxisAccessor API Reference](../../catalog_keyaxis_accessor_api_reference)

## Signature

```cpp
Catalog CatalogKeyAxisAccessor::note() const
```

## What It Does

Returns a new catalog containing only the keys whose `Stream::kind()` is
`StreamKind::Note`.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog containing only the `Note`-kind keys.

## Examples

Example 1. Select the text key(s) out of a mixed-kind catalog.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"})),
});
auto out = ct.keyaxis().note();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [city]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
city                     | Note         | Note             | 2       | [Accra, Tema]                                    
========================================================================================================================
```

## Notes

- A `Note`-kind filter, not a cast — a `Series` key doesn't become `Note`
  here; it's simply excluded. Use [`Catalog::cast().note()`](../catalog-cast/note.md)
  to actually convert kinds.
- An empty result is a valid, defined, empty catalog — not a throw.

## See Also

- [`CatalogKeyAxisAccessor::series`](series.md) / [`timeline()`](timeline.md) /
  [`timeindex()`](timeindex.md): the shorthands for the other scalar kinds.
- [`Catalog::keyaxis`](../catalog/keyaxis.md): the entry point this accessor
  is reached from.
- [`RegistryFieldAxisAccessor::note`](../registry-fieldaxis/note.md): the
  bundle-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="match.html">
    <span class="label">Previous</span>
    <span class="title">match</span>
  </a>
  <a class="next-link" href="prefix.html">
    <span class="label">Next</span>
    <span class="title">prefix</span>
  </a>
</div>
