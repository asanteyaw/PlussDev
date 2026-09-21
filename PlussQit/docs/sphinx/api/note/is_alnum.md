<!-- pluss-hand-authored -->
# Note::is_alnum

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::is_alnum() const
```

## Test Whether Every Character In A Cell Is A Letter Or Digit

`is_alnum()` reports, for each cell, whether the cell is **non-empty and
every byte is an ASCII letter or digit** (`A`–`Z`, `a`–`z`, `0`–`9`). Use it
to validate identifier-like fields — slugs, codes, keys — that must contain
no spaces, punctuation, or separators.

The check is **ASCII-only** (byte-level), so `é`, `ü`, or any non-Latin
letter fails the cell. An **empty cell is `0`**, consistent with the rest of the
`is_*` family.

### Return value

A Bool `Note` with the receiver's shape, one native truth value per cell.
Use `bools()` for a borrowed contiguous host view.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note(
    {"abc", "abc123", "123", "abc_123", "", "a b"});
const auto mask = n.is_alnum();
std::cout << "[";
for (std::size_t i = 0; i < mask.bools().size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask.bools()[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 1, 0, 0, 0]
```

`"abc_123"` has an underscore, the empty cell has nothing, and `"a b"` has a
space — all `0`.

## See Also

- [`Note::is_alpha`](is_alpha.md) / [`Note::is_digit`](is_digit.md): the
  narrower checks.
- [`Note::contains_any_digit`](contains_any_digit.md) /
  [`Note::contains_any_alpha`](contains_any_alpha.md): "has at least one"
  rather than "is all".

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="integers.html">
    <span class="label">Previous</span>
    <span class="title">integers</span>
  </a>
  <a class="next-link" href="is_alpha.html">
    <span class="label">Next</span>
    <span class="title">is_alpha</span>
  </a>
</div>
