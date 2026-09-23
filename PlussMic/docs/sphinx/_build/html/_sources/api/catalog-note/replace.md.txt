<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 927f0c0b7f2f -->

# CatalogNoteAccessor::replace

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signatures

```cpp
Catalog CatalogNoteAccessor::replace(std::string_view from, std::string_view to) const

Catalog CatalogNoteAccessor::replace(
    std::string_view from, std::string_view to, const pluss::table::TextOptions& options) const
```

## What It Does

Replaces every occurrence of `from` with `to` in every bound key's text, returning a
new catalog with those keys replaced. The two-argument overload treats `from` as
literal text; the three-argument overload lets `options.regex` switch to a regex
replacement.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**from** (`std::string_view`)
    The text to replace — literal under the two-argument overload, or literal/regex
    under the three-argument overload depending on `options.regex`.

**to** (`std::string_view`)
    The replacement text.

**options** (`pluss::table::TextOptions`, three-argument overload only)
    `options.regex` (default `false`): treat `from` as a regex pattern.
    `options.case_sensitive` (default `true`): ignore case when `false`.
    `options.limit` (default `-1`, unlimited): cap the number of replacements per
    value.

## Returns

`Catalog`
    A new catalog with every bound key's text replaced.

## Examples

Example 1. Two-argument overload: every literal `"a"` becomes `"X"`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Kumasi"})),
});
auto out = ct.nt().replace("a", "X");
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
city                     | Note         | Note             | 2       | [AccrX, KumXsi]                                  
========================================================================================================================
```

`"Accra"`'s capital `"A"` is untouched (case-sensitive by default) — only the lowercase
`"a"` at the end is replaced. `"Kumasi"`'s `"a"` becomes `"X"` too.

## Notes

- See the important callout above.
- The two-argument overload's `from` is always literal — matching
  [`has()`](has.md)'s two-argument-overload default, not [`count()`](count.md)'s
  single-argument-overload default.
- Case-sensitive by default; pass `TextOptions{.case_sensitive = false}` via the
  three-argument overload for case-insensitive replacement.

## See Also

- [`CatalogNoteAccessor::count`](count.md) / [`has()`](has.md): related pattern
  operations sharing the same `TextOptions` type.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::replace`](../registry-note/replace.md): the scribble-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="partition.html">
    <span class="label">Previous</span>
    <span class="title">partition</span>
  </a>
  <a class="next-link" href="rstrip.html">
    <span class="label">Next</span>
    <span class="title">rstrip</span>
  </a>
</div>
