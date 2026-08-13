<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 682ff91d4e09 -->

# ArchiveCastAccessor::note

[Back to ArchiveCastAccessor API Reference](../../archive_cast_accessor_api_reference)

## Signature

```cpp
Archive ArchiveCastAccessor::note() const
```

## What It Does

Casts the bound key(s) to `Note`-kind (text) in **every catalog** of the stack and
returns a new archive with those keys replaced — one call per catalog, delegating to
that catalog's own [`CatalogCastAccessor::note`](../catalog-cast/note.md).

## Parameters

`None`

## Returns

`Archive`
    A new archive of the same depth, with every bound key cast to `Note`-kind in every
    catalog.

## Examples

Example 1. Cast a numeric key to text formatting.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "n"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.5})) });
auto archive = pluss::kindle::Archive({c0});

auto out = archive.cast().note();
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [n]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
n                        | Note         | Note             | 2       | [1, 2.5]                                         
========================================================================================================================
```

## Notes

- Bound via [`Archive::cast()`](../archive/cast.md) — remember that entry point binds
  `catalog(0)`'s key set and requires every catalog to share it.
- Unlike [`series()`](series.md), this direction never produces nulls — every value
  (numeric, text, or timestamp) has a well-defined text representation.

## See Also

- [`ArchiveCastAccessor::series`](series.md): the inverse direction, casting text keys
  to numeric.
- [`CatalogCastAccessor::note`](../catalog-cast/note.md): the single-catalog delegate
  this method applies to every catalog.
- [`Archive::cast`](../archive/cast.md): the entry point this accessor is reached from.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
