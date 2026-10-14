<!-- pluss-hand-authored -->

# SlateNoteAccessor::count

[Back to SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference)

## Signatures

```cpp
SlateNoteAccessor::count(std::string_view pattern) const -> Slate
SlateNoteAccessor::count(std::string_view pattern, const TextOptions& options) const -> Slate
```

## What It Does

Counts non-overlapping occurrences of `pattern` in each cell of the bound slate,
replacing each cell with an `Int64` — the payload kind changes from `Note` to
`Series`, same as [`length()`](length.md).

```{danger} The single-argument overload defaults to REGEX mode, not literal
This is the single easiest mistake to make on this accessor. `count(pattern)` with
no `options` is **not** the literal-text search you might expect from
[`has()`](has.md)'s single-argument default — internally it calls the two-argument
overload with `{.regex = true}`. That means any regex metacharacter in `pattern`
(`.`, `*`, `+`, `?`, `(`, `)`, `|`, `[`, `]`, and so on) is interpreted as a pattern
instruction, not a literal character. Counting literal periods, parentheses, or other
punctuation with the single-argument overload will silently produce the wrong number
— see Example 1, where `count(".")` counts *every character* in each cell, not the
count of literal dots. Pass `{.regex = false}` explicitly (or escape the pattern)
whenever the search text might contain characters regex would interpret specially.
```

The two-argument overload takes an explicit `TextOptions`, letting you choose
`regex` and `case_sensitive` independently, and cap the count with `limit` (default
`-1`, meaning unlimited):

```cpp
struct TextOptions {
  bool regex{false};
  bool case_sensitive{true};
  std::int64_t limit{-1};
  std::string prefix{"part"};  // unused by count()
};
```

Matches never overlap: once a match is found, scanning resumes right after it, so
`count("aa")` on `"aaaa"` reports `2`, not `3`.

## Parameters

**pattern** (`std::string_view`)

The text to count — a regular expression pattern under the single-argument overload
(see the warning above), or literal/regex text under the two-argument overload
depending on `options.regex`.

**options** (`TextOptions`, two-argument overload only)

`options.regex` (default `false`): treat `pattern` as a regex instead of literal
text. `options.case_sensitive` (default `true`): ASCII-fold both sides before
comparing (literal mode) or compile with `std::regex::icase` (regex mode) when
`false`. `options.limit` (default `-1`): stop counting once this many matches have
been found; `-1` means no cap.

## Returns

`Slate`
    A new `Series`-kind (`Int64`) slate, same length as the bound slate, holding
    each cell's match count.

## Examples

Example 1. The headline gotcha: `count(".")` under the single-argument overload
counts *every character*, because `.` is the regex wildcard — compare against the
same call with `{.regex = false}`, which counts actual literal dots.

```cpp
using namespace pluss::table;

auto notes = Slate(pluss::make_note({"banana", "a.b.c", "no dots", "x"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << notes;

auto out1 = notes.nt().count(".");
std::cout << out1;

auto out2 = notes.nt().count(".", {.regex = false});
std::cout << out2;
```

### Output (before)

```text
0  banana
1  a.b.c
2  no dots
3  NULL

ptype: Note
```

### Output (single-argument `count(".")`, regex mode)

```text
0  6
1  5
2  7
3  NULL

ptype: Series
```

### Output (`count(".", {.regex = false})`, literal mode)

```text
0  0
1  2
2  0
3  NULL

ptype: Series
```

Under regex mode, `.` matches any single character, so each row's count is just that
row's character count (`"banana"` is 6 characters, `"a.b.c"` is 5, `"no dots"` is 7).
Under literal mode, the count is the number of actual `.` characters: two in
`"a.b.c"`, zero elsewhere.

Example 2. `limit` caps the count — useful when you only care whether a cell has "at
least N" occurrences, not the exact total.

```cpp
auto repeats = Slate(pluss::make_note({"ababab", "aaaa", "xyz", "x"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
auto out3 = repeats.nt().count("a", {.regex = true, .limit = 2});
std::cout << out3;
```

### Output

```text
0  2
1  2
2  0
3  NULL

ptype: Series
```

`"ababab"` actually contains `"a"` three times and `"aaaa"` four times, but both are
reported as `2` — counting stops the moment the limit is reached, rather than
continuing and clamping the final number.

## Notes

- The single-argument overload defaults to `{.regex = true}` — see the danger
  callout above. This is the opposite default from [`has()`](has.md)'s
  single-argument overload, which defaults to literal mode.
- Matches are always non-overlapping: after a match, scanning resumes immediately
  past it.
- Changes the bound slate's payload kind from `Note` to `Series` (`Int64`).
- A null cell on input stays null on output.
- `options.prefix` is part of `TextOptions` but has no effect on `count()` — it's
  only meaningful to [`split()`](split.md) and [`indicate()`](indicate.md).
- Requires the bound slate to be `Note`-kind, or the call throws.

## See Also

- [`SlateNoteAccessor::has`](has.md): a related pattern-search method whose
  single-argument default is literal — the mirror image of `count()`'s gotcha.
- [`SlateNoteAccessor::findall`](findall.md): returns the actual matched
  substrings (one Slate per match position) instead of just a count.
- [`SlateNoteAccessor::length`](length.md): the other method on this accessor that
  turns text into an `Int64` Series.
- [`DataFrameNoteAccessor::count`](../dataframe-note/count.md): the multi-feature
  counterpart, sharing this exact regex-by-default gotcha.

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
