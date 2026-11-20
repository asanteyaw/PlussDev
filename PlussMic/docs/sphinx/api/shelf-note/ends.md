<!-- pluss-hand-authored -->

# ShelfNoteAccessor::ends

[Back to ShelfNoteAccessor API Reference](../../shelf_note_accessor_api_reference)

## Signature

```cpp
Shelf ShelfNoteAccessor::ends(std::string_view suffix) const
```

## What It Does

Checks the bound shelf's cells for a literal, case-sensitive suffix match,
replacing each cell with a `Bool` — `true` where the cell's text ends with
`suffix`, `false` where it doesn't. A direct forward to
[`Shelf::ends()`](../shelf/ends.md); the mirror image of [`starts()`](starts.md).
Requires the bound shelf to be `Note`-kind.

## Parameters

**suffix** (`std::string_view`)

The literal text to check for at the end of each cell. An empty suffix matches every
non-null cell.

## Returns

`Shelf`
    A new `Bool`-typed `Series` shelf, same length as the bound shelf, holding the
    elementwise suffix check.

## Examples

Example 1. Check a filenames shelf for a literal, case-sensitive suffix.

```cpp
using namespace pluss::table;

auto files = Shelf(pluss::make_note({"report.pdf", "notes.PDF", "summary.docx"}));
auto out = files.nt().ends(".pdf");
std::cout << out;
```

### Output

```text
0  true
1  false
2  false

ptype: Series
```

Row `1` (`"notes.PDF"`) doesn't match — the uppercase `".PDF"` extension doesn't
match lowercase `".pdf"` under a case-sensitive comparison.

Example 2. Case-insensitive suffix matching by normalizing first.

```cpp
auto out2 = files.nt().lower().nt().ends(".pdf");
std::cout << out2;
```

### Output

```text
0  true
1  true
2  false

ptype: Series
```

Lowercasing first folds `"notes.PDF"` down to `"notes.pdf"`, so row `1` now matches
too — `ends()` itself never gained a case-insensitivity option.

Example 3. A non-`Note` shelf throws — the same message
[`Shelf::ends()`](../shelf/ends.md) produces directly.

```cpp
auto numbers = Shelf(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().ends("1");
} catch (const pluss::Error& e) {
  std::string msg(e.message());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Shelf::ends: expected Note payload
```

## Notes

- `ShelfNoteAccessor::ends()` is a one-line forward to
  [`Shelf::ends()`](../shelf/ends.md) — identical behavior and throws.
- Case-sensitive, literal comparison only — no regex mode.
- Changes the payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.

## See Also

- [`Shelf::ends`](../shelf/ends.md): the direct, non-accessor form this method
  forwards to.
- [`ShelfNoteAccessor::starts`](starts.md): the same check anchored at the start
  instead of the end.
- [`ShelfNoteAccessor::lower`](lower.md) / [`ShelfNoteAccessor::casefold`](casefold.md):
  normalize case ahead of a case-insensitive suffix check.

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
