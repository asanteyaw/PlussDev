<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7fe0f907f824 -->

# CatalogNoteAccessor::length

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::length() const
```

## What It Does

Measures the character length of every bound key's text, replacing each `Note`-kind
key with an `Int64` `Series`-kind key of lengths.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key replaced by an `Int64` `Series` of per-value
    character lengths.

## Examples

Example 1.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Kumasi"})),
});
auto out = ct.nt().length();
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
city                     | Series       | Series[int64]    | 2       | [5, 6]                                           
========================================================================================================================
```

## Notes

- See the important callout above.
- Changes each bound key's kind from `Note` to `Series` (`Int64`) — unlike the
  case/trim/pad family, which stay `Note`-kind.
- Length is measured in characters, not bytes — matters for multi-byte UTF-8 text.

## See Also

- [`CatalogNoteAccessor::count`](count.md): the other method on this accessor that
  turns text into an `Int64` `Series`.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::length`](../registry-note/length.md): the scribble-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="indicate.html">
    <span class="label">Previous</span>
    <span class="title">indicate</span>
  </a>
  <a class="next-link" href="lower.html">
    <span class="label">Next</span>
    <span class="title">lower</span>
  </a>
</div>
