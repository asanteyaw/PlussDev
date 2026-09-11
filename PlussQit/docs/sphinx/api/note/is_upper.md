<!-- pluss-hand-authored -->
# Note::is_upper

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::is_upper() const
```

## Test Whether A Cell Is Upper-Case

`is_upper()` reports, for each cell, whether it **contains at least one cased
letter and none of them are lower-case**. Digits, spaces, and punctuation are
ignored; a cell with no letters (`"123"`, `""`) fails.

Case is judged on **ASCII letters**. A non-ASCII letter is treated as
uncased.

### Return value

A flat `std::vector<std::uint8_t>` of `0`/`1`, one per cell, row-major,
length `numel()`.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note(
    {"ABC", "abc", "Abc", "ABC123", "A B", "123", ""});
const auto mask = n.is_upper();
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 0, 0, 1, 1, 0, 0]
```

`"ABC123"` and `"A B"` pass (the digit and space are ignored); `"123"` and
`""` have no cased letter.

## See Also

- [`Note::is_lower`](is_lower.md): the lower-case counterpart.
- [`Note::is_title`](is_title.md): title-case check.
- [`Note::upper`](upper.md): convert to upper-case (Unicode-aware).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_title.html">
    <span class="label">Previous</span>
    <span class="title">is_title</span>
  </a>
  <a class="next-link" href="is_utf8_valid.html">
    <span class="label">Next</span>
    <span class="title">is_utf8_valid</span>
  </a>
</div>
