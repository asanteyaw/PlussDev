<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 51af2f7509be -->

# CatalogNoteAccessor::ends

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::ends(std::string_view suffix) const
```

## What It Does

Tests whether every bound key's text ends with `suffix` (a literal string, not a
pattern), replacing each `Note`-kind key with a `Bool` `Series`-kind key of results.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**suffix** (`std::string_view`)
    The literal text to test for at the end of each value — always literal, never a
    regex pattern.

## Returns

`Catalog`
    A new catalog with every bound key replaced by a `Bool` `Series` of per-value
    suffix-match results.

## Examples

Example 1.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Kumasi"})),
});
auto out = ct.nt().ends("si");
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
city                     | Series       | Series[bool]     | 2       | [false, true]                                    
========================================================================================================================
```

## Notes

- See the important callout above.
- Changes each bound key's kind from `Note` to `Series` (`Bool`).
- `suffix` is always literal text — there is no `TextOptions` overload to switch to
  regex matching.
- Case-sensitive; there is no built-in case-insensitive option for this method.

## See Also

- [`CatalogNoteAccessor::starts`](starts.md): the prefix-matching counterpart.
- [`CatalogNoteAccessor::has`](has.md): a substring (anywhere-in-value) test instead of
  an end-anchored one.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::ends`](../registry-note/ends.md): the scribble-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="encode.html">
    <span class="label">Previous</span>
    <span class="title">encode</span>
  </a>
  <a class="next-link" href="findall.html">
    <span class="label">Next</span>
    <span class="title">findall</span>
  </a>
</div>
