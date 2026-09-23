<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fea2beaf18cd -->

# SlotNoteAccessor::count

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signatures

```cpp
Slot SlotNoteAccessor::count(std::string_view pattern) const

Slot SlotNoteAccessor::count(std::string_view pattern,
                             const pluss::table::TextOptions& options) const
```

## What It Does

Counts non-overlapping occurrences of `pattern` in each value of the
bound slot, replacing each `Note` value with an `Int64` `Series`.

```{danger} The single-argument overload defaults to REGEX mode, not literal
`count(pattern)` with no `options` behaves as though called with
`{.regex = true}`. Any regex metacharacter in `pattern` (`.`, `*`, `+`,
`?`, `(`, `)`, `|`, `[`, `]`, and so on) is interpreted as a pattern
instruction, not a literal character. `count(".")` on `"a.b.c"` counts
*every character* (5), not the two literal dots — see Example 1. This is
the opposite default from [`has()`](has.md)'s single-argument overload,
which is literal by default. Pass `TextOptions{.regex = false}` explicitly
whenever the search text might contain characters regex would interpret
specially.
```

## Parameters

**pattern** (`std::string_view`)
    The text to count — a regular expression under the single-argument
    overload (see the danger callout above), or literal/regex text under
    the two-argument overload depending on `options.regex`.

**options** (`pluss::table::TextOptions`, two-argument overload only)
    `options.regex` (default `false`): treat `pattern` as regex instead
    of literal text. `options.case_sensitive` (default `true`): ignore
    case when `false`. `options.limit` (default `-1`, unlimited): stop
    counting once this many matches have been found per value.

## Returns

`Slot`
    A new `Series[int64]` slot of per-value match counts.

## Examples

Example 1. The headline gotcha: `count(".")` under the single-argument
overload counts every character; the explicit `{.regex = false}` form
counts actual literal dots.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("banana"), pluss::make_note("a.b.c")});

auto regexCount = slot.nt().count(".");
std::cout << "regex: " << std::get<pluss::Series>(regexCount.at(0)).to_vec<std::int64_t>()[0]
          << " " << std::get<pluss::Series>(regexCount.at(1)).to_vec<std::int64_t>()[0] << "\n";

pluss::table::TextOptions literal;
literal.regex = false;
auto literalCount = slot.nt().count(".", literal);
std::cout << "literal: " << std::get<pluss::Series>(literalCount.at(0)).to_vec<std::int64_t>()[0]
          << " " << std::get<pluss::Series>(literalCount.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
regex: 6 5
literal: 0 2
```

Example 2. Case-insensitive counting of a literal pattern.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("banana"), pluss::make_note("Banana")});
pluss::table::TextOptions opts;
opts.case_sensitive = false;
auto out = slot.nt().count("AN", opts);
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
2 2
```

## Notes

- The single-argument overload defaults to `{.regex = true}` — see the
  danger callout above.
- Matches are always non-overlapping.
- Changes each value's kind from `Note` to `Series` (`Int64`).
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::has`](has.md): a related pattern-search method whose
  single-argument default is literal.
- [`SlotNoteAccessor::matches`](matches.md) / [`fullmatch()`](fullmatch.md):
  boolean pattern tests, rather than a count.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
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
