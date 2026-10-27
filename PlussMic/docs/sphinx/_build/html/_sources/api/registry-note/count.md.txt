<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 461c257e36f7 -->

# RegistryNoteAccessor::count

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signatures

```cpp
Registry RegistryNoteAccessor::count(std::string_view pattern) const

Registry RegistryNoteAccessor::count(std::string_view pattern,
                                     const pluss::table::TextOptions& options) const
```

## What It Does

Counts non-overlapping occurrences of `pattern` in each bound field's text,
replacing each `Note` field with an `Int64` `Series` field of counts.

```{danger} The single-argument overload defaults to REGEX mode, not literal
`count(pattern)` with no `options` is **not** a literal-text search — it
internally behaves as though called with `{.regex = true}`. Any regex
metacharacter in `pattern` (`.`, `*`, `+`, `?`, `(`, `)`, `|`, `[`, `]`, and so
on) is interpreted as a pattern instruction, not a literal character.
`count(".")` on `"a.b.c"` counts *every character* (5), not the two literal
dots — see Example 1. This is the opposite default from
[`has()`](has.md)'s single-argument overload, which is literal by default.
Pass `TextOptions{.regex = false}` explicitly whenever the search text might
contain characters regex would interpret specially.
```

## Parameters

**pattern** (`std::string_view`)
    The text to count — a regular expression under the single-argument
    overload (see the danger callout above), or literal/regex text under the
    two-argument overload depending on `options.regex`.

**options** (`pluss::table::TextOptions`, two-argument overload only)
    `options.regex` (default `false`): treat `pattern` as regex instead of
    literal text. `options.case_sensitive` (default `true`): ignore case when
    `false`. `options.limit` (default `-1`, unlimited): stop counting once
    this many matches have been found per value.

## Returns

`Registry`
    A new registry with every bound field replaced by an `Int64` `Series`
    field of per-value match counts; every unbound field unchanged.

## Examples

Example 1. The headline gotcha: `count(".")` under the single-argument
overload counts every character, since `.` is the regex wildcard; the
explicit `{.regex = false}` form counts actual literal dots.

```cpp
auto rg = pluss::scribble::Registry({
    {"txt", pluss::scribble::Slot({pluss::make_note("banana"), pluss::make_note("a.b.c")})},
});

auto regexCount = rg.fields({"txt"}).nt().count(".");
auto txt1 = regexCount.fields("txt");
std::cout << "regex: " << std::get<pluss::Series>(txt1.at(0)).host<std::int64_t>()[0]
          << " " << std::get<pluss::Series>(txt1.at(1)).host<std::int64_t>()[0] << "\n";

pluss::table::TextOptions literal;
literal.regex = false;
auto literalCount = rg.fields({"txt"}).nt().count(".", literal);
auto txt2 = literalCount.fields("txt");
std::cout << "literal: " << std::get<pluss::Series>(txt2.at(0)).host<std::int64_t>()[0]
          << " " << std::get<pluss::Series>(txt2.at(1)).host<std::int64_t>()[0];
```

### Output

```text
regex: 6 5
literal: 0 2
```

`"banana"` is 6 characters and `"a.b.c"` is 5 — under regex mode `.` matches
any single character, so the counts are just the character counts. Under
literal mode, `"banana"` has zero literal dots and `"a.b.c"` has two.

Example 2. Case-insensitive counting of a literal pattern.

```cpp
auto rg = pluss::scribble::Registry({
    {"txt", pluss::scribble::Slot({pluss::make_note("banana"), pluss::make_note("Banana")})},
});

pluss::table::TextOptions opts;
opts.case_sensitive = false;
auto out = rg.fields({"txt"}).nt().count("AN", opts);
auto txt = out.fields("txt");
std::cout << std::get<pluss::Series>(txt.at(0)).host<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(txt.at(1)).host<std::int64_t>()[0];
```

### Output

```text
2 2
```

`opts.regex` defaults to `false` when a `TextOptions` value is explicitly
passed and its fields aren't set — only the single-argument overload changes
this default, per the danger callout above.

## Notes

- The single-argument overload defaults to `{.regex = true}` — see the danger
  callout above. This is the opposite of [`has()`](has.md)'s single-argument
  default.
- Matches are always non-overlapping: after a match, scanning resumes
  immediately past it.
- Changes each bound field's kind from `Note` to `Series` (`Int64`).
- `options.prefix` is part of `TextOptions` but has no effect on `count()`.
- Requires every bound field to be `Note`-kind — a non-`Note` field throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`RegistryNoteAccessor::has`](has.md): a related pattern-search method whose
  single-argument default is literal — the mirror image of this gotcha.
- [`RegistryNoteAccessor::matches`](matches.md) / [`fullmatch()`](fullmatch.md):
  boolean pattern tests, rather than a count.
- [`RegistryNoteAccessor::length`](length.md): the other method on this
  accessor that turns text into an `Int64` `Series`.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.

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
