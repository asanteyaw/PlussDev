<!-- pluss-hand-authored -->
# Note::remove_digits

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::remove_digits() const
```

## Delete Digit Characters From Every Cell

`remove_digits()` returns a new `Note`, same shape as the input, with every
ASCII digit `0`–`9` deleted from each cell. Use it to drop version numbers,
counts, IDs, or dates from text you want to compare or group by wording
alone.

Only the ten ASCII digits are removed. Letters, whitespace, punctuation, and
non-ASCII characters are kept, and nothing is trimmed or collapsed — so
`"v2.1"` becomes `"v."` and a cell that is all digits becomes `""`.

`remove_digits` is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"v2.1 build 405", "no digits", "2026"}).remove_digits())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["v. build ", "no digits", ""]
```

`"2026"` is all digits, so it reduces to the empty string.

## See Also

- [`Note::keep_digits`](keep_digits.md): the complement — keep **only** the
  digits.
- [`Note::remove_letters`](remove_letters.md) /
  [`Note::remove_punctuation`](remove_punctuation.md): drop other classes.
- [`Note::contains_any_digit`](contains_any_digit.md): test for a digit
  first.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_control_chars.html">
    <span class="label">Previous</span>
    <span class="title">remove_control_chars</span>
  </a>
  <a class="next-link" href="remove_emojis.html">
    <span class="label">Next</span>
    <span class="title">remove_emojis</span>
  </a>
</div>
