<!-- pluss-hand-authored -->

# SlateNoteAccessor::ends

[Back to SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference)

## Signature

```cpp
SlateNoteAccessor::ends(std::string_view suffix) const -> Slate
```

## What It Does

Checks the bound slate's cells for a literal, case-sensitive suffix match,
replacing each cell with a `Bool` — `true` where the cell's text ends with
`suffix`, `false` where it doesn't. A direct forward to
[`Slate::ends()`](../slate/ends.md); the mirror image of [`starts()`](starts.md).
Requires the bound slate to be `Note`-kind.

## Parameters

**suffix** (`std::string_view`)

The literal text to check for at the end of each cell. An empty suffix matches every
non-null cell.

## Returns

`Slate`
    A new `Bool`-typed `Series` slate, same length as the bound slate, holding the
    elementwise suffix check.

## Examples

Example 1. Check a filenames slate for a literal, case-sensitive suffix.

```cpp
using namespace pluss::table;

auto files = Slate(pluss::make_note({"report.pdf", "notes.PDF", "summary.docx"}));
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

Example 3. A non-`Note` slate throws — the same message
[`Slate::ends()`](../slate/ends.md) produces directly.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().ends("1");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::ends: expected Note payload
```

## Notes

- `SlateNoteAccessor::ends()` is a one-line forward to
  [`Slate::ends()`](../slate/ends.md) — identical behavior and throws.
- Case-sensitive, literal comparison only — no regex mode.
- Changes the payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.

## See Also

- [`Slate::ends`](../slate/ends.md): the direct, non-accessor form this method
  forwards to.
- [`SlateNoteAccessor::starts`](starts.md): the same check anchored at the start
  instead of the end.
- [`SlateNoteAccessor::lower`](lower.md) / [`SlateNoteAccessor::casefold`](casefold.md):
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
