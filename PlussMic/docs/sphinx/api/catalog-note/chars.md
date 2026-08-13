<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8b313ba01a64 -->

# CatalogNoteAccessor::chars

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::chars(
    std::int64_t start,
    std::optional<std::int64_t> stop = std::nullopt,
    std::int64_t step = 1) const
```

## What It Does

Extracts a character-index slice `[start, stop)` (with an optional `step`) from every
bound key's text, Python-`str[start:stop:step]`-style, and returns a new catalog with
those keys replaced.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**start** (`std::int64_t`)
    The character index to start the slice at (inclusive).

**stop** (`std::optional<std::int64_t>`, default empty)
    The character index to stop the slice before (exclusive). When empty, the slice
    runs to the end of the string.

**step** (`std::int64_t`, default `1`)
    The stride between characters kept in the slice.

## Returns

`Catalog`
    A new catalog with every bound key's text replaced by the extracted character
    slice.

## Examples

Example 1. `chars(0, 3)` extracts the first 3 characters of every value.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Kumasi"})),
});
auto out = ct.nt().chars(0, 3);
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
city                     | Note         | Note             | 2       | [Acc, Kum]                                       
========================================================================================================================
```

## Notes

- See the important callout above.
- Same slicing semantics as Python's `str[start:stop:step]` — `stop` omitted means
  "to the end", and a negative `step` is not supported (matches the `std::int64_t`
  parameter having no documented negative-stride behavior at this layer).

## See Also

- [`CatalogNoteAccessor::pad`](pad.md): the complementary operation — extends text
  rather than extracting a slice of it.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::chars`](../registry-note/chars.md): the bundle-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="casefold.html">
    <span class="label">Previous</span>
    <span class="title">casefold</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
