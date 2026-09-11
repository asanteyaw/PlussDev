<!-- pluss-hand-authored -->
# Note::contains_any_alpha

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::contains_any_alpha() const
```

## Test Whether A Cell Has At Least One Letter

`contains_any_alpha()` reports, for each cell, whether **any** byte is an
ASCII letter (`A`–`Z`, `a`–`z`). It is the "some" counterpart of
[`is_alpha`](is_alpha.md)'s "all": use it to separate rows that carry text
from rows that are pure numbers, codes, or punctuation.

The check is **ASCII-only** — a cell whose only letters are non-ASCII (`é`,
`日`) returns `0` unless it also has an ASCII letter. An empty cell returns
`0`.

### Return value

A flat `std::vector<std::uint8_t>` of `0`/`1`, one per cell, row-major,
length `numel()`.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note({"abc", "abc123", "123", "-42-", "", "a1"});
const auto mask = n.contains_any_alpha();
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 0, 0, 0, 1]
```

## See Also

- [`Note::is_alpha`](is_alpha.md): "every character is a letter".
- [`Note::contains_any_digit`](contains_any_digit.md): the digit version.
- [`Note::count_substring`](count_substring.md): count occurrences of a
  literal, not a class.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contains.html">
    <span class="label">Previous</span>
    <span class="title">contains</span>
  </a>
  <a class="next-link" href="contains_any_digit.html">
    <span class="label">Next</span>
    <span class="title">contains_any_digit</span>
  </a>
</div>
