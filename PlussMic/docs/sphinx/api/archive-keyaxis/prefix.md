<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2b5f32bf7dc1 -->

# ArchiveKeyAxisAccessor::prefix

[Back to ArchiveKeyAxisAccessor API Reference](../../archive_keyaxis_accessor_api_reference)

## Signature

```cpp
Archive ArchiveKeyAxisAccessor::prefix(std::string_view prefix) const
```

## What It Does

Returns a new archive where **every catalog** in the stack has been narrowed to only
the keys whose name **starts with** `prefix` — each catalog filtered independently, by
delegating to that catalog's own
[`CatalogKeyAxisAccessor::prefix`](../catalog-keyaxis/prefix.md).

```{important} Each catalog is filtered independently — no cross-catalog key uniformity required
Same rule as [`like()`](like.md): `prefix()` never validates keys across catalogs. A
ragged archive (different key sets per catalog) works without throwing, and the result
can end up just as ragged.
```

## Parameters

**prefix** (`std::string_view`)
    The exact leading substring each surviving key name must start with. Matching is
    case-sensitive and has no separate `case_sensitive` toggle.

## Returns

`Archive`
    A new archive of the same depth, with each catalog narrowed to its own
    prefix-matching keys.

## Examples

Example 1. `prefix("pre_")` keeps only the one key that starts with `"pre_"`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "pre_a"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "b_suf"_k = pluss::kindle::Stream(pluss::make_series({2.0})),
    "other"_k = pluss::kindle::Stream(pluss::make_series({3.0})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.keyaxis().prefix("pre_");
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [pre_a]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
pre_a                    | Series       | Series[float64]  | 1       | [1]                                              
========================================================================================================================
```

## Notes

- See the important callout above.
- Case-sensitive, unconditionally — unlike [`like()`](like.md), there is no
  `case_sensitive` parameter to toggle this.
- A catalog with zero matches becomes a valid, defined, empty catalog in the result.

## See Also

- [`ArchiveKeyAxisAccessor::suffix`](suffix.md): the trailing-anchor counterpart.
- [`ArchiveKeyAxisAccessor::like`](like.md): the unanchored substring search.
- [`CatalogKeyAxisAccessor::prefix`](../catalog-keyaxis/prefix.md): the single-catalog
  delegate this method applies to every catalog independently.
- [`Archive::keyaxis`](../archive/keyaxis.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
