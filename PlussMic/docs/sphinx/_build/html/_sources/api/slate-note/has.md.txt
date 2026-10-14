<!-- pluss-hand-authored -->

# SlateNoteAccessor::has

[Back to SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference)

## Signature

```cpp
SlateNoteAccessor::has(std::string_view needle, const TextOptions& options = {}) const -> Slate
```

## What It Does

Checks each cell of the bound slate for the presence of `needle` anywhere in the
cell, replacing each cell with a `Bool`. It's the general substring/pattern search on
this accessor — where [`starts()`](starts.md) and [`ends()`](ends.md) only check the
two ends of the string, `has()` looks everywhere, and it can be switched between a
plain literal search and a regular-expression search via `options`.

`TextOptions` (namespace-scope in `Slate.h`, used unqualified) has four fields,
though only two matter to `has()`:

```cpp
struct TextOptions {
  bool regex{false};
  bool case_sensitive{true};
  std::int64_t limit{-1};      // unused by has()
  std::string prefix{"part"};  // unused by has()
};
```

That default (`{}`) makes a bare `has(needle)` call a **literal, case-sensitive
substring search** — `needle` is matched character-for-character, not compiled as a
pattern.

```{warning} Different from count()/findall()'s single-argument default
`has(needle)` with no `options` searches *literally*. This is easy to confuse with
[`count()`](count.md) and [`findall()`](findall.md), whose single-argument overloads
default internally to `{.regex = true}` instead — so `has(pattern)` and
`count(pattern)` treat the exact same string differently unless you pass `options`
explicitly. If a pattern with regex metacharacters (`.`, `*`, `+`, `(`, `|`, and so
on) doesn't seem to be matching under `has()`, this default is the first thing to
check — pass `{.regex = true}` explicitly to search by pattern instead of literal
text.
```

With `options.regex = true`, `needle` is compiled as an ECMAScript regular
expression and each cell is tested with `regex_search` — a *contains* semantics,
matching anywhere in the cell, not anchored to the whole string (contrast
[`fullmatch()`](fullmatch.md), which requires the whole cell to match).

`case_sensitive = false` works in both modes: for literal search it ASCII-folds both
the needle and the haystack before comparing; for regex search it compiles the
pattern with the `std::regex::icase` flag.

## Parameters

**needle** (`std::string_view`)

The text to search for — a literal substring by default, or a regular expression
pattern when `options.regex` is `true`.

**options** (`TextOptions`, default `{}`)

`options.regex` (default `false`): treat `needle` as a regex pattern instead of
literal text. `options.case_sensitive` (default `true`): when `false`, the search
ignores case.

## Returns

`Slate`
    A new `Bool`-typed `Series` slate, same length as the bound slate: `true`
    where the cell contains a match, `false` where it doesn't.

## Examples

Example 1. Default call: literal, case-sensitive substring search.

```cpp
using namespace pluss::table;

auto logs = Slate(pluss::make_note(
    {"ERROR: disk full", "warning: low memory", "INFO: restart ok", "x"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << logs;

auto out1 = logs.nt().has("ERROR");
std::cout << out1;
```

### Output (before)

```text
0  ERROR: disk full
1  warning: low memory
2  INFO: restart ok
3  NULL

ptype: Note
```

### Output (after)

```text
0  true
1  false
2  false
3  NULL

ptype: Series
```

Only row 0 contains the exact literal text `"ERROR"`. Row 3 was constructed invalid
and stays `NULL` regardless of what text happens to sit underneath it.

Example 2. The `case_sensitive` toggle.

```cpp
auto out2 = logs.nt().has("error");
auto out3 = logs.nt().has("error", {.case_sensitive = false});
std::cout << out2;
std::cout << out3;
```

### Output (default, case-sensitive)

```text
0  false
1  false
2  false
3  NULL

ptype: Series
```

### Output (`case_sensitive = false`)

```text
0  true
1  false
2  false
3  NULL

ptype: Series
```

Example 3. Regex mode: search by pattern instead of literal text.

```cpp
auto out4 = logs.nt().has(R"(\d+)", {.regex = true});
std::cout << out4;
```

### Output

```text
0  false
1  false
2  false
3  NULL

ptype: Series
```

None of the sample rows contain a digit, so every valid row is `false` — included
to show the call shape.

## Notes

- Default `TextOptions{}` means literal, case-sensitive — unlike
  [`count()`](count.md) and [`findall()`](findall.md), whose single-argument
  overloads default to regex mode. See the warning above.
- `regex = true` uses *contains* semantics — for a whole-cell match use
  [`fullmatch()`](fullmatch.md), and for a start-anchored pattern use
  [`matches()`](matches.md).
- An invalid pattern under `options.regex = true` throws at the point `has()` is
  called.
- Changes the payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.
- Requires the bound slate to be `Note`-kind, or the call throws.

## See Also

- [`SlateNoteAccessor::starts`](starts.md) / [`ends()`](ends.md): narrower,
  options-free checks anchored to one end of the cell.
- [`SlateNoteAccessor::matches`](matches.md) / [`fullmatch()`](fullmatch.md):
  regex-only checks anchored to the start, or to the whole cell.
- [`SlateNoteAccessor::count`](count.md) / [`findall()`](findall.md): related
  pattern-search methods whose single-argument default is the *opposite* of
  `has()`'s.
- [`DataFrameNoteAccessor::has`](../dataframe-note/has.md): the multi-feature
  counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fullmatch.html">
    <span class="label">Previous</span>
    <span class="title">fullmatch</span>
  </a>
  <a class="next-link" href="indicate.html">
    <span class="label">Next</span>
    <span class="title">indicate</span>
  </a>
</div>
