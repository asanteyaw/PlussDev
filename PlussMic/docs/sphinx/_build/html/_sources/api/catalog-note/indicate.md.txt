<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7c4a153e997b -->

# CatalogNoteAccessor::indicate

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
std::unordered_map<std::string, pluss::table::DataFrame>
CatalogNoteAccessor::indicate(
    std::string_view delimiter = "|", std::string prefix = "") const
```

## What It Does

Multi-label-binarizes every bound key's text: splits each value on `delimiter`, then
builds one `Bool` column per **distinct token seen anywhere in the key**, returning
**one `pluss::table::DataFrame` per bound key** — like [`findall()`](findall.md), this
method leaves the kindle model. Each column is named after the token itself (with
`prefix` prepended, if given), and each row's column is `true` if that row's value
contains that token.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

```{important} Returns one DataFrame per key, not a Catalog
Same escape from the kindle model as findall()/split()/partition() — see
CatalogNoteAccessor::findall's own important callout.
```

## Parameters

**delimiter** (`std::string_view`, defaults to `"|"`)
    The literal text used to split each value into tokens before binarizing.

**prefix** (`std::string`, default empty)
    Optional prefix prepended to each generated column's name.

## Returns

`std::unordered_map<std::string, pluss::table::DataFrame>`
    One entry per bound key, keyed by key name. Each `DataFrame` has one `Bool`
    column per distinct token seen across every row of that key.

## Examples

Example 1. `"a|b"` and `"c"`, split on the default `"|"`, produce three columns — one
per distinct token (`a`, `b`, `c`) — each `true` exactly where that row's value
contained the token.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "tags"_k = pluss::kindle::Stream(pluss::make_note({"a|b", "c"})),
});
auto result = ct.nt().indicate("|");
for (const auto& [key, frame] : result) {
  std::cout << "key=" << key << "\n" << frame;
}
```

### Output

```text
key=tags
==========================
     | a      b      c    
--------------------------
0    | true   true   false
1    | false  false  true 
--------------------------
[2 cs x 3 feat]
```

## Notes

- See the important callouts above.
- Column order follows the order distinct tokens are first encountered, not
  necessarily alphabetical — treat column presence/absence as the contract, not
  column position.
- The default `delimiter` is `"|"`, matching a common multi-value-tag convention;
  pass any other literal delimiter for differently-formatted tag text.

## See Also

- [`CatalogNoteAccessor::findall`](findall.md) / [`split()`](split.md): the other
  methods on this accessor that leave the kindle model for a `DataFrame` result.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="length.html">
    <span class="label">Next</span>
    <span class="title">length</span>
  </a>
</div>
