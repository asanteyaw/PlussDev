<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 780cefb649bf -->

# CatalogNoteAccessor::lower

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::lower() const
```

## What It Does

Converts every bound key's text to lowercase, returning a new catalog with those keys
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
    A new catalog with every bound key's text lowercased.

## Examples

Example 1.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "TEMA"})),
});
auto out = ct.nt().lower();
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
city                     | Note         | Note             | 2       | [accra, tema]                                    
========================================================================================================================
```

## Notes

- See the important callout above.
- For a Unicode-normalizing lowercase-like transform instead of a simple ASCII-style
  case conversion, see [`casefold()`](casefold.md) — the two agree for plain ASCII
  input (as in the example above), but `casefold()` is the more robust choice for
  case-insensitive *comparisons* of text that may contain non-ASCII characters.

## See Also

- [`CatalogNoteAccessor::upper`](upper.md): the inverse conversion.
- [`CatalogNoteAccessor::casefold`](casefold.md): a Unicode-aware alternative
  purpose-built for case-insensitive comparison.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::lower`](../registry-note/lower.md): the scribble-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="length.html">
    <span class="label">Previous</span>
    <span class="title">length</span>
  </a>
  <a class="next-link" href="lstrip.html">
    <span class="label">Next</span>
    <span class="title">lstrip</span>
  </a>
</div>
