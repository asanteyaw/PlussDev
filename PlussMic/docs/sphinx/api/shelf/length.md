<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d18a8f8605d6 -->

# Shelf::length

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::length() const
```

## What It Does

Replaces a `Note` (string) shelf with its character count, as an `Int64`
`Series`-kind shelf — same row order, but the payload kind changes from `Note` to
`Series`. Requires the shelf to be `Note`-kind.

```{important} Counts Unicode codepoints, not bytes
`length()` counts *characters* (Unicode codepoints), not raw UTF-8 storage bytes.
For plain ASCII text the two are identical — but any character outside the ASCII
range (an accented letter, a currency symbol, an emoji) is encoded as more than one
byte in UTF-8 while still counting as exactly one character.
```

## Parameters

`None`

## Returns

`Shelf`
    A new `Series`-kind (`Int64`) shelf, same length as `*this`, holding each
    cell's character count.

## Examples

Example 1. Character count of a plain ASCII text shelf.

```cpp
using namespace pluss::table;

auto cities = Shelf(pluss::make_note({"Accra", "Tema", "Kumasi"}));
auto out = cities.length();
std::cout << out;
```

### Output

```text
0  5
1  4
2  6

ptype: Series
```

`"Accra"` is 5 characters, `"Tema"` is 4, `"Kumasi"` is 6.

Example 2. A non-`Note` shelf throws.

```cpp
auto numbers = Shelf(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.length();
} catch (const pluss::Error& e) {
  std::string msg(e.message());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Shelf::length: expected Note payload
```

## Notes

- Changes the shelf's payload kind from `Note` to `Series` (`Int64`) — chain a
  numeric operation (comparison, arithmetic) directly onto the result.
- Counts Unicode codepoints, not raw UTF-8 storage bytes.
- A null cell on input stays null on output.
- See [`DataFrameNoteAccessor::length`](../dataframe-note/length.md) for the
  multi-feature counterpart and a worked multi-byte-character example.

## See Also

- [`Shelf::pad`](pad.md): uses the same character-count notion of "width."
- [`Shelf::chars`](chars.md): slice out a character range instead of counting.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="le.html">
    <span class="label">Previous</span>
    <span class="title">le</span>
  </a>
  <a class="next-link" href="log.html">
    <span class="label">Next</span>
    <span class="title">log</span>
  </a>
</div>
