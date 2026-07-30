<!-- pluss-hand-authored -->

# ThreadNoteAccessor::ends

[Back to ThreadNoteAccessor API Reference](../../thread_note_accessor_api_reference)

## Signature

```cpp
ThreadNoteAccessor::ends(std::string_view suffix) const -> Thread
```

## What It Does

Checks the bound thread's cells for a literal, case-sensitive suffix match,
replacing each cell with a `Bool` — `true` where the cell's text ends with
`suffix`, `false` where it doesn't. A direct forward to
[`Thread::ends()`](../thread/ends.md); the mirror image of [`starts()`](starts.md).
Requires the bound thread to be `Note`-kind.

## Parameters

**suffix** (`std::string_view`)

The literal text to check for at the end of each cell. An empty suffix matches every
non-null cell.

## Returns

`Thread`
    A new `Bool`-typed `Series` thread, same length as the bound thread, holding the
    elementwise suffix check.

## Examples

Example 1. Check a filenames thread for a literal, case-sensitive suffix.

```cpp
using namespace pluss::table;

auto files = Thread(pluss::make_note({"report.pdf", "notes.PDF", "summary.docx"}));
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

Example 3. A non-`Note` thread throws — the same message
[`Thread::ends()`](../thread/ends.md) produces directly.

```cpp
auto numbers = Thread(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().ends("1");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::ends: expected Note payload
```

## Notes

- `ThreadNoteAccessor::ends()` is a one-line forward to
  [`Thread::ends()`](../thread/ends.md) — identical behavior and throws.
- Case-sensitive, literal comparison only — no regex mode.
- Changes the payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.

## See Also

- [`Thread::ends`](../thread/ends.md): the direct, non-accessor form this method
  forwards to.
- [`ThreadNoteAccessor::starts`](starts.md): the same check anchored at the start
  instead of the end.
- [`ThreadNoteAccessor::lower`](lower.md) / [`ThreadNoteAccessor::casefold`](casefold.md):
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
