<!-- pluss-hand-authored -->

# ShelfNoteAccessor::lower

[Back to ShelfNoteAccessor API Reference](../../shelf_note_accessor_api_reference)

## Signature

```cpp
Shelf ShelfNoteAccessor::lower() const
```

## What It Does

Lowercases every character in the bound shelf's cells — the classic normalization
step before a case-insensitive comparison, a grouping key that shouldn't be split by
casing accidents, or a lookup against a canonical lowercase reference list. A direct
forward to [`Shelf::lower()`](../shelf/lower.md). Requires the bound shelf to be
`Note`-kind.

```{note} ASCII vs. Unicode case mapping
On plain ASCII text, `lower()` behaves exactly as you'd expect. Non-ASCII case
mapping depends on whether this build links the optional Unicode backend (GNU
libunistring).
```

## Parameters

`None`

## Returns

`Shelf`
    A new `Note`-kind shelf, same length as the bound shelf, with every character
    lowercased.

## Examples

Example 1. Lowercase a text shelf via the accessor.

```cpp
using namespace pluss::table;

auto cities = Shelf(pluss::make_note({"Accra", "TEMA", "Kumasi"}));
auto out = cities.nt().lower();
std::cout << out;
```

### Output

```text
0  accra
1  tema
2  kumasi

ptype: Note
```

Example 2. A non-`Note` shelf throws — the same message
[`Shelf::lower()`](../shelf/lower.md) produces directly.

```cpp
auto numbers = Shelf(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().lower();
} catch (const pluss::Error& e) {
  std::string msg(e.message());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Shelf::lower: expected Note payload
```

## Notes

- `ShelfNoteAccessor::lower()` is a one-line forward to
  [`Shelf::lower()`](../shelf/lower.md) — identical behavior and throws.
- A null cell on input stays null on output.
- For the opposite transform, see [`upper()`](upper.md). For a comparison-oriented
  case normalization, see [`casefold()`](casefold.md).

## See Also

- [`Shelf::lower`](../shelf/lower.md): the direct, non-accessor form this method
  forwards to.
- [`ShelfNoteAccessor::upper`](upper.md): the inverse transform.
- [`ShelfNoteAccessor::casefold`](casefold.md): case folding intended for
  comparison rather than display.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="length.html">
    <span class="label">Previous</span>
    <span class="title">length</span>
  </a>
  <a class="next-link" href="lstrip.html">
    <span class="label">Next</span>
    <span class="title">lstrip</span>
  </a>
</div>
