<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 486e7d948ddd -->

# ArchiveKeyAxisAccessor::suffix

[Back to ArchiveKeyAxisAccessor API Reference](../../archive_keyaxis_accessor_api_reference)

## Signature

```cpp
Archive ArchiveKeyAxisAccessor::suffix(std::string_view suffix) const
```

## What It Does

Returns a new archive where **every catalog** in the stack has been narrowed to only
the keys whose name **ends with** `suffix` — each catalog filtered independently, by
delegating to that catalog's own
[`CatalogKeyAxisAccessor::suffix`](../catalog-keyaxis/suffix.md).

```{important} Each catalog is filtered independently — no cross-catalog key uniformity required
Same rule as [`like()`](like.md): `suffix()` never validates keys across catalogs. A
ragged archive (different key sets per catalog) works without throwing, and the result
can end up just as ragged.
```

## Parameters

**suffix** (`std::string_view`)
    The exact trailing substring each surviving key name must end with. Matching is
    case-sensitive and has no separate `case_sensitive` toggle.

## Returns

`Archive`
    A new archive of the same depth, with each catalog narrowed to its own
    suffix-matching keys.

## Examples

Example 1. `suffix("_suf")` keeps only the one key that ends with `"_suf"`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "pre_a"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "b_suf"_k = pluss::kindle::Stream(pluss::make_series({2.0})),
    "other"_k = pluss::kindle::Stream(pluss::make_series({3.0})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.keyaxis().suffix("_suf");
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [b_suf]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
b_suf                    | Series       | Series[float64]  | 1       | [2]                                              
========================================================================================================================
```

## Notes

- See the important callout above.
- Case-sensitive, unconditionally — unlike [`like()`](like.md), there is no
  `case_sensitive` parameter to toggle this.
- A catalog with zero matches becomes a valid, defined, empty catalog in the result.

## See Also

- [`ArchiveKeyAxisAccessor::prefix`](prefix.md): the leading-anchor counterpart.
- [`ArchiveKeyAxisAccessor::like`](like.md): the unanchored substring search.
- [`CatalogKeyAxisAccessor::suffix`](../catalog-keyaxis/suffix.md): the single-catalog
  delegate this method applies to every catalog independently.
- [`Archive::keyaxis`](../archive/keyaxis.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series.html">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
  <a class="next-link" href="timeindex.html">
    <span class="label">Next</span>
    <span class="title">timeindex</span>
  </a>
</div>
