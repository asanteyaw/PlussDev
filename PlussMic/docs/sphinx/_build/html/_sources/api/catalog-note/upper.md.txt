<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 04d58a21d223 -->

# CatalogNoteAccessor::upper

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::upper() const
```

## What It Does

Converts every bound key's text to uppercase, returning a new catalog with those keys
replaced.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key's text uppercased.

## Examples

Example 1.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "TEMA"})),
});
auto out = ct.nt().upper();
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
city                     | Note         | Note             | 2       | [ACCRA, TEMA]                                    
========================================================================================================================
```

## Notes

- See the important callout above.

## See Also

- [`CatalogNoteAccessor::lower`](lower.md): the inverse conversion.
- [`CatalogNoteAccessor::titlecase`](titlecase.md): capitalizes just the first letter
  of each word rather than the whole string.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::upper`](../registry-note/upper.md): the scribble-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="titlecase.html">
    <span class="label">Previous</span>
    <span class="title">titlecase</span>
  </a>
</div>
