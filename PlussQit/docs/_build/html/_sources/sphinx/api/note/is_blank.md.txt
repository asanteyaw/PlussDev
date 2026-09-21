<!-- pluss-hand-authored -->
# Note::is_blank

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::is_blank() const
```

## Test Whether A Cell Has No Visible Content

`is_blank()` reports, for each cell, whether it is **empty or contains only
ASCII whitespace** (space, `\t`, `\n`, `\r`, `\f`, `\v`). It is the "is this
field effectively empty?" check — the one to use for filtering blank rows,
counting missing values, or deciding whether a cell needs a default.

The difference from [`is_whitespace`](is_whitespace.md) is the empty case:
`is_blank("")` is `1`, `is_whitespace("")` is `0`. `is_blank` treats the
empty string and a whitespace-only string the same way.

### Return value

A Bool `Note` with the receiver's shape, one native truth value per cell.
Use `bools()` for a borrowed contiguous host view.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note({"", "  ", "\t", " x ", "x"});
const auto mask = n.is_blank();
std::cout << "[";
for (std::size_t i = 0; i < mask.bools().size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask.bools()[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 1, 0, 0]
```

The empty cell, `"  "`, and `"\t"` are all blank; `" x "` and `"x"` have a
letter.

## See Also

- [`Note::is_whitespace`](is_whitespace.md): all-whitespace but **not**
  empty.
- [`Note::strip`](strip.md): trim boundary whitespace (a blank cell strips
  to `""`).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_ascii.html">
    <span class="label">Previous</span>
    <span class="title">is_ascii</span>
  </a>
  <a class="next-link" href="is_bool.html">
    <span class="label">Next</span>
    <span class="title">is_bool</span>
  </a>
</div>
