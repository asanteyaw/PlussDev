<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b1bd199b8ce7 -->

# CatalogNoteAccessor::count

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signatures

```cpp
Catalog CatalogNoteAccessor::count(std::string_view pattern) const

Catalog CatalogNoteAccessor::count(
    std::string_view pattern, const pluss::table::TextOptions& options) const
```

## What It Does

Counts non-overlapping occurrences of `pattern` in every bound key's text, replacing
each `Note`-kind key with an `Int64` `Series`-kind key of counts.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

```{danger} The single-argument overload defaults to REGEX mode, not literal
`count(pattern)` with no `options` is **not** a literal-text search — it internally
behaves as though called with `{.regex = true}`. Any regex metacharacter in `pattern`
(`.`, `*`, `+`, `?`, `(`, `)`, `|`, `[`, `]`, and so on) is interpreted as a pattern
instruction, not a literal character. `count(".")` on `"a.b.c"` counts *every
character* (5), not the two literal dots — see Example 1. This is the opposite default
from [`has()`](has.md)'s single-argument overload, which is literal by default. Pass
`TextOptions{.regex = false}` explicitly whenever the search text might contain
characters regex would interpret specially.
```

## Parameters

**pattern** (`std::string_view`)
    The text to count — a regular expression under the single-argument overload (see
    the danger callout above), or literal/regex text under the two-argument overload
    depending on `options.regex`.

**options** (`pluss::table::TextOptions`, two-argument overload only)
    `options.regex` (default `false`): treat `pattern` as regex instead of literal
    text. `options.case_sensitive` (default `true`): ignore case when `false`.
    `options.limit` (default `-1`, unlimited): stop counting once this many matches
    have been found per value.

## Returns

`Catalog`
    A new catalog with every bound key replaced by an `Int64` `Series` of per-value
    match counts.

## Examples

Example 1. The headline gotcha: `count(".")` under the single-argument overload counts
every character, since `.` is the regex wildcard; the explicit `{.regex = false}` form
counts actual literal dots.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "txt"_k = pluss::kindle::Stream(pluss::make_note({"banana", "a.b.c"})),
});

auto regexCount = ct.nt().count(".");
std::cout << "regex: " << regexCount;

pluss::table::TextOptions literal;
literal.regex = false;
auto literalCount = ct.nt().count(".", literal);
std::cout << "literal: " << literalCount;
```

### Output

```text
regex: ========================================================================================================================
kindle::Catalog  keys=1
keys: [txt]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
txt                      | Series       | Series[int64]    | 2       | [6, 5]                                           
========================================================================================================================
literal: ========================================================================================================================
kindle::Catalog  keys=1
keys: [txt]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
txt                      | Series       | Series[int64]    | 2       | [0, 2]                                           
========================================================================================================================
```

`"banana"` is 6 characters and `"a.b.c"` is 5 — under regex mode `.` matches any single
character, so the counts are just the character counts. Under literal mode, `"banana"`
has zero literal dots and `"a.b.c"` has two.

Example 2. Case-insensitive counting of a literal pattern.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "txt"_k = pluss::kindle::Stream(pluss::make_note({"banana", "Banana"})),
});

pluss::table::TextOptions opts;
opts.case_sensitive = false;
auto out = ct.nt().count("AN", opts);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [txt]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
txt                      | Series       | Series[int64]    | 2       | [2, 2]                                           
========================================================================================================================
```

`opts.regex` defaults to `false` when a `TextOptions` value is explicitly passed and
its fields aren't set — only the single-argument overload changes this default, per
the danger callout above.

## Notes

- The single-argument overload defaults to `{.regex = true}` — see the danger callout
  above. This is the opposite of [`has()`](has.md)'s single-argument default.
- Matches are always non-overlapping: after a match, scanning resumes immediately past
  it.
- Changes each bound key's kind from `Note` to `Series` (`Int64`).
- `options.prefix` is part of `TextOptions` but has no effect on `count()`.

## See Also

- [`CatalogNoteAccessor::has`](has.md): a related pattern-search method whose
  single-argument default is literal — the mirror image of this gotcha.
- [`CatalogNoteAccessor::matches`](matches.md) / [`fullmatch()`](fullmatch.md):
  boolean pattern tests, rather than a count.
- [`CatalogNoteAccessor::length`](length.md): the other method on this accessor that
  turns text into an `Int64` `Series`.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::count`](../registry-note/count.md): the bundle-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="chars.html">
    <span class="label">Previous</span>
    <span class="title">chars</span>
  </a>
  <a class="next-link" href="encode.html">
    <span class="label">Next</span>
    <span class="title">encode</span>
  </a>
</div>
