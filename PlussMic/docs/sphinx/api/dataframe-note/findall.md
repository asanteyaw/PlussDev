<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d7205b08764f -->

# DataFrameNoteAccessor::findall

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signatures

```cpp
DataFrameNoteAccessor::findall(std::string_view pattern) const -> DataFrame

DataFrameNoteAccessor::findall(std::string_view pattern,
                                const TextOptions& options) const -> DataFrame
```

## What It Does

Finds every match of `pattern` in each cell of the **single selected text
feature** and expands the result into one output column per match position
— column `match_0` holds each row's first match, `match_1` its second, and
so on. Rows with fewer matches than another row simply have `NULL` in the
columns beyond their own match count; the number of output columns is
however many matches the *most-matching* row produced.

```{important} findall() requires exactly one selected feature
Unlike most methods on this accessor, `findall()` (like [`extract()`](extract.md),
[`split()`](split.md), [`partition()`](partition.md), and [`indicate()`](indicate.md))
operates on exactly one column, not "every feature the accessor was built
with." Calling it through an accessor built from more than one feature — e.g.
`df.features({"a", "b"}).nt().findall(...)` — throws
`"DataFrame::nt().findall requires one feature"`. Build the accessor with
a single feature name for these methods:
`df.features({"city"}).nt().findall(...)`.
```

```{danger} The single-argument overload defaults to regex mode, same gotcha as count()
Exactly like [`count()`](count.md), the single-argument `findall(pattern)`
defaults internally to `{.regex = true}` — a literal `"."` is interpreted
as "match any character," not a literal period. See Example 1, where the
default call splits every cell into one match per *character*.
```

The output DataFrame keeps the original frame's row labels and title —
only the single selected column is replaced, by the expanded `match_N`
columns.

## Parameters

**pattern** (`std::string_view`)
    A regular expression pattern under the single-argument overload (always
    regex mode — see the danger box above), or literal/regex text under the
    two-argument overload depending on `options.regex`.

**options** (`TextOptions`, two-argument overload only)
    `options.regex` (default `false`): treat `pattern` as regex instead of
    literal text.
    `options.case_sensitive` (default `true`): case-insensitive matching
    when `false`.
    `options.limit` (default `-1`): cap the number of matches collected per
    row (and therefore an upper bound on the number of output columns).
    `options.prefix` is **not** used by `findall()` — output columns are
    always named `match_0`, `match_1`, and so on, regardless of what
    `options.prefix` is set to. (Contrast [`split()`](split.md) and
    [`indicate()`](indicate.md), which do honor `options.prefix`.)

## Returns

`DataFrame`
    A new frame with the selected column replaced by `match_0`, `match_1`,
    ... columns (as many as the row with the most matches needed), keeping
    every other feature and the original row labels untouched. A row with
    fewer matches than the column count has `NULL` in the extra columns.

## Examples

Example 1. The regex-by-default gotcha, and the column-expansion mechanic
at the same time: `findall(".")` under the default overload splits each
cell into one match per character.

```cpp
using namespace pluss::table;

auto samples = DataFrame({
    {"text", Slate(pluss::make_note({"hi", "a.b", "xyz", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << samples;

auto out1 = samples.features({"text"}).nt().findall(".");
std::cout << out1;

auto out2 = samples.features({"text"}).nt().findall(".", {.regex = false});
std::cout << out2;
```

### Output (before)

```text
===========
     | text
-----------
0    | hi  
1    | a.b 
2    | xyz 
3    | NULL
-----------
[4 cs x 1 feat]
```

### Output (`findall(".")`, regex mode — every character is a match)

```text
================================
     | match_0  match_1  match_2
--------------------------------
0    | h        i        NULL   
1    | a        .        b      
2    | x        y        z      
3    | NULL     NULL     NULL   
--------------------------------
[4 cs x 3 feat]
```

### Output (`findall(".", {.regex = false})`, literal mode — actual dots only)

