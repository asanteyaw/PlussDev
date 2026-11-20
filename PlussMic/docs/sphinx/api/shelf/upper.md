<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4cf18ce5d958 -->

# Shelf::upper

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::upper() const
```

## What It Does

Uppercases every character in a `Note` (string) shelf's cells — the mirror image
of [`lower()`](lower.md). Common uses include normalizing free-typed codes or
abbreviations before comparing or grouping by them. Requires the shelf to be
`Note`-kind.

```{note} ASCII vs. Unicode case mapping
Exactly the same caveat as [`lower()`](lower.md): ASCII `a`-`z` mapping is always
available; correct handling of accented or non-Latin uppercase mapping depends on
whether this build links the optional Unicode backend.
```

## Parameters

`None`

## Returns

`Shelf`
    A new `Note`-kind shelf, same length as `*this`, with every character
    uppercased; non-alphabetic characters are left exactly as they were.

## Examples

Example 1. Uppercase a text shelf.

```cpp
using namespace pluss::table;

auto codes = Shelf(pluss::make_note({"gh-001", "Gh-002", "GH-003"}));
auto out = codes.upper();
std::cout << out;
```

### Output

```text
0  GH-001
1  GH-002
2  GH-003

ptype: Note
```

`"gh"`, `"Gh"`, and `"GH"` all normalize to the same `"GH"` — the hyphen and digits
are untouched throughout, since `upper()` only ever changes letters.

Example 2. A non-`Note` shelf throws.

```cpp
auto numbers = Shelf(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.upper();
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

- A null cell on input stays null on output.
- For the opposite transform, see [`lower()`](lower.md).

## See Also

- [`Shelf::lower`](lower.md): the inverse transform.
- [`Shelf::casefold`](casefold.md): case folding intended for comparison rather
  than display.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unname.html">
    <span class="label">Previous</span>
    <span class="title">unname</span>
  </a>
  <a class="next-link" href="value.html">
    <span class="label">Next</span>
    <span class="title">value</span>
  </a>
</div>
