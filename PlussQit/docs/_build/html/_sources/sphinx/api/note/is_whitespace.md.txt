<!-- pluss-hand-authored -->
# Note::is_whitespace

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::is_whitespace() const
```

## Test Whether A Cell Is All Whitespace

`is_whitespace()` reports, for each cell, whether it is **non-empty and
every byte is an ASCII whitespace character** (space, `\t`, `\n`, `\r`,
`\f`, `\v`). Use it to detect padding-only cells before a trim step, or to
filter blank rows.

The one thing to remember: the **empty string is not whitespace** and
returns `0` (there is no character to be whitespace). If you want
"empty or all-whitespace", that is [`is_blank`](is_blank.md).

### Return value

A flat `std::vector<std::uint8_t>` of `0`/`1`, one per cell, row-major,
length `numel()`.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note({"  ", "\t\n", "", " a ", "abc"});
const auto mask = n.is_whitespace();
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 0, 0, 0]
```

`"  "` and `"\t\n"` are all whitespace; the empty cell is `0`; `" a "` has a
letter.

## See Also

- [`Note::is_blank`](is_blank.md): empty **or** all-whitespace.
- [`Note::strip`](strip.md): remove the boundary whitespace.
- [`Note::lengths_bytes`](lengths_bytes.md): `0` for an empty cell — another
  way to spot true blanks.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_vector.html">
    <span class="label">Previous</span>
    <span class="title">is_vector</span>
  </a>
  <a class="next-link" href="jaccard_acronyms.html">
    <span class="label">Next</span>
    <span class="title">jaccard_acronyms</span>
  </a>
</div>
