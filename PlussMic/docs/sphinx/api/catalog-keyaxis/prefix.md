<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 33048dc9d84f -->

# CatalogKeyAxisAccessor::prefix

[Back to CatalogKeyAxisAccessor API Reference](../../catalog_keyaxis_accessor_api_reference)

## Signature

```cpp
Catalog CatalogKeyAxisAccessor::prefix(std::string_view prefix) const
```

## What It Does

Returns a new catalog containing only the keys whose **name** starts with
`prefix`. Case-sensitive; there is no `case_sensitive` option on this method,
unlike [`like()`](like.md).

```{important} Filters keys — doesn't rename them
`prefix("user_")` means "keep only keys starting with `user_`". It does
**not** strip the prefix or rename the retained keys — see
[`Catalog::rename`](../catalog/rename.md) if you actually want to transform
key names.
```

## Parameters

**prefix** (`std::string_view`)
    The prefix each key name must start with.

## Returns

`Catalog`
    A new catalog containing only the keys whose name starts with `prefix`.

## Examples

Example 1. Select every key name starting with `user`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "user_id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
});
auto out = ct.keyaxis().prefix("user");
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

The retained key is still named `user_id` in full — see the important callout
above.

## Notes

- Case-sensitive — no `case_sensitive` parameter, unlike [`like()`](like.md).
- An empty result is a valid, defined, empty catalog — not a throw.
- See the important callout above.

## See Also

- [`CatalogKeyAxisAccessor::suffix`](suffix.md): the mirror-image,
  ends-with counterpart.
- [`CatalogKeyAxisAccessor::like`](like.md): unanchored substring search,
  anywhere in the name.
- [`CatalogKeyAxisAccessor::match`](match.md): regex-based matching, for
  patterns this method and `suffix()` can't express.
- [`Catalog::keyaxis`](../catalog/keyaxis.md): the entry point this accessor
  is reached from.
- [`RegistryFieldAxisAccessor::prefix`](../registry-fieldaxis/prefix.md): the
  scribble-model equivalent.

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
