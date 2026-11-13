<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d594efbf6ff7 -->

# Slate::lower

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate Slate::lower() const
```

## What It Does

Lowercases every character in a `Note` (string) slate's cells — the classic
normalization step before a case-insensitive comparison, a grouping key that
shouldn't be split by casing accidents, or a lookup against a canonical lowercase
reference list. Requires the slate to be `Note`-kind.

```{note} ASCII vs. Unicode case mapping
On plain ASCII text, `lower()` behaves exactly as you'd expect: `A`-`Z` becomes
`a`-`z`, everything else is untouched. Non-ASCII case mapping depends on whether this
build links the optional Unicode backend (GNU libunistring).
```

## Parameters

`None`

## Returns

`Slate`
    A new `Note`-kind slate, same length as `*this`, with every character
    lowercased; non-alphabetic characters (digits, punctuation, whitespace) are left
    exactly as they were.

## Examples

Example 1. Lowercase a text slate.

```cpp
using namespace pluss::table;

auto cities = Slate(pluss::make_note({"Accra", "TEMA", "Kumasi"}));
auto out = cities.lower();
std::cout << out;
```

### Output

```text
0  accra
1  tema
2  kumasi

ptype: Note
```

Example 2. A non-`Note` slate throws.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.lower();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::lower: expected Note payload
```

## Notes

- A null cell on input stays null on output.
- For the opposite transform, see [`upper()`](upper.md). For a comparison-oriented
  case normalization, see [`casefold()`](casefold.md).

## See Also

- [`Slate::upper`](upper.md): the inverse transform.
- [`Slate::casefold`](casefold.md): case folding intended for comparison rather
  than display.
- [`Slate::titlecase`](titlecase.md): capitalize the first letter of each word
  instead of lowercasing everything.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lor.html">
    <span class="label">Previous</span>
    <span class="title">lor</span>
  </a>
  <a class="next-link" href="lstrip.html">
    <span class="label">Next</span>
    <span class="title">lstrip</span>
  </a>
</div>
