<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d75a5d8f030d -->

# CatalogNoteAccessor::starts

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::starts(std::string_view prefix) const
```

## What It Does

Tests whether every bound key's text starts with `prefix` (a literal string, not a
pattern), replacing each `Note`-kind key with a `Bool` `Series`-kind key of results.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**prefix** (`std::string_view`)
    The literal text to test for at the start of each value — always literal, never a
    regex pattern.

## Returns

`Catalog`
    A new catalog with every bound key replaced by a `Bool` `Series` of per-value
    prefix-match results.

## Examples

Example 1.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Kumasi"})),
});
auto out = ct.nt().starts("Ac");
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
city                     | Series       | Series[bool]     | 2       | [true, false]                                    
========================================================================================================================
```

## Notes

- See the important callout above.
- Changes each bound key's kind from `Note` to `Series` (`Bool`).
- `prefix` is always literal text — unlike [`has()`](has.md)/[`count()`](count.md),
  there is no `TextOptions` overload to switch to regex matching.
- Case-sensitive; there is no built-in case-insensitive option for this method.

## See Also

- [`CatalogNoteAccessor::ends`](ends.md): the suffix-matching counterpart.
- [`CatalogNoteAccessor::has`](has.md): a substring (anywhere-in-value) test instead of
  a start-anchored one, with optional regex/case-insensitive matching.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::starts`](../registry-note/starts.md): the scribble-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split.html">
    <span class="label">Previous</span>
    <span class="title">split</span>
  </a>
  <a class="next-link" href="strip.html">
    <span class="label">Next</span>
    <span class="title">strip</span>
  </a>
</div>
