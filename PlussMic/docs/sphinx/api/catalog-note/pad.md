<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: df45391a6232 -->

# CatalogNoteAccessor::pad

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::pad(
    std::int64_t width,
    pluss::table::TextAlign align = pluss::table::TextAlign::Left,
    char fill = ' ') const
```

## What It Does

Pads every bound key's text up to `width` characters with `fill`, aligning the
original text according to `align`, and returns a new catalog with those keys
replaced. Values already at or beyond `width` are left unchanged.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**width** (`std::int64_t`)
    The target character width. Values already at least this long are left unchanged.

**align** (`pluss::table::TextAlign`, defaults to `Left`)
    Where the original text lands within the padded field: `Left` (text at the start,
    fill on the right — the default), `Right` (fill on the left), or `Center`.

**fill** (`char`, defaults to `' '`)
    The character used to pad.

## Examples

Example 1. Default `align = Left`: shorter values get right-padded with spaces up to
`width`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "code"_k = pluss::kindle::Stream(pluss::make_note({"1", "22"})),
});
auto out = ct.nt().pad(4);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [code]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
code                     | Note         | Note             | 2       | [1   , 22  ]                                     
========================================================================================================================
```

## Returns

`Catalog`
    A new catalog with every bound key's text padded to `width`.

## Notes

- See the important callout above.
- Values already at or beyond `width` characters are returned unchanged — `pad()`
  never truncates.
- `fill` defaults to a space; pass any other single character (e.g. `'0'`) for
  zero-padding-style output.

## See Also

- [`CatalogNoteAccessor::chars`](chars.md): extracts a character range instead of
  padding.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::pad`](../registry-note/pad.md): the scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize.html">
    <span class="label">Previous</span>
    <span class="title">normalize</span>
  </a>
  <a class="next-link" href="partition.html">
    <span class="label">Next</span>
    <span class="title">partition</span>
  </a>
</div>
