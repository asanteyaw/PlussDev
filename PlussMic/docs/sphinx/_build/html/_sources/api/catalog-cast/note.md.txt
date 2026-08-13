<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eee14176ef52 -->

# CatalogCastAccessor::note

[Back to CatalogCastAccessor API Reference](../../catalog_cast_accessor_api_reference)

## Signature

```cpp
Catalog CatalogCastAccessor::note() const
```

## What It Does

Casts every bound key's values to `Note`-kind (text) and returns a new catalog
with those keys replaced. Unlike casting toward `Series`, `Note`-conversion
always succeeds — every domain kind has a printable text form.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key cast to `Note`-kind.

## Examples

Example 1. Cast a numeric key to text.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
});
auto out = ct.cast().note();
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
id                       | Note         | Note             | 3       | [1, 2, 3]                                        
========================================================================================================================
```

## Notes

- Bound via [`Catalog::cast()`](../catalog/cast.md) — remember that entry
  point applies to *every* key of the catalog it's called on, so scope it with
  [`Catalog::keys(names)`](../catalog/keys.md) first if you only want to cast
  some keys.
- Unlike [`series()`](series.md), casting to `Note` doesn't have a
  silent-failure mode — any value can be rendered as text.

## See Also

- [`CatalogCastAccessor::series`](series.md): the inverse direction, with a
  silent-null failure mode for unparseable text.
- [`Catalog::cast`](../catalog/cast.md): the entry point this accessor is
  reached from.
- [`RegistryCastAccessor::note`](../registry-cast/note.md): the bundle-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
