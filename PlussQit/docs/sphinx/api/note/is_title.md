<!-- pluss-hand-authored -->
# Note::is_title

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::is_title() const
```

## Test Whether A Cell Is Title-Cased

`is_title()` reports, for each cell, whether it is title-cased: **every
alphabetic word begins with an upper-case letter and continues in
lower-case, and there is at least one such cased word**. It is the check that
pairs with
[`title`](title.md): `note.title().is_title()` is all `1` for cells that
have any letters.

Word boundaries are any non-letter (space, punctuation, digit). A cell with
no cased letters at all — `"123"`, `"  "`, `""` — is **not** title-cased and
returns `0`. Case is judged on **ASCII letters**; a non-ASCII letter is
uncased and does not start or break a word for this check.

### Return value

A flat `std::vector<std::uint8_t>` of `0`/`1`, one per cell, row-major,
length `numel()`.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note(
    {"Two Words", "Abc", "TwoWords", "two words", "TWO", "123", ""});
const auto mask = n.is_title();
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 0, 0, 0, 0, 0]
```

`"TwoWords"` has an internal capital `W` (mid-word upper-case); `"two
words"` has no leading capitals; `"TWO"` is all upper; `"123"` and `""`
have no cased letters.

## See Also

- [`Note::title`](title.md): convert a cell to title case.
- [`Note::is_lower`](is_lower.md) / [`Note::is_upper`](is_upper.md): the
  single-case checks.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_tensor3.html">
    <span class="label">Previous</span>
    <span class="title">is_tensor3</span>
  </a>
  <a class="next-link" href="is_upper.html">
    <span class="label">Next</span>
    <span class="title">is_upper</span>
  </a>
</div>
