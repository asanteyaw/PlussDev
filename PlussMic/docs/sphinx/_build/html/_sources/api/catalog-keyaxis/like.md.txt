<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 549d978787f9 -->

# CatalogKeyAxisAccessor::like

[Back to CatalogKeyAxisAccessor API Reference](../../catalog_keyaxis_accessor_api_reference)

## Signature

```cpp
Catalog CatalogKeyAxisAccessor::like(std::string_view needle, bool case_sensitive = true) const
```

## What It Does

Returns a new catalog containing only the keys whose **name** contains
`needle` as a substring.

```{important} Defaults to case-sensitive, unlike RegistryFieldAxisAccessor::like
`CatalogKeyAxisAccessor::like()` defaults `case_sensitive` to `true`. The
bundle-model equivalent, [`RegistryFieldAxisAccessor::like`](../registry-fieldaxis/like.md),
defaults to `false` (case-insensitive) instead — a real asymmetry between the
two models' accessors, not just a naming difference. Pass `false` explicitly
here for a case-insensitive substring search.
```

## Parameters

**needle** (`std::string_view`)
    The substring to search for within each key name.

**case_sensitive** (`bool`, optional, defaults to `true`)
    Whether the substring search is case-sensitive.

## Returns

`Catalog`
    A new catalog containing only the keys whose name contains `needle`.

## Examples

Example 1. Default (case-sensitive) search matches both `id` and `user_id`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "user_id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
});
auto out = ct.keyaxis().like("id");
std::cout << out;
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

Example 2 (edge case). A case-mismatched search against the default
`case_sensitive = true` matches nothing — see the important callout above.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "user_id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})),
});
auto out = ct.keyaxis().like("ID");
std::cout << "nkeys=" << out.nkeys();
```

### Output

```text
nkeys=0
```

Example 3. Pass `false` explicitly to get case-insensitive matching.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "user_id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})),
});
auto out = ct.keyaxis().like("ID", false);
std::cout << out;
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

## Notes

- See the important callout above — this method's default differs from its
  bundle-model counterpart.
- Matches against key **names**, not key values.
- An empty result is a valid, defined, empty catalog — not a throw.

## See Also

- [`CatalogKeyAxisAccessor::match`](match.md): the regex-based counterpart,
  for more precise patterns than a plain substring.
- [`CatalogKeyAxisAccessor::prefix`](prefix.md) / [`suffix()`](suffix.md):
  anchored variants of this same substring search.
- [`Catalog::keyaxis`](../catalog/keyaxis.md): the entry point this accessor
  is reached from.
- [`Catalog::keys`](../catalog/keys.md): select keys you already know the
  exact names of, rather than by pattern.
- [`RegistryFieldAxisAccessor::like`](../registry-fieldaxis/like.md): the
  bundle-model equivalent, with the opposite default.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="match.html">
    <span class="label">Next</span>
    <span class="title">match</span>
  </a>
</div>
