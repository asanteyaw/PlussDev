<!-- pluss-hand-authored -->

# ShelfNoteAccessor::upper

[Back to ShelfNoteAccessor API Reference](../../shelf_note_accessor_api_reference)

## Signature

```cpp
Shelf ShelfNoteAccessor::upper() const
```

## What It Does

Uppercases every character in the bound shelf's cells — the mirror image of
[`lower()`](lower.md). A direct forward to
[`Shelf::upper()`](../shelf/upper.md). Requires the bound shelf to be
`Note`-kind. Common uses include normalizing free-typed codes or abbreviations
before comparing or grouping by them.

```{note} ASCII vs. Unicode case mapping
Exactly the same caveat as [`lower()`](lower.md): ASCII `a`-`z` mapping is
always available; correct handling of accented or non-Latin uppercase mapping
depends on whether this build links the optional Unicode backend.
```

## Parameters

`None`

## Returns

`Shelf`
    A new `Note`-kind shelf, same length as the bound shelf, with every
    character uppercased; non-alphabetic characters are left exactly as they
    were.

## Examples

Example 1. Uppercase a text shelf via the accessor.

```cpp
using namespace pluss::table;

auto codes = Shelf(pluss::make_note({"gh-001", "Gh-002", "GH-003"}));
auto out = codes.nt().upper();
std::cout << out;
```

### Output

```text
0  GH-001
1  GH-002
2  GH-003

ptype: Note
```

`"gh"`, `"Gh"`, and `"GH"` all normalize to the same `"GH"` — the hyphen and
digits are untouched throughout.

Example 2. A non-`Note` shelf throws — the same message
[`Shelf::upper()`](../shelf/upper.md) produces directly.

```cpp
auto numbers = Shelf(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().upper();
} catch (const pluss::Error& e) {
  std::string msg(e.message());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Shelf::upper: expected Note payload
```

## Notes

- `ShelfNoteAccessor::upper()` is a one-line forward to
  [`Shelf::upper()`](../shelf/upper.md) — identical behavior and throws.
- A null cell on input stays null on output.
- For the opposite transform, see [`lower()`](lower.md).

## See Also

- [`Shelf::upper`](../shelf/upper.md): the direct, non-accessor form this
  method forwards to.
- [`ShelfNoteAccessor::lower`](lower.md): the inverse transform.
- [`ShelfNoteAccessor::casefold`](casefold.md): case folding intended for
  comparison rather than display.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="titlecase.html">
    <span class="label">Previous</span>
    <span class="title">titlecase</span>
  </a>
</div>
