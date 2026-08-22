<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5fb4618781ba -->

# CatalogNoteAccessor::casefold

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::casefold() const
```

## What It Does

Converts every bound key's text to a case-insensitive-comparison-safe form, returning
a new catalog with those keys replaced. For plain ASCII text, `casefold()` produces the
same result as [`lower()`](lower.md) (see the example below) — the distinction matters
for non-ASCII text, where casefolding follows Unicode's full case-folding rules rather
than a simple per-character lowercase mapping, making it the more robust choice when
the goal is comparing two strings for equality regardless of case.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key's text casefolded.

## Examples

Example 1 (verification, not a distinct rendering). For plain ASCII input, `casefold()`
produces byte-identical output to [`lower()`](lower.md).

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "TEMA"})),
});
auto out = ct.nt().casefold();
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
- Prefer `casefold()` over `lower()` when the intent is case-insensitive
  *comparison*, even though both agree on plain ASCII text.

## See Also

- [`CatalogNoteAccessor::lower`](lower.md): the simpler, non-Unicode-aware lowercase
  conversion this method agrees with on ASCII text.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::casefold`](../registry-note/casefold.md): the scribble-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="chars.html">
    <span class="label">Next</span>
    <span class="title">chars</span>
  </a>
</div>
