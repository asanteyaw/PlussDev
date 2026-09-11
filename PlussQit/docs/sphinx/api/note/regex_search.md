<!-- pluss-hand-authored -->
# Note::regex_search

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::regex_search(std::string_view pattern) const
```

## Test Whether A Pattern Occurs Anywhere In A Cell

`regex_search(pattern)` reports, for each cell, whether `pattern` matches
**somewhere** in the cell — unanchored, the regex analogue of
[`contains`](contains.md). Use it to flag rows that mention a shape (a date,
a URL, a code) without caring where in the cell it appears or what surrounds
it.

The difference from [`regex_match`](regex_match.md) is anchoring:
`regex_search(R"(\d+)")` is `1` for `"year 2026"`, `regex_match(R"(\d+)")` is
`0` for it (the whole cell is not digits).

`pattern` is an ECMAScript regular expression. An invalid pattern throws
`pluss::Error` before any cell is tested.

### Return value

A flat `std::vector<std::uint8_t>` of `0`/`1`, one per cell, row-major,
length `numel()`.

## Example 1: Unanchored — Match Anywhere

```cpp
const auto n = pluss::make_note({"2026", "year 2026", "none"});
const auto mask = n.regex_search(R"(\d+)");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 0]
```

Both `"2026"` and `"year 2026"` contain a digit run.

## See Also

- [`Note::regex_match`](regex_match.md): the whole-cell, anchored version.
- [`Note::contains`](contains.md): a literal (non-regex) "occurs anywhere"
  test.
- [`Note::regex_findall`](regex_findall.md): return the matches, not just a
  yes/no.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="regex_replace.html">
    <span class="label">Previous</span>
    <span class="title">regex_replace</span>
  </a>
  <a class="next-link" href="remove.html">
    <span class="label">Next</span>
    <span class="title">remove</span>
  </a>
</div>