```text
==============
     | match_0
--------------
0    | NULL   
1    | .      
2    | NULL   
3    | NULL   
--------------
[4 cs x 1 feat]
```

Under regex mode, `.` matches any character, so `"hi"` produces two matches
(`"h"`, `"i"`) and `"a.b"`/`"xyz"` each produce three — the widest row
(three matches) sets the output at three `match_N` columns, and `"hi"`'s
missing third match shows as `NULL`. Under literal mode, only actual `.`
characters count: just one, in `"a.b"`; every other row (including its own
`.`-free characters) contributes zero matches, so the whole result
collapses to a single `match_0` column that's `NULL` everywhere except that
one row.

Example 2. A realistic case: extract every run of digits from a cell,
producing a ragged, NULL-padded result.

```cpp
auto orders = DataFrame({
    {"text", Slate(pluss::make_note(
        {"1 2 3 4", "10 20", "no digits", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << orders;

auto out3 = orders.features({"text"}).nt().findall(R"(\d+)", {.regex = true});
std::cout << out3;
```

### Output (before)

```text
================
     | text     
----------------
0    | 1 2 3 4  
1    | 10 20    
2    | no digits
3    | NULL     
----------------
[4 cs x 1 feat]
```

### Output

```text
=========================================
     | match_0  match_1  match_2  match_3
-----------------------------------------
0    | 1        2        3        4      
1    | 10       20       NULL     NULL   
2    | NULL     NULL     NULL     NULL   
3    | NULL     NULL     NULL     NULL   
-----------------------------------------
[4 cs x 4 feat]
```

Row 0 has four number matches, so it drives the output to four columns.
Row 1 (two matches) has `NULL` in `match_2`/`match_3`. Row 2 (no matches at
all) and row 3 (invalid) are `NULL` across every column.

Example 3. `options.limit` caps matches per row — and since the widest row
determines the column count, it also caps how many `match_N` columns
appear at all.

```cpp
auto out4 = orders.features({"text"}).nt().findall(
    R"(\d+)", {.regex = true, .limit = 2});
std::cout << out4;
```

### Output

```text
=======================
     | match_0  match_1
-----------------------
0    | 1        2      
1    | 10       20     
2    | NULL     NULL   
3    | NULL     NULL   
-----------------------
[4 cs x 2 feat]
```

With `limit = 2`, row 0 keeps only its first two matches (`"1"`, `"2"`) even
though it has four in the raw text — the result is capped at two columns
total instead of the four Example 2 produced.

## Notes

- Requires the accessor to be built from exactly **one** feature — see the
  important callout above.
- The single-argument overload defaults to `{.regex = true}` — the same
  gotcha documented on [`count()`](count.md).
- Output columns are always named `match_0`, `match_1`, ... —
  `options.prefix` has no effect here (contrast [`split()`](split.md) /
  [`indicate()`](indicate.md), which do use it).
- The number of output columns equals the largest per-row match count
  (after any `limit` is applied); rows with fewer matches get `NULL` in the
  extra columns.
- An invalid input cell contributes zero matches (all its `match_N` cells
  are `NULL`), the same as a cell with genuinely no matches — there is no
  way to distinguish "invalid row" from "row that matched nothing" in the
  output alone.

## See Also

- [`DataFrameNoteAccessor::extract`](extract.md): capture-group-based
  extraction, for pulling apart structured substrings instead of collecting
  every raw match.
- [`DataFrameNoteAccessor::count`](count.md): just the match count, without
  the matched text itself, and without the single-feature restriction.
- [`DataFrameNoteAccessor::split`](split.md): delimiter-based cell
  splitting, another single-feature, column-expanding method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="extract.html">
    <span class="label">Previous</span>
    <span class="title">extract</span>
  </a>
  <a class="next-link" href="fullmatch.html">
    <span class="label">Next</span>
    <span class="title">fullmatch</span>
  </a>
</div>
