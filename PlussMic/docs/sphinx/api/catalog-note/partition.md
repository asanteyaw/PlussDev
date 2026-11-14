<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1a85fcbde1b8 -->

# CatalogNoteAccessor::partition

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
std::unordered_map<std::string, pluss::table::DataFrame>
std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::partition(std::string_view separator, std::string prefix = "part") const
```

## What It Does

Splits every bound key's text at the **first** occurrence of `separator`, returning
**one `pluss::table::DataFrame` per bound key** with exactly 3 columns — the text
before the separator, the separator itself, and the text after — unlike
[`split()`](split.md), which drops the separator and can produce more than 2 segments.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

```{important} Returns one DataFrame per key, not a Catalog
Same escape from the kindle model as findall()/split() — see
CatalogNoteAccessor::findall's own important callout.
```

## Parameters

**separator** (`std::string_view`)
    Literal text to partition on — only the *first* occurrence is used.

**prefix** (`std::string`, default `"part"`)
    Column-name prefix for the resulting `DataFrame`'s 3 columns.

## Returns

`std::unordered_map<std::string, pluss::table::DataFrame>`
    One entry per bound key, keyed by key name. Each `DataFrame` has exactly 3
    columns: `{prefix}_0` (before), `{prefix}_1` (the separator), `{prefix}_2` (after).

## Examples

Example 1. Partitioning `"a@x.com"` on `"@"` produces 3 columns — unlike
[`split("@")`](split.md)'s 2-column result, the separator itself survives as
`part_1`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "email"_k = pluss::kindle::Stream(pluss::make_note({"a@x.com", "b@y.com"})),
});
auto result = ct.nt().partition("@");
for (const auto& [key, frame] : result) {
  std::cout << "key=" << key << "\n" << frame;
}
```

### Output

```text
key=email
=============================
     | part_0  part_1  part_2
-----------------------------
0    | a       @       x.com 
1    | b       @       y.com 
-----------------------------
[2 cs x 3 feat]
```

## Notes

- See the important callouts above.
- Only the *first* occurrence of `separator` is used, even if it appears multiple
  times in a value — the remainder (everything after the first match) lands entirely
  in the third column.
- Always exactly 3 columns, regardless of input — contrast with
  [`split()`](split.md)'s variable column count.

## See Also

- [`CatalogNoteAccessor::split`](split.md): splits on every occurrence and drops the
  separator, rather than keeping it in a fixed 3-column layout.
- [`CatalogNoteAccessor::findall`](findall.md): extracts pattern matches instead of
  splitting around a literal separator.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pad.html">
    <span class="label">Previous</span>
    <span class="title">pad</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>
