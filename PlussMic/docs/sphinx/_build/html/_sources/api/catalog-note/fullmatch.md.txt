<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1b3bb55ea463 -->

# CatalogNoteAccessor::fullmatch

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::fullmatch(std::string_view pattern) const
```

## What It Does

Tests whether every bound key's text matches `pattern` in its **entirety**
(`std::regex_match` semantics), replacing each `Note`-kind key with a `Bool`
`Series`-kind key of results. Equivalent to
[`matches(pattern, true)`](matches.md), spelled without the extra boolean argument.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**pattern** (`std::string_view`)
    A regular expression, always — never literal text. The pattern must match the
    entire value; a partial match anywhere in the middle is not enough.

## Returns

`Catalog`
    A new catalog with every bound key replaced by a `Bool` `Series` of per-value
    full-match results.

## Examples

Example 1. Only the value that matches the pattern end-to-end returns `true`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Kumasi", "Tema"})),
});
auto out = ct.nt().fullmatch("Accra");
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
city                     | Series       | Series[bool]     | 3       | [true, false, false]                             
========================================================================================================================
```

## Notes

- See the important callout above.
- Always anchors the pattern to both ends of the text (`std::regex_match` semantics),
  so a partial match anywhere in the middle is not enough.
- Changes each bound key's kind from `Note` to `Series` (`Bool`).
- There's no `TextOptions` overload here — no `case_sensitive` or `limit` control; use
  inline regex flags (e.g. `(?i)`) in `pattern` if case-insensitivity is needed.

## See Also

- [`CatalogNoteAccessor::matches`](matches.md): the more general form — `full = false`
  (the default there) only requires the pattern to appear somewhere in the value.
- [`CatalogNoteAccessor::has`](has.md): a related pattern test with an optional literal
  mode, absent here.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::fullmatch`](../registry-note/fullmatch.md): the scribble-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="findall.html">
    <span class="label">Previous</span>
    <span class="title">findall</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
