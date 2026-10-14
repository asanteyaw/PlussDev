<!-- pluss-hand-authored -->

# SlateNoteAccessor::length

[Back to SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference)

## Signature

```cpp
SlateNoteAccessor::length() const -> Slate
```

## What It Does

Replaces the bound slate with its character count, as an `Int64` `Series`-kind
slate — a direct forward to [`Slate::length()`](../slate/length.md). Requires the
bound slate to be `Note`-kind.

```{important} Counts Unicode codepoints, not bytes
`length()` counts *characters* (Unicode codepoints), not raw UTF-8 storage bytes.
For plain ASCII text the two are identical — but any character outside the ASCII
range is encoded as more than one byte in UTF-8 while still counting as exactly one
character.
```

## Parameters

`None`

## Returns

`Slate`
    A new `Series`-kind (`Int64`) slate, same length as the bound slate, holding
    each cell's character count.

## Examples

Example 1. Character count of a plain ASCII text slate.

```cpp
using namespace pluss::table;

auto cities = Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}));
auto out = cities.nt().length();
std::cout << out;
```

### Output

```text
0  5
1  4
2  6

ptype: Series
```

Example 2. A non-`Note` slate throws — the same message
[`Slate::length()`](../slate/length.md) produces directly.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().length();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::length: expected Note payload
```

## Notes

- `SlateNoteAccessor::length()` is a one-line forward to
  [`Slate::length()`](../slate/length.md) — identical behavior and throws.
- Changes the payload kind from `Note` to `Series` (`Int64`).
- Counts Unicode codepoints, not raw UTF-8 storage bytes.
- A null cell on input stays null on output.

## See Also

- [`Slate::length`](../slate/length.md): the direct, non-accessor form this
  method forwards to.
- [`SlateNoteAccessor::pad`](pad.md): uses the same character-count notion of
  "width."
- [`SlateNoteAccessor::chars`](chars.md): slice out a character range instead of
  counting.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="indicate.html">
    <span class="label">Previous</span>
    <span class="title">indicate</span>
  </a>
  <a class="next-link" href="lower.html">
    <span class="label">Next</span>
    <span class="title">lower</span>
  </a>
</div>
