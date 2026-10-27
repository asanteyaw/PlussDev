<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 650d2ea2d86a -->

# AxelNoteAccessor::count

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signatures

```cpp
Axel AxelNoteAccessor::count(std::string_view pattern) const

Axel AxelNoteAccessor::count(std::string_view pattern,
                             const pluss::table::TextOptions& options) const
```

## What It Does

Counts non-overlapping occurrences of `pattern` in each bound field's
text, in every registry frame, replacing each `Note` field with an
`Int64` `Series` field of counts. Applies
[`RegistryNoteAccessor::count`](../registry-note/count.md) to every
frame via [`Axel::map`](../axel/map.md).

```{danger} The single-argument overload defaults to REGEX mode, not literal
`count(pattern)` with no `options` is **not** a literal-text search — it
internally behaves as though called with `{.regex = true}`. Any regex
metacharacter in `pattern` (`.`, `*`, `+`, `?`, `(`, `)`, `|`, `[`, `]`,
and so on) is interpreted as a pattern instruction, not a literal
character. `count(".")` on `"a.b.c"` counts *every character* (5), not
the two literal dots — see Example 1. This is the opposite default from
[`has()`](has.md)'s single-argument overload, which is literal by
default. Pass `TextOptions{.regex = false}` explicitly whenever the
search text might contain characters regex would interpret specially.
```

## Parameters

**pattern** (`std::string_view`)
    The text to count — a regular expression under the single-argument
    overload (see the danger callout above), or literal/regex text
    under the two-argument overload depending on `options.regex`.

**options** (`pluss::table::TextOptions`, two-argument overload only)
    `options.regex` (default `false`): treat `pattern` as regex instead
    of literal text. `options.case_sensitive` (default `true`): ignore
    case when `false`. `options.limit` (default `-1`, unlimited): stop
    counting once this many matches have been found per value.

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by an
    `Int64` `Series` field of per-value match counts; every unbound
    field unchanged.

## Examples

Example 1. The headline gotcha: `count(".")` under the single-argument
overload counts every character, since `.` is the regex wildcard; the
explicit `{.regex = false}` form counts actual literal dots.

```cpp
auto r0 = pluss::scribble::Registry({{"txt", pluss::scribble::Slot({pluss::make_note("banana"), pluss::make_note("a.b.c")})}});
auto axel = pluss::scribble::Axel({r0});

auto regexCount = axel.fields({"txt"}).nt().count(".");
auto txt1 = regexCount.registry(0).fields("txt");
std::cout << "regex: " << std::get<pluss::Series>(txt1.at(0)).host<std::int64_t>()[0]
          << " " << std::get<pluss::Series>(txt1.at(1)).host<std::int64_t>()[0] << "\n";

pluss::table::TextOptions literal;
literal.regex = false;
auto literalCount = axel.fields({"txt"}).nt().count(".", literal);
auto txt2 = literalCount.registry(0).fields("txt");
std::cout << "literal: " << std::get<pluss::Series>(txt2.at(0)).host<std::int64_t>()[0]
          << " " << std::get<pluss::Series>(txt2.at(1)).host<std::int64_t>()[0];
```

### Output

```text
regex: 6 5
literal: 0 2
```

## Notes

- The single-argument overload defaults to `{.regex = true}` — see the
  danger callout above. This is the opposite of [`has()`](has.md)'s
  single-argument default.
- Matches are always non-overlapping: after a match, scanning resumes
  immediately past it.
- Changes each bound field's kind from `Note` to `Series` (`Int64`).
- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.

## See Also

- [`AxelNoteAccessor::has`](has.md): a related pattern-search method
  whose single-argument default is literal — the mirror image of this
  gotcha.
- [`AxelNoteAccessor::matches`](matches.md) / [`fullmatch()`](fullmatch.md):
  boolean pattern tests, rather than a count.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

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
