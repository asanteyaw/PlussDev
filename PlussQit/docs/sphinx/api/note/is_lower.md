<!-- pluss-hand-authored -->
# Note::is_lower

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::is_lower() const
```

## Test Whether A Cell Is Lower-Case

`is_lower()` reports, for each cell, whether it **contains at least one
cased letter and none of them are upper-case**. Digits, spaces, and
punctuation are ignored, so
`"abc123"` and `"a b"` pass, but a cell with no letters at all (`"123"`,
`""`) fails because there is nothing cased to be lower-case.

Case is judged on **ASCII letters** (`A`–`Z` / `a`–`z`). A non-ASCII letter
is treated as uncased — it neither satisfies nor breaks the check — so
`"café"` passes on the strength of its ASCII `caf`.

### Return value

A Bool `Note` with the receiver's shape, one native truth value per cell.
Use `bools()` for a borrowed contiguous host view.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note(
    {"abc", "ABC", "Abc", "abc123", "123", "", "a b", "café"});
const auto mask = n.is_lower();
std::cout << "[";
for (std::size_t i = 0; i < mask.bools().size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask.bools()[i]);
std::cout << "]\n";
```

### Output

```text
[1, 0, 0, 1, 0, 0, 1, 1]
```

`"ABC"` and `"Abc"` have an upper-case letter; `"123"` and `""` have no
cased letter at all.

## See Also

- [`Note::is_upper`](is_upper.md): the upper-case counterpart.
- [`Note::is_title`](is_title.md): title-case check.
- [`Note::lower`](lower.md): convert to lower-case (Unicode-aware).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_int64.html">
    <span class="label">Previous</span>
    <span class="title">is_int64</span>
  </a>
  <a class="next-link" href="is_matrix.html">
    <span class="label">Next</span>
    <span class="title">is_matrix</span>
  </a>
</div>
