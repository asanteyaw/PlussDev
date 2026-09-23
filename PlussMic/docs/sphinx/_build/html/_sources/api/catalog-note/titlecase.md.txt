<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3ee392aa5b56 -->

# CatalogNoteAccessor::titlecase

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::titlecase() const
```

## What It Does

Capitalizes the first letter of each whitespace-separated word in every bound key's
text, lowercasing the rest, and returns a new catalog with those keys replaced.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key's text titlecased.

## Examples

Example 1. Multi-word text: each word's first letter is capitalized independently.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"accra city", "tema"})),
});
auto out = ct.nt().titlecase();
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
city                     | Note         | Note             | 2       | [Accra City, Tema]                               
========================================================================================================================
```

## Notes

- See the important callout above.
- Word boundaries are determined by whitespace — punctuation-adjacent letters (e.g.
  the letter right after an apostrophe) are not separately capitalized.

## See Also

- [`CatalogNoteAccessor::upper`](upper.md): capitalizes the entire string, not just the
  first letter of each word.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::titlecase`](../registry-note/titlecase.md): the scribble-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip.html">
    <span class="label">Previous</span>
    <span class="title">strip</span>
  </a>
  <a class="next-link" href="upper.html">
    <span class="label">Next</span>
    <span class="title">upper</span>
  </a>
</div>
