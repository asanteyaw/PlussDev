<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ca2320a990ce -->

# CatalogCastAccessor::series

[Back to CatalogCastAccessor API Reference](../../catalog_cast_accessor_api_reference)

## Signature

```cpp
Catalog CatalogCastAccessor::series() const
```

## What It Does

Casts every bound key's values to `Series`-kind and returns a new catalog with
those keys replaced.

```{important} Unparseable text becomes null, not a throw
Casting a `Note` value that can't be parsed as numeric (e.g. `"Accra"`)
doesn't throw — it silently becomes a null value in the resulting key. Check
[`Catalog::isnull`](../catalog/isnull.md) after casting if you need to detect
values that failed to convert.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key cast to `Series`-kind.

## Examples

Example 1. Cast a `Note` key of numeric-looking text to `Series`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_note({"1", "2", "3"})),
});
auto out = ct.cast().series();
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
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
========================================================================================================================
```

Example 2 (edge case). Unparseable text becomes null rather than throwing —
see the important callout above.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
});
auto out = ct.cast().series();
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
city                     | Series       | Series[float64]  | 2       | [NA, NA]                                         
========================================================================================================================
```

## Notes

- Bound via [`Catalog::cast()`](../catalog/cast.md) — remember that entry
  point applies to *every* key of the catalog it's called on, so scope it with
  [`Catalog::keys(names)`](../catalog/keys.md) first if you only want to cast
  some keys.
- See the important callout above for the silent-null behavior on unparseable
  text.

## See Also

- [`CatalogCastAccessor::note`](note.md): the inverse direction, casting
  numeric keys to text.
- [`Catalog::cast`](../catalog/cast.md): the entry point this accessor is
  reached from.
- [`RegistryCastAccessor::series`](../registry-cast/series.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
</div>
