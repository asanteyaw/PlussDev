<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c3bcedc549ac -->

# DataFrameNoteAccessor::count

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signatures

```cpp
DataFrameNoteAccessor::count(std::string_view pattern) const -> DataFrame

DataFrameNoteAccessor::count(std::string_view pattern,
                              const TextOptions& options) const -> DataFrame
```

## What It Does

Counts non-overlapping occurrences of `pattern` in every selected text
feature's cells, replacing each cell with an `Int64` — the feature's
payload kind changes from `Note` to `Series`, same as [`length()`](length.md).

```{danger} The single-argument overload defaults to REGEX mode, not literal
This is the single easiest mistake to make on this accessor.
`count(pattern)` with no `options` is **not** the literal-text search you
might expect from [`has()`](has.md)'s single-argument default — internally
it calls the two-argument overload with `{.regex = true}`. That means any
regex metacharacter in `pattern` (`.`, `*`, `+`, `?`, `(`, `)`, `|`, `[`,
`]`, and so on) is interpreted as a pattern instruction, not a literal
character. Counting literal periods, parentheses, or other punctuation with
the single-argument overload will silently produce the wrong number — see
Example 1, where `count(".")` counts *every character* in each cell, not
the count of literal dots. Pass `{.regex = false}` explicitly (or escape
the pattern) whenever the search text might contain characters regex would
interpret specially.
```

The two-argument overload takes an explicit `TextOptions`, letting you
choose `regex` and `case_sensitive` independently, and cap the count with
`limit` (default `-1`, meaning unlimited):

```cpp
struct TextOptions {
  bool regex{false};
  bool case_sensitive{true};
  std::int64_t limit{-1};
  std::string prefix{"part"};  // unused by count()
};
```

Matches never overlap: once a match is found, scanning resumes right after
it, so `count("aa")` on `"aaaa"` reports `2`, not `3`.

## Parameters

**pattern** (`std::string_view`)
    The text to count — a regular expression pattern under the
    single-argument overload (see the warning above), or literal/regex text
    under the two-argument overload depending on `options.regex`.

**options** (`TextOptions`, two-argument overload only)
    `options.regex` (default `false`): treat `pattern` as a regex instead
    of literal text.
    `options.case_sensitive` (default `true`): ASCII-fold both sides before
    comparing (literal mode) or compile with `std::regex::icase` (regex
    mode) when `false`.
    `options.limit` (default `-1`): stop counting once this many matches
    have been found; `-1` means no cap.

## Returns

`DataFrame`
    A new frame with every selected feature replaced by an `Int64` Series
    of match counts.

## Examples

Example 1. The headline gotcha: `count(".")` under the single-argument
overload counts *every character*, because `.` is the regex wildcard —
compare against the same call with `{.regex = false}`, which counts actual
literal dots.

```cpp
using namespace pluss::table;

auto notes = DataFrame({
    {"text", Thread(pluss::make_note(
        {"banana", "a.b.c", "no dots", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << notes;

auto out1 = notes.nt().count(".");
std::cout << out1;

auto out2 = notes.nt().count(".", {.regex = false});
std::cout << out2;
```

### Output (before)

```text
==============
     | text   
--------------
0    | banana 
1    | a.b.c  
2    | no dots
3    | NULL   
--------------
[4 cs x 1 feat]
```

### Output (single-argument `count(".")`, regex mode)

```text
===========
     | text
-----------
0    | 6   
1    | 5   
2    | 7   
3    | NULL
-----------
[4 cs x 1 feat]
```

### Output (`count(".", {.regex = false})`, literal mode)

```text
===========
     | text
-----------
0    | 0   
1    | 2   
2    | 0   
3    | NULL
-----------
[4 cs x 1 feat]
```

Under regex mode, `.` matches any single character, so each row's count is
just that row's character count (`"banana"` is 6 characters, `"a.b.c"` is
5, `"no dots"` is 7). Under literal mode, the count is the number of actual
`.` characters: two in `"a.b.c"`, zero elsewhere. These are wildly
different numbers from the exact same `pattern` string — the only thing
that changed is whether `options.regex` was left at its (very different)
default.

Example 2. The `case_sensitive` toggle, in literal mode, with a needle
whose case doesn't match every row.

```cpp
auto mixed = DataFrame({
    {"text", Thread(pluss::make_note(
        {"banana", "BANANA", "no match", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});

auto out3 = mixed.nt().count("ANA", {.case_sensitive = true});
std::cout << out3;

auto out4 = mixed.nt().count("ANA", {.case_sensitive = false});
std::cout << out4;
```

### Output (`case_sensitive = true`)

```text
===========
     | text
-----------
0    | 0   
1    | 1   
2    | 0   
3    | NULL
-----------
[4 cs x 1 feat]
```

### Output (`case_sensitive = false`)

```text
===========
     | text
-----------
0    | 1   
1    | 1   
2    | 0   
3    | NULL
-----------
[4 cs x 1 feat]
```

Case-sensitively, `"ANA"` (uppercase) only occurs in `"BANANA"` (also
uppercase) — `"banana"` (lowercase) scores `0`. Folding case before
comparing makes `"banana"` match too, since `"ana"` occurs once in it
(`b-ANA-na`, non-overlapping).

Example 3. `limit` caps the count — useful when you only care whether a
cell has "at least N" occurrences, not the exact total.

```cpp
auto repeats = DataFrame({
    {"text", Thread(pluss::make_note(
        {"ababab", "aaaa", "xyz", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});

auto out5 = repeats.nt().count("a", {.regex = true, .limit = 2});
std::cout << out5;
```

### Output

```text
===========
     | text
-----------
0    | 2   
1    | 2   
2    | 0   
3    | NULL
-----------
[4 cs x 1 feat]
```

`"ababab"` actually contains `"a"` three times and `"aaaa"` four times, but
both are reported as `2` — counting stops the moment the limit is reached,
rather than continuing and clamping the final number.

## Notes

- The single-argument overload defaults to `{.regex = true}` — see the
  danger callout above. This is the opposite default from
  [`has()`](has.md)'s single-argument overload, which defaults to literal
  mode.
- Matches are always non-overlapping: after a match, scanning resumes
  immediately past it.
- Changes the feature's payload kind from `Note` to `Series` (`Int64`).
- A null cell on input stays null on output (evaluated internally as `0`
  but re-masked to `NULL` before returning).
- `options.prefix` is part of `TextOptions` but has no effect on `count()`
  — it's only meaningful to [`split()`](split.md) and [`indicate()`](indicate.md).

## See Also

- [`DataFrameNoteAccessor::has`](has.md): a related pattern-search method
  whose single-argument default is literal — the mirror image of `count()`'s
  gotcha.
- [`DataFrameNoteAccessor::findall`](findall.md): returns the actual matched
  substrings (one column per match) instead of just a count.
- [`DataFrameNoteAccessor::length`](length.md): the other method on this
  accessor that turns text into an `Int64` Series.

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
