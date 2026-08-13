<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 385e693d00bf -->

# CatalogNoteAccessor::lstrip

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signatures

```cpp
Catalog CatalogNoteAccessor::lstrip() const

Catalog CatalogNoteAccessor::lstrip(std::string_view chars) const
```

## What It Does

Trims characters from only the **left** (leading) edge of every bound key's text,
returning a new catalog with those keys replaced. The no-argument overload trims
leading whitespace; the `chars`-argument overload trims any of the given characters
from the left edge only, stopping at the first character that isn't in the set.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from the left edge only.

## Returns

`Catalog`
    A new catalog with every bound key's text left-trimmed; trailing characters are
    always left untouched.

## Examples

Example 1. No-argument overload trims only leading whitespace — trailing whitespace is
preserved, distinguishing this from [`strip()`](strip.md).

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"  Accra  ", "  Tema  "})),
});
auto out = ct.nt().lstrip();
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
city                     | Note         | Note             | 2       | [Accra  , Tema  ]                                
========================================================================================================================
```

## Notes

- See the important callout above — this precondition applies to every method on
  `CatalogNoteAccessor`.
- Trailing characters are always left untouched, regardless of `chars`.
- For a right-side or both-sides trim, see [`rstrip()`](rstrip.md) and
  [`strip()`](strip.md).

## See Also

- [`CatalogNoteAccessor::strip`](strip.md) / [`rstrip()`](rstrip.md): the both-sides and
  right-side variants.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::lstrip`](../registry-note/lstrip.md): the bundle-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lower.html">
    <span class="label">Previous</span>
    <span class="title">lower</span>
  </a>
  <a class="next-link" href="matches.html">
    <span class="label">Next</span>
    <span class="title">matches</span>
  </a>
</div>
