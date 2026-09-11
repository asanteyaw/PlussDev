<!-- pluss-hand-authored -->
# Note::keep_digits

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::keep_digits() const
```

## Keep Only Digit Characters In Every Cell

`keep_digits()` returns a new `Note`, same shape as the input, with every
character that is **not** an ASCII digit `0`–`9` deleted from each cell. Use
it to pull the numeric core out of a formatted value — a phone number, an
ID with separators, a version string — so it can be parsed or matched as one
run of digits.

Only the ten ASCII digits survive. Separators, spaces, letters, and
non-ASCII characters are removed, and their positions are **not** preserved
— `"v2.1.3"` becomes `"213"`, not `"2 1 3"`. A cell with no digits becomes
`""`.

`keep_digits` is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"order 17", "v2.1.3", "none", ""}).keep_digits())
    << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["17", "213", "", ""]
```

## See Also

- [`Note::remove_digits`](remove_digits.md): the complement.
- [`Note::regex_findall`](regex_findall.md): extract digit **runs**
  separately (`R"(\d+)"`) instead of concatenating them.
- [`Note::is_digit`](is_digit.md): test whether a cell is already pure
  digits.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keep_ascii.html">
    <span class="label">Previous</span>
    <span class="title">keep_ascii</span>
  </a>
  <a class="next-link" href="keep_printable.html">
    <span class="label">Next</span>
    <span class="title">keep_printable</span>
  </a>
</div>
