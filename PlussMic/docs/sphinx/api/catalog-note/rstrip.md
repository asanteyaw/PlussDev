<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8b0227f4c2db -->

# CatalogNoteAccessor::rstrip

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signatures

```cpp
Catalog CatalogNoteAccessor::rstrip() const

Catalog CatalogNoteAccessor::rstrip(std::string_view chars) const
```

## What It Does

Trims characters from only the **right** (trailing) edge of every bound key's text,
returning a new catalog with those keys replaced. The no-argument overload trims
trailing whitespace; the `chars`-argument overload trims any of the given characters
from the right edge only, stopping at the first character that isn't in the set.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from the right edge only.

## Returns

`Catalog`
    A new catalog with every bound key's text right-trimmed; leading characters are
    always left untouched.

## Examples

Example 1. No-argument overload trims only trailing whitespace — leading whitespace is
preserved, distinguishing this from [`strip()`](strip.md).

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"  Accra  ", "  Tema  "})),
});
auto out = ct.nt().rstrip();
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
city                     | Note         | Note             | 2       | [  Accra,   Tema]                                
========================================================================================================================
```

## Notes

- See the important callout above — this precondition applies to every method on
  `CatalogNoteAccessor`.
- Leading characters are always left untouched, regardless of `chars`.
- For a left-side or both-sides trim, see [`lstrip()`](lstrip.md) and
  [`strip()`](strip.md).

## See Also

- [`CatalogNoteAccessor::strip`](strip.md) / [`lstrip()`](lstrip.md): the both-sides and
  left-side variants.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::rstrip`](../registry-note/rstrip.md): the bundle-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace.html">
    <span class="label">Previous</span>
    <span class="title">replace</span>
  </a>
  <a class="next-link" href="split.html">
    <span class="label">Next</span>
    <span class="title">split</span>
  </a>
</div>
