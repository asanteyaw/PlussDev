<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e343a05cfaa6 -->

# DataFrameNoteAccessor::fullmatch

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::fullmatch(std::string_view pattern) const -> DataFrame
```

## What It Does

Tests every selected text feature's cells against a regular expression,
requiring the pattern to match the **entire** cell (start to end), and
replaces each cell with a `Bool`. `fullmatch(pattern)` is not a separate
implementation — it's defined purely as `matches(pattern, true)`; see
[`matches()`](matches.md) for the full explanation of how this whole-cell
mode differs from `matches()`'s default start-anchored-only behavior.

```{note} Equivalent to matches(pattern, true)
`fullmatch(pattern)` and `matches(pattern, true)` are two ways to write
literally the same call. Reach for whichever reads more clearly at the call
site — `fullmatch()` states the "must match completely" intent more
directly, while `matches(pattern, true)` is convenient when the `full` flag
is itself a variable.
```

## Parameters

**pattern** (`std::string_view`)
    A regular expression pattern (ECMAScript syntax, via `std::regex`) that
    must match the cell's entire text to report `true`.

## Returns

`DataFrame`
    A new frame with every selected feature replaced by a `Bool` Series:
    `true` where the cell's entire text matches `pattern`, `false`
    otherwise.

## Examples

Example 1. A pattern that must match the whole cell, contrasted with a cell
that only matches a prefix.

```cpp
using namespace pluss::table;

auto codes = DataFrame({
    {"code", Thread(pluss::make_note(
        {"A123", "A123XYZ", "123A", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << codes;

auto out = codes.nt().fullmatch("A[0-9]+");
std::cout << out;
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

Row 0 (`"A123"`) is exactly and entirely `A[0-9]+`, so it's `true`. Row 1
(`"A123XYZ"`) matches the pattern only as a prefix — the trailing `"XYZ"`
means the pattern doesn't consume the whole cell, so `fullmatch()` reports
`false` here even though [`matches("A[0-9]+")`](matches.md) (without
`full = true`) would report `true` for this same cell. Row 2 doesn't match
at all. Row 3 stays `NULL`.

Example 2. A pattern that anchors itself with `^`/`$` behaves identically
under `fullmatch()`, since the whole-cell requirement is already implicit —
the explicit anchors are redundant but harmless.

```cpp
auto out2 = codes.nt().fullmatch("^A[0-9]+$");
std::cout << out2;
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

Same result as Example 1 — `fullmatch()` uses `std::regex_match` under the
hood, which already requires consuming the entire string, so manual `^`/`$`
anchors don't change anything.

## Notes

- Purely `matches(pattern, true)` under the hood — see
  [`matches()`](matches.md) for the shared mechanics (validity passthrough,
  regex compilation, error behavior).
- `^`/`$` anchors in the pattern are redundant (but harmless) here, since
  `fullmatch()` already requires the whole cell to match.
- Changes the feature's payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.
- An invalid regex pattern throws at call time (`"Thread::fullmatch:
  invalid regular expression: ..."`).

## See Also

- [`DataFrameNoteAccessor::matches`](matches.md): the more general method
  this one delegates to, including the default start-anchored-only mode.
- [`DataFrameNoteAccessor::has`](has.md): unanchored search — matches
  anywhere in the cell, not the whole thing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="findall.html">
    <span class="label">Previous</span>
    <span class="title">findall</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
