<!-- pluss-hand-authored -->
# Note::is_digit

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::is_digit() const
```

## Test Whether Every Character In A Cell Is A Digit

`is_digit()` reports, for each cell, whether the cell is **non-empty and
every byte is an ASCII digit** `0`–`9`. Use it to gate a numeric parse — a
cell that passes can be turned into an integer without surprises (no sign,
no decimal point, no thousands separator, no whitespace).

The check is byte-level, so it accepts **only** `0`–`9`: a leading `-`, a
`.`, a `,`, or a Unicode digit all fail the cell. An **empty cell is `0`**,
consistent with the rest of the `is_*` family.

### Return value

A flat `std::vector<std::uint8_t>` of `0`/`1`, one per cell, row-major,
length `numel()`.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note({"123", "007", "12.5", "-3", "1 2", "", "12a"});
const auto mask = n.is_digit();
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 0, 0, 0, 0, 0]
```

Only `"123"` and `"007"` are pure digit runs.

## See Also

- [`Note::is_alnum`](is_alnum.md): letters **or** digits.
- [`Note::contains_any_digit`](contains_any_digit.md): "has at least one
  digit" rather than "is all digits".
- [`Note::regex_match`](regex_match.md): a whole-cell pattern check for
  richer numeric shapes (`-?\d+(\.\d+)?`).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_bytes.html">
    <span class="label">Previous</span>
    <span class="title">is_bytes</span>
  </a>
  <a class="next-link" href="is_lower.html">
    <span class="label">Next</span>
    <span class="title">is_lower</span>
  </a>
</div>
