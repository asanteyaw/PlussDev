<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ded67a67fccc -->

# CatalogNoteAccessor::findall

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signatures

```cpp
std::unordered_map<std::string, pluss::table::DataFrame>
std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::findall(std::string_view pattern) const

std::unordered_map<std::string, pluss::table::DataFrame>
std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::findall(
    std::string_view pattern, const pluss::table::TextOptions& options) const
```

## What It Does

Extracts every non-overlapping regex match from each bound key's text, returning **one
`pluss::table::DataFrame` per bound key** (not a `Catalog`) — unlike every other method
on this accessor, `findall()` genuinely leaves the kindle model. Each returned
`DataFrame` has one `match_N` column per match position (as many columns as the
value with the most matches needs), one row per original catalog row, with shorter
values' extra columns left empty.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

```{important} Returns one DataFrame per key, not a Catalog
findall() (along with split() and partition()) breaks the "every method returns a
Catalog" pattern the rest of this accessor follows -- the match structure genuinely
doesn't fit back into one Note-kind stream per key, so it comes back as a
std::unordered_map<std::string, pluss::table::DataFrame>, one map entry per bound key
name.
```

## Parameters

**pattern** (`std::string_view`)
    A regular expression — matches are extracted, not just tested for.

**options** (`pluss::table::TextOptions`, two-argument overload only)
    `options.case_sensitive` (default `true`) is the field that matters here;
    `options.regex` is implicitly `true` for this method regardless of what's passed.

## Returns

`std::unordered_map<std::string, pluss::table::DataFrame>`
    One entry per bound key, keyed by key name. Each `DataFrame` has one `match_N`
    column per match position and one row per original value.

## Examples

Example 1. Extracting lowercase-letter runs from an email-like string produces three
match columns (the local part, the domain name, and the TLD, each being a separate
run of lowercase letters).

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "email"_k = pluss::kindle::Stream(pluss::make_note({"a@x.com", "b@y.com"})),
});
auto result = ct.nt().findall("[a-z]+");
for (const auto& [key, frame] : result) {
  std::cout << "key=" << key << "\n" << frame;
}
```

### Output

```text
key=email
================================
     | match_0  match_1  match_2
--------------------------------
0    | a        x        com    
1    | b        y        com    
--------------------------------
[2 cs x 3 feat]
```

## Notes

- See the important callouts above.
- Column count is driven by the value with the most matches — shorter matches leave
  trailing columns empty for that row.
- Matches are always non-overlapping, same as [`count()`](count.md).

## See Also

- [`CatalogNoteAccessor::split`](split.md): breaks text apart at delimiter matches,
  rather than extracting the matches themselves.
- [`CatalogNoteAccessor::partition`](partition.md): a fixed 3-column split that also
  keeps the separator.
- [`CatalogNoteAccessor::count`](count.md): counts matches instead of extracting them.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ends.html">
    <span class="label">Previous</span>
    <span class="title">ends</span>
  </a>
  <a class="next-link" href="fullmatch.html">
    <span class="label">Next</span>
    <span class="title">fullmatch</span>
  </a>
</div>
