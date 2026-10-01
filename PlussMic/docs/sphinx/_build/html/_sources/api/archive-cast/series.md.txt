<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bbecc9a7400b -->

# ArchiveCastAccessor::series

[Back to ArchiveCastAccessor API Reference](../../archive_cast_accessor_api_reference)

## Signature

```cpp
Archive ArchiveCastAccessor::series() const
```

## What It Does

Casts the bound key(s) to `Series`-kind in **every catalog** of the stack and returns a
new archive with those keys replaced — one call per catalog, delegating to that
catalog's own [`CatalogCastAccessor::series`](../catalog-cast/series.md).

```{important} Unparseable text becomes null, not a throw
Same as [`CatalogCastAccessor::series`](../catalog-cast/series.md): casting a `Note`
value that can't be parsed as numeric (e.g. `"Accra"`) doesn't throw — it silently
becomes a null value in the resulting key, in whichever catalog(s) contain it. Check
[`Archive::isnull`](../archive/isnull.md) after casting if you need to detect values
that failed to convert.
```

## Parameters

`None`

## Returns

`Archive`
    A new archive of the same depth, with every bound key cast to `Series`-kind in
    every catalog.

## Examples

Example 1. Cast a `Note`-kind key of numeric-looking text to `Series`, in both
catalogs of a 2-deep archive.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_note({"1", "2"})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_note({"3", "4"})) });
auto archive = pluss::kindle::Archive({c0, c1});

auto out = archive.cast().series();
std::cout << out.catalog(0);
std::cout << out.catalog(1);
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
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 2       | [3, 4]                                           
========================================================================================================================
```

Example 2 (edge case). Unparseable text becomes null rather than throwing — see the
important callout above.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})) });
auto archive = pluss::kindle::Archive({c0});

auto out = archive.cast().series();
std::cout << out.catalog(0);
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

- Bound via [`Archive::cast()`](../archive/cast.md) — remember that entry point binds
  `catalog(0)`'s key set and requires every catalog to share it (throws immediately at
  `cast()` otherwise); it doesn't re-validate per method.
- See the important callout above for the silent-null behavior on unparseable text.

## See Also

- [`ArchiveCastAccessor::note`](note.md): the inverse direction, casting numeric keys
  to text.
- [`CatalogCastAccessor::series`](../catalog-cast/series.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::cast`](../archive/cast.md): the entry point this accessor is reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
</div>
