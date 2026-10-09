<!-- pluss-hand-authored -->
# Note::RegexAccessor::search

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::RegexAccessor::search(std::string_view pattern) const
```

## Test Whether A Pattern Occurs Anywhere In A Cell

`search(pattern)` reports, for each cell, whether `pattern` matches
**somewhere** in the cell — unanchored, the regex analogue of
[`Note::contains`](../note/contains.md). Use it to flag cells that contain a shape (a date,
a URL, a code) without caring where in the cell it appears or what surrounds
it.

The difference from [`match`](match.md) is anchoring:
`search(R"(\d+)")` is `1` for `"year 2026"`, `match(R"(\d+)")` is
`0` for it (the whole cell is not digits).

`pattern` is an ECMAScript regular expression. An invalid pattern throws
`pluss::Error` before any cell is tested.

### Return value

A Bool `Note` with the receiver's shape, one native truth value per cell.
Use `bools()` for a borrowed contiguous host view.

## Example 1: Unanchored — Match Anywhere

```cpp
const auto n = pluss::make_note({"2026", "year 2026", "none"});
const auto mask = n.regex().search(R"(\d+)");
std::cout << "[";
for (std::size_t i = 0; i < mask.bools().size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask.bools()[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 0]
```

Both `"2026"` and `"year 2026"` contain a digit run.

## See Also

- [`Note::RegexAccessor::match`](match.md): the whole-cell, anchored version.
- [`Note::contains`](../note/contains.md): a literal (non-regex) "occurs anywhere"
  test.
- [`Note::RegexAccessor::findall`](findall.md): return the matches, not just a
  yes/no.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace.html">
    <span class="label">Previous</span>
    <span class="title">replace</span>
  </a>
  <a class="next-link" href="split.html">
    <span class="label">Next</span>
    <span class="title">split</span>
  </a>
</div>
