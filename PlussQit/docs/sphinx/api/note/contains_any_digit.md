<!-- pluss-hand-authored -->
# Note::contains_any_digit

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::contains_any_digit() const
```

## Test Whether A Cell Has At Least One Digit

`contains_any_digit()` reports, for each cell, whether **any** byte is an
ASCII digit `0`–`9`. It is the "some" counterpart of
[`is_digit`](is_digit.md)'s "all": use it to flag rows that carry a number
somewhere — an ID, a year, a quantity — as opposed to pure prose.

An empty cell returns `0`.

### Return value

A flat `std::vector<std::uint8_t>` of `0`/`1`, one per cell, row-major,
length `numel()`.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note({"order 17", "abc", "v2", "", "3.14", "no numbers"});
const auto mask = n.contains_any_digit();
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 0, 1, 0, 1, 0]
```

## See Also

- [`Note::is_digit`](is_digit.md): "every character is a digit".
- [`Note::contains_any_alpha`](contains_any_alpha.md): the letter version.
- [`Note::regex_findall`](regex_findall.md): pull the digit runs out.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contains_any_alpha.html">
    <span class="label">Previous</span>
    <span class="title">contains_any_alpha</span>
  </a>
  <a class="next-link" href="contiguous.html">
    <span class="label">Next</span>
    <span class="title">contiguous</span>
  </a>
</div>
