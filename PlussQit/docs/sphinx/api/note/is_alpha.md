<!-- pluss-hand-authored -->
# Note::is_alpha

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::is_alpha() const
```

## Test Whether Every Character In A Cell Is A Letter

`is_alpha()` reports, for each cell, whether the cell is **non-empty and
every byte is an ASCII letter** (`A`–`Z`, `a`–`z`). It is one of the
character-property predicates, for validating that a field contains only the
class of characters you expect before parsing or storing it.

### Scope

The check is **ASCII-only**: it uses byte-level classification, so an
accented letter (`é`, `ü`) or a non-Latin letter causes the cell to fail
even though a human would call it alphabetic. If your data has non-ASCII
letters, transliterate first ([`to_ascii`](../../note_api_reference.md)), or
use a regex predicate with an explicit class.

### Return value

Not a `Note`: a flat `std::vector<std::uint8_t>` of `0`/`1`, one entry per
cell, in row-major order, length `numel()`. An **empty cell is `0`** (there
is no character to be a letter), matching the rest of the `is_*` family.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note(
    {"abc", "ABC", "Abc", "abc123", "123", "", "a b", "café"});
const auto mask = n.is_alpha();
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 1, 0, 0, 0, 0, 0]
```

`"abc123"` has digits, `"123"` has no letters, the empty and space cells have
none, `"a b"` has a space, and `"café"` has the non-ASCII `é` — all `0`.

## See Also

- [`Note::is_alnum`](is_alnum.md) / [`Note::is_digit`](is_digit.md): the
  other character-class checks.
- [`Note::is_lower`](is_lower.md) / [`Note::is_upper`](is_upper.md): letter
  **case** checks.
- [`Note::contains_any_alpha`](contains_any_alpha.md): "has at least one
  letter" rather than "is all letters".

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_alnum.html">
    <span class="label">Previous</span>
    <span class="title">is_alnum</span>
  </a>
  <a class="next-link" href="is_ascii.html">
    <span class="label">Next</span>
    <span class="title">is_ascii</span>
  </a>
</div>
