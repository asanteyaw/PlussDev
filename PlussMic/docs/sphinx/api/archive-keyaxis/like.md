<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 549229b173c7 -->

# ArchiveKeyAxisAccessor::like

[Back to ArchiveKeyAxisAccessor API Reference](../../archive_keyaxis_accessor_api_reference)

## Signature

```cpp
Archive ArchiveKeyAxisAccessor::like(std::string_view needle, bool case_sensitive = true) const
```

## What It Does

Returns a new archive where **every catalog** in the stack has been narrowed to only
the keys whose name contains `needle` as a substring — each catalog filtered
independently, by delegating to that catalog's own
[`CatalogKeyAxisAccessor::like`](../catalog-keyaxis/like.md).

```{important} Each catalog is filtered independently — no cross-catalog key uniformity required
Unlike [`Archive::tl()`](../archive/tl.md)/[`ts()`](../archive/ts.md)/[`cast()`](../archive/cast.md),
which bind a single key set (from `catalog(0)`) and require every catalog to share it,
`ArchiveKeyAxisAccessor` methods never validate keys across catalogs — each catalog runs
its own independent substring search against its own key names. A ragged archive (whose
catalogs have different key sets) works without throwing, and the result can end up
just as ragged: one catalog might match two keys, another might match zero, another
might match keys with entirely different names. See Example 2.
```

```{important} Defaults to case-sensitive, unlike RegistryFieldAxisAccessor::like
Same default as [`CatalogKeyAxisAccessor::like`](../catalog-keyaxis/like.md):
`case_sensitive` defaults to `true`. The bundle-model equivalent,
[`RegistryFieldAxisAccessor::like`](../registry-fieldaxis/like.md), defaults to `false`
instead. Pass `false` explicitly here for a case-insensitive substring search.
```

## Parameters

**needle** (`std::string_view`)
    The substring to search for within each catalog's key names.

**case_sensitive** (`bool`, optional, defaults to `true`)
    Whether the substring search is case-sensitive.

## Returns

`Archive`
    A new archive of the same depth, with each catalog narrowed to its own
    substring-matching keys.

## Examples

Example 1. A search that matches within a single catalog.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k      = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "user_id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})),
    "city"_k    = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.keyaxis().like("id");
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, user_id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 2       | [1, 2]                                           
user_id                  | Series       | Series[float64]  | 2       | [3, 4]                                           
========================================================================================================================
```

Example 2. A ragged archive — each catalog has a completely different key set — is
filtered independently, without throwing, per the first important callout above.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k      = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "user_id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})),
});
auto c1 = pluss::kindle::Catalog({
    "identity"_k = pluss::kindle::Stream(pluss::make_series({5.0})),
    "other"_k    = pluss::kindle::Stream(pluss::make_series({6.0})),
});
auto archive = pluss::kindle::Archive({c0, c1});

auto out = archive.keyaxis().like("id");
std::cout << out.catalog(0);
std::cout << out.catalog(1);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, user_id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 2       | [1, 2]                                           
user_id                  | Series       | Series[float64]  | 2       | [3, 4]                                           
========================================================================================================================
========================================================================================================================
kindle::Catalog  keys=1
keys: [identity]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
identity                 | Series       | Series[float64]  | 1       | [5]                                              
========================================================================================================================
```

`c0` keeps 2 matching keys, `c1` keeps only 1 (`"other"` doesn't contain `"id"`) — the
two catalogs end up with different key counts, and no error is raised for that.

## Notes

- See both important callouts above.
- Matches against key **names**, not key values.
- A catalog with zero matches becomes a valid, defined, empty catalog in the result —
  not a throw, and not a reason to drop that depth position from the stack.

## See Also

- [`ArchiveKeyAxisAccessor::match`](match.md): the regex-based counterpart, for more
  precise patterns than a plain substring.
- [`ArchiveKeyAxisAccessor::prefix`](prefix.md) / [`suffix()`](suffix.md): anchored
  variants of this same substring search.
- [`CatalogKeyAxisAccessor::like`](../catalog-keyaxis/like.md): the single-catalog
  delegate this method applies to every catalog independently.
- [`Archive::keyaxis`](../archive/keyaxis.md): the entry point this accessor is reached
  from.
- [`Archive::keys`](../archive/keys.md): select keys you already know the exact names
  of, rather than by pattern — and which *does* require every catalog to share them.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="match.html">
    <span class="label">Next</span>
    <span class="title">match</span>
  </a>
</div>
