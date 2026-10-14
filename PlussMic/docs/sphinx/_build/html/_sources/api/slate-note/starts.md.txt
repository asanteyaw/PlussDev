<!-- pluss-hand-authored -->

# SlateNoteAccessor::starts

[Back to SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference)

## Signature

```cpp
SlateNoteAccessor::starts(std::string_view prefix) const -> Slate
```

## What It Does

Checks the bound slate's cells for a literal, case-sensitive prefix match,
replacing each cell with a `Bool` — a direct forward to
[`Slate::starts()`](../slate/starts.md). Requires the bound slate to be
`Note`-kind. Always a plain, case-sensitive, literal check — no regex mode; use
[`matches()`](matches.md) with a start-anchored pattern for anything more
flexible.

## Parameters

**prefix** (`std::string_view`)

The literal text to check for at the start of each cell. An empty prefix
matches every non-null cell.

## Returns

`Slate`
    A new `Bool`-typed `Series` slate, same length as the bound slate, holding
    the elementwise prefix check.

## Examples

Example 1. Check a text slate for a literal prefix.

```cpp
using namespace pluss::table;

auto codes = Slate(pluss::make_note({"SKU-1001", "sku-1002", "REF-2003"}));
auto out = codes.nt().starts("SKU-");
std::cout << out;
```

### Output

```text
0  true
1  false
2  false

ptype: Series
```

Row `1` (`"sku-1002"`) doesn't match — the comparison is case-sensitive.

Example 2. A non-`Note` slate throws — the same message
[`Slate::starts()`](../slate/starts.md) produces directly.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().starts("1");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::starts: expected Note payload
```

## Notes

- `SlateNoteAccessor::starts()` is a one-line forward to
  [`Slate::starts()`](../slate/starts.md) — identical behavior and throws.
- Case-sensitive, literal comparison only — no regex mode. Normalize with
  [`lower()`](lower.md)/[`casefold()`](casefold.md) first, or use
  [`matches()`](matches.md), for anything more flexible.
- Changes the payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output.

## See Also

- [`Slate::starts`](../slate/starts.md): the direct, non-accessor form this
  method forwards to.
- [`SlateNoteAccessor::ends`](ends.md): the same check anchored at the end
  instead of the start.
- [`SlateNoteAccessor::matches`](matches.md): a regex-anchored-at-start check,
  for patterns more flexible than a literal prefix.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split.html">
    <span class="label">Previous</span>
    <span class="title">split</span>
  </a>
  <a class="next-link" href="strip.html">
    <span class="label">Next</span>
    <span class="title">strip</span>
  </a>
</div>
