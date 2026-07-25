<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::matches

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::matches(std::string_view pattern, bool full = false) const -> DataFrame
```

## What It Does

Tests every selected text feature's cells against a regular expression,
replacing each cell with a `Bool`. The `full` flag switches between two
different anchoring behaviors:

- **`full = false`** (the default): the pattern is anchored to the **start**
  of the cell only — internally, `pattern` is wrapped as
  `"^(?:" + pattern + ")"` and searched with `regex_search`. A match at the
  beginning is enough; anything after the matched portion is ignored. This
  is *not* a full-string match, despite the method name — a pattern that
  matches only the first few characters of a longer cell still reports
  `true`.
- **`full = true`**: the pattern must match the **entire** cell,
  start to end (`regex_match` semantics) — equivalent to calling
  [`fullmatch(pattern)`](fullmatch.md) directly. In fact, `fullmatch()` is
  implemented as nothing more than `matches(pattern, true)`.

```{important} matches() without full=true is a start-anchored search, not a whole-cell match
The default call `matches(pattern)` only requires `pattern` to match
starting at position 0 — it does **not** require the match to consume the
whole cell. `matches("A[0-9]+")` against `"A123XYZ"` is `true`, because
`"A123"` matches starting at the beginning even though `"XYZ"` is left
over. If a whole-cell match is what's actually needed, pass `full = true`
or call [`fullmatch()`](fullmatch.md) — see Example 1 for the contrast.
```

## Parameters

**pattern** (`std::string_view`)
    A regular expression pattern (ECMAScript syntax, via `std::regex`).

**full** (`bool`, default `false`)
    `false`: match anchored at the start only. `true`: the pattern must
    match the entire cell — identical to [`fullmatch(pattern)`](fullmatch.md).

## Returns

`DataFrame`
    A new frame with every selected feature replaced by a `Bool` Series:
    `true` where the cell matches, `false` where it doesn't.

## Examples

Example 1. The core distinction: a pattern that matches a prefix reports
`true` under the default (`full = false`), but `false` once the whole cell
is required to match (`full = true`).

```cpp
using namespace pluss::table;

auto codes = DataFrame({
    {"code", Thread(pluss::make_note(
        {"A123", "A123XYZ", "123A", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << codes;

auto start_only = codes.note().matches("A[0-9]+");
std::cout << start_only;

auto whole_cell = codes.note().matches("A[0-9]+", true);
std::cout << whole_cell;
```

### Output (before)

```text
==============
     | code   
--------------
0    | A123   
1    | A123XYZ
2    | 123A   
3    | NULL   
--------------
[4 cs x 1 feat]
```

### Output (`full = false`, start-anchored)

```text
============
     | code 
------------
0    | true 
1    | true 
2    | false
3    | NULL 
------------
[4 cs x 1 feat]
```

### Output (`full = true`, whole-cell)

```text
============
     | code 
------------
0    | true 
1    | false
2    | false
3    | NULL 
------------
[4 cs x 1 feat]
```

Row 0 (`"A123"`) matches `A[0-9]+` exactly, so it's `true` either way. Row 1
(`"A123XYZ"`) matches the pattern at its start (`"A123"`) but has leftover
text after it — `true` under the lenient default, `false` once a whole-cell
match is required. Row 2 (`"123A"`) doesn't start with `A` at all, so it's
`false` under both. Row 3 is invalid and stays `NULL`.

Example 2. `full = true` and [`fullmatch()`](fullmatch.md) are the exact
same operation — this call is identical to `codes.note().fullmatch("A[0-9]+")`.

```cpp
auto same_as_fullmatch = codes.note().matches("A[0-9]+", true);
std::cout << same_as_fullmatch;
```

### Output

```text
============
     | code 
------------
0    | true 
1    | false
2    | false
3    | NULL 
------------
[4 cs x 1 feat]
```

Identical output to the "whole-cell" table in Example 1 — `matches(pattern,
true)` and `fullmatch(pattern)` are two spellings of the same call.

Example 3. An invalid regex pattern throws immediately, before any row is
evaluated.

```cpp
try {
  auto bad = codes.note().matches("[unclosed");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::matches: invalid regular expression: <implementation-defined regex error text>
```

The exact wording after the colon comes from the underlying `std::regex`
implementation and varies by standard library, but the `"Thread::matches:
invalid regular expression: "` prefix is always present.

## Notes

- `full = false` (the default) is a start-anchored *search*, not a
  whole-string match — see the important callout above.
- `full = true` is identical to calling [`fullmatch()`](fullmatch.md)
  directly; `fullmatch()` is implemented purely in terms of this call.
- Changes the feature's payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.
- An invalid regex pattern throws at call time, before any row is
  evaluated.

## See Also

- [`DataFrameNoteAccessor::fullmatch`](fullmatch.md): the dedicated
  whole-cell-match method, equivalent to `matches(pattern, true)`.
- [`DataFrameNoteAccessor::has`](has.md): unanchored search — matches
  anywhere in the cell, not just at the start.
- [`DataFrameNoteAccessor::findall`](findall.md) / [`extract()`](extract.md):
  return the matched text itself, instead of just a boolean.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lstrip.html">
    <span class="label">Previous</span>
    <span class="title">lstrip</span>
  </a>
  <a class="next-link" href="normalize.html">
    <span class="label">Next</span>
    <span class="title">normalize</span>
  </a>
</div>
