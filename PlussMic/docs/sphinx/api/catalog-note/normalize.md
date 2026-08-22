<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ce8d154b85b3 -->

# CatalogNoteAccessor::normalize

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::normalize(
    pluss::table::UnicodeForm form = pluss::table::UnicodeForm::NFC) const
```

## What It Does

Applies Unicode normalization to every bound key's text, returning a new catalog with
those keys replaced. Normalization matters when text that looks identical can be
represented by different underlying byte sequences (e.g. a composed accented
character vs. the base letter plus a combining accent mark) — normalizing first makes
such text comparable and matchable consistently.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**form** (`pluss::table::UnicodeForm`, defaults to `NFC`)
    Which Unicode normalization form to apply — `NFC` (canonical composition, the
    default) or the library's other supported forms.

## Returns

`Catalog`
    A new catalog with every bound key's text normalized.

## Examples

Example 1. Plain ASCII text is unaffected by normalization (no multi-byte or
combining characters to normalize) — included as a baseline confirming the call
succeeds and returns unmodified `Note` output for such input.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra"})),
});
auto out = ct.nt().normalize();
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
city                     | Note         | Note             | 1       | [Accra]                                          
========================================================================================================================
```

## Notes

- See the important callout above.
- The default form, `NFC`, is the most common choice for general-purpose text
  comparison and storage.
- Has no visible effect on plain ASCII text, since ASCII has no composed/decomposed
  character distinction to normalize.

## See Also

- [`CatalogNoteAccessor::encode`](encode.md): the other byte/text-level transform on
  this accessor.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::normalize`](../registry-note/normalize.md): the scribble-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matches.html">
    <span class="label">Previous</span>
    <span class="title">matches</span>
  </a>
  <a class="next-link" href="pad.html">
    <span class="label">Next</span>
    <span class="title">pad</span>
  </a>
</div>
