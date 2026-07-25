<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::ends

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::ends(std::string_view suffix) const -> DataFrame
```

## What It Does

Checks every selected text feature's cells for a literal, case-sensitive
suffix match, replacing each cell with a `Bool` — `true` where the cell's
text ends with `suffix`, `false` where it doesn't. This is the mirror image
of [`starts()`](starts.md): same single-overload, no-options shape, just
anchored at the end of the string instead of the beginning. As with every
boolean-producing method on this accessor, the feature's payload kind
changes from `Note` to `Series` (`Bool`).

`ends()` has exactly one overload — always literal, always case-sensitive,
no regex mode. For a case-insensitive or pattern-based suffix check, use
[`has()`](has.md) with a `TextOptions` and a trailing anchor (e.g.
`has("suffix$", {.regex = true})`), or normalize case first with
[`lower()`](lower.md)/[`casefold()`](casefold.md).

## Parameters

**suffix** (`std::string_view`)
    The literal text to check for at the end of each cell. An empty suffix
    matches every non-null cell (every string ends with the empty string).

## Returns

`DataFrame`
    A new frame with every selected feature replaced by a `Bool` Series:
    `true` where the cell ends with `suffix`, `false` where it doesn't.

## Examples

Example 1. Check a filenames feature for a literal, case-sensitive suffix.

```cpp
using namespace pluss::table;

auto files = DataFrame({
    {"filename", Thread(pluss::make_note(
        {"report.pdf", "notes.PDF", "summary.docx", "invoice.pdf"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << files;

auto out = files.note().ends(".pdf");
std::cout << out;
```

### Output (before)

```text
===================
     | filename    
-------------------
0    | report.pdf  
1    | notes.PDF   
2    | summary.docx
3    | NULL        
-------------------
[4 cs x 1 feat]
```

### Output (after)

```text
===============
     | filename
---------------
0    | true    
1    | false   
2    | false   
3    | NULL    
---------------
[4 cs x 1 feat]
```

Row 0 (`"report.pdf"`) ends with `".pdf"`, so it's `true`. Row 1
(`"notes.PDF"`) doesn't — the uppercase `".PDF"` extension doesn't match
lowercase `".pdf"` under a case-sensitive comparison. Row 2 doesn't end with
`.pdf` at all. Row 3 was constructed invalid; it stays `NULL`.

Example 2. Case-insensitive suffix matching by normalizing first: chain
[`lower()`](lower.md) ahead of `ends()` to catch both `".pdf"` and `".PDF"`.

```cpp
auto out2 = files.note().lower().note().ends(".pdf");
std::cout << out2;
```

### Output

```text
===============
     | filename
---------------
0    | true    
1    | true    
2    | false   
3    | NULL    
---------------
[4 cs x 1 feat]
```

Lowercasing first folds `"notes.PDF"` down to `"notes.pdf"`, so row 1 now
matches too. `ends()` itself never gained a case-insensitivity option — the
normalization happens entirely in the `lower()` step ahead of it.

Example 3. An empty suffix matches every valid cell, regardless of content —
symmetric with `starts("")`.

```cpp
auto trivial = files.note().ends("");
std::cout << trivial;
```

### Output

```text
===============
     | filename
---------------
0    | true    
1    | true    
2    | true    
3    | NULL    
---------------
[4 cs x 1 feat]
```

## Notes

- Case-sensitive, literal comparison only — no regex mode, no
  case-insensitivity toggle. See Example 2 for the normalize-then-check
  pattern that works around this.
- Changes the feature's payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.
- Pairs naturally with `DataFrame::filter()` (via `.features(name)` to pull
  the single-column `Thread` result out) to keep only matching rows — see
  [`starts()`](starts.md)'s Example 2 for the pattern.

## See Also

- [`DataFrameNoteAccessor::starts`](starts.md): the same check anchored at
  the start of the cell instead of the end.
- [`DataFrameNoteAccessor::has`](has.md): substring search anywhere in the
  cell, with optional regex and case-insensitive modes.
- [`DataFrameNoteAccessor::lower`](lower.md) / [`casefold()`](casefold.md):
  normalize case ahead of a case-insensitive suffix check, as in Example 2.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="encode.html">
    <span class="label">Previous</span>
    <span class="title">encode</span>
  </a>
  <a class="next-link" href="extract.html">
    <span class="label">Next</span>
    <span class="title">extract</span>
  </a>
</div>
