<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f5d131053ce4 -->

# CatalogNoteAccessor::split

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signatures

```cpp
std::unordered_map<std::string, pluss::table::DataFrame>
CatalogNoteAccessor::split(
    std::string_view delimiter, std::int64_t limit = -1, std::string prefix = "part") const

std::unordered_map<std::string, pluss::table::DataFrame>
CatalogNoteAccessor::split(
    std::string_view pattern, const pluss::table::TextOptions& options) const
```

## What It Does

Splits every bound key's text at each occurrence of `delimiter`, returning **one
`pluss::table::DataFrame` per bound key** — like [`findall()`](findall.md), this method
leaves the kindle model rather than returning a `Catalog`. Each returned `DataFrame`
has one `{prefix}_N` column per split segment (`part_0`, `part_1`, ... by default),
one row per original catalog row. The delimiter text itself is dropped from the
output — contrast with [`partition()`](partition.md), which keeps it.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

```{important} Returns one DataFrame per key, not a Catalog
Same escape from the kindle model as findall()/partition() — see
CatalogNoteAccessor::findall's own important callout.
```

## Parameters

**delimiter** (`std::string_view`, first overload)
    Literal text to split on (first overload) — not a regex pattern.

**limit** (`std::int64_t`, default `-1`, first overload)
    Maximum number of splits to perform per value; `-1` means unlimited.

**prefix** (`std::string`, default `"part"`, first overload)
    Column-name prefix for the resulting `DataFrame`'s segment columns.

**pattern** (`std::string_view`, second overload)
    A pattern to split on — literal or regex depending on `options.regex`.

**options** (`pluss::table::TextOptions`, second overload)
    Full text-matching options for the pattern-based split.

## Returns

`std::unordered_map<std::string, pluss::table::DataFrame>`
    One entry per bound key, keyed by key name. Each `DataFrame` has one
    `{prefix}_N` column per split segment.

## Examples

Example 1. Default `limit`/`prefix`: splitting `"a@x.com"` on `"@"` produces two
`part_N` columns — the delimiter itself does not appear in the output.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "email"_k = pluss::kindle::Stream(pluss::make_note({"a@x.com", "b@y.com"})),
});
auto result = ct.nt().split("@");
for (const auto& [key, frame] : result) {
  std::cout << "key=" << key << "\n" << frame;
}
```

### Output

```text
key=email
=====================
     | part_0  part_1
---------------------
0    | a       x.com 
1    | b       y.com 
---------------------
[2 cs x 2 feat]
```

Example 2. An explicit `prefix` renames the resulting columns; `limit = 1` caps the
split count (each value here only has one `"@"` anyway, so the output is unchanged —
`limit` only matters when a value has more delimiter occurrences than `limit` allows).

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "email"_k = pluss::kindle::Stream(pluss::make_note({"a@x.com", "b@y.com"})),
});
auto result = ct.nt().split("@", 1, "seg");
for (const auto& [key, frame] : result) {
  std::cout << "key=" << key << "\n" << frame;
}
```

### Output

```text
key=email
===================
     | seg_0  seg_1
-------------------
0    | a      x.com
1    | b      y.com
-------------------
[2 cs x 2 feat]
```

## Notes

- See the important callouts above.
- Column count is driven by the value with the most splits — values with fewer
  segments leave trailing columns empty for that row.
- The delimiter text is always dropped from the output — use
  [`partition()`](partition.md) if you need it retained.

## See Also

- [`CatalogNoteAccessor::partition`](partition.md): a fixed 3-column split that keeps
  the separator as its own column.
- [`CatalogNoteAccessor::findall`](findall.md): extracts pattern matches instead of the
  text between them.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rstrip.html">
    <span class="label">Previous</span>
    <span class="title">rstrip</span>
  </a>
  <a class="next-link" href="starts.html">
    <span class="label">Next</span>
    <span class="title">starts</span>
  </a>
</div>
