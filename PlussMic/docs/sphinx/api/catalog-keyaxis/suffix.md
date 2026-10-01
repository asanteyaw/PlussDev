<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f090b5ffd2a5 -->

# CatalogKeyAxisAccessor::suffix

[Back to CatalogKeyAxisAccessor API Reference](../../catalog_keyaxis_accessor_api_reference)

## Signature

```cpp
Catalog CatalogKeyAxisAccessor::suffix(std::string_view suffix) const
```

## What It Does

Returns a new catalog containing only the keys whose **name** ends with
`suffix`. Case-sensitive; there is no `case_sensitive` option on this method,
unlike [`like()`](like.md).

```{important} Filters keys — doesn't rename them
`suffix("_raw")` means "keep only keys ending with `_raw`". It does **not**
strip the suffix or rename the retained keys — see
[`Catalog::rename`](../catalog/rename.md) if you actually want to transform
key names.
```

## Parameters

**suffix** (`std::string_view`)
    The suffix each key name must end with.

## Returns

`Catalog`
    A new catalog containing only the keys whose name ends with `suffix`.

## Examples

Example 1. Select every key name ending with `_id`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "user_id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
});
auto out = ct.keyaxis().suffix("_id");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [user_id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
user_id                  | Series       | Series[float64]  | 2       | [3, 4]                                           
========================================================================================================================
```

## Notes

- Case-sensitive — no `case_sensitive` parameter, unlike [`like()`](like.md).
- An empty result is a valid, defined, empty catalog — not a throw.
- See the important callout above.

## See Also

- [`CatalogKeyAxisAccessor::prefix`](prefix.md): the mirror-image,
  starts-with counterpart.
- [`CatalogKeyAxisAccessor::like`](like.md): unanchored substring search,
  anywhere in the name.
- [`CatalogKeyAxisAccessor::match`](match.md): regex-based matching, for
  patterns this method and `prefix()` can't express.
- [`Catalog::keyaxis`](../catalog/keyaxis.md): the entry point this accessor
  is reached from.
- [`RegistryFieldAxisAccessor::suffix`](../registry-fieldaxis/suffix.md): the
  scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series.html">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
</div>
