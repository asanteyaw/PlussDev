<!-- pluss-hand-authored -->
# Note::is_ascii

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::is_ascii() const
```

## Test Whether Every Byte In A Cell Is ASCII

`is_ascii()` reports, for each cell, whether every byte is in the range
`0x00`–`0x7F`. Use it to check that a field will survive an ASCII-only
channel unchanged, to route non-ASCII rows to a normalization step, or to
confirm that a byte-level operation (the ASCII case predicates, the ASCII
tokenizers) will behave as intended on that cell.

Unlike the character-class predicates, `is_ascii()` on an **empty cell is
`1`** — vacuously, every byte (none) is ASCII.

### Return value

A flat `std::vector<std::uint8_t>` of `0`/`1`, one per cell, row-major,
length `numel()`.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note({"plain", "", "café", "日本語", "a\tb"});
const auto mask = n.is_ascii();
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 0, 0, 1]
```

`"café"` and `"日本語"` have bytes `>= 0x80`; `"a\tb"` is fine (tab is
`0x09`).

## See Also

- [`Note::is_utf8_valid`](is_utf8_valid.md): a weaker check — well-formed
  UTF-8, ASCII or not.
- `pluss::coden::Note::to_ascii` / `transliterate_ascii`: coerce a cell to
  ASCII.
- [`Note::lengths_bytes`](lengths_bytes.md) vs
  [`Note::lengths_chars`](lengths_chars.md): they agree exactly when
  `is_ascii()` is `1`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_alpha.html">
    <span class="label">Previous</span>
    <span class="title">is_alpha</span>
  </a>
  <a class="next-link" href="is_blank.html">
    <span class="label">Next</span>
    <span class="title">is_blank</span>
  </a>
</div>
