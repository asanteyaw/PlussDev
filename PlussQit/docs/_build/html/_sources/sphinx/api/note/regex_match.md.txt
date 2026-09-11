<!-- pluss-hand-authored -->
# Note::regex_match

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::regex_match(std::string_view pattern) const
```

## Test Whether A Whole Cell Matches A Pattern

`regex_match(pattern)` reports, for each cell, whether `pattern` matches the
**entire** cell — anchored at both ends, as if the pattern were wrapped in
`^(?:...)$`. It is the validation predicate of the regex family: use it to
check that a field has exactly the shape you require (a well-formed date, a
key, a code) before trusting it.

Because the match is whole-cell, a pattern like `\d+` passes only for a cell
that is *nothing but* digits; to accept "contains a digit run somewhere",
either loosen the pattern (`.*\d+.*`) or use
[`regex_search`](regex_search.md).

`pattern` is an ECMAScript regular expression. An invalid pattern throws
`pluss::Error` before any cell is tested.

### Return value

A flat `std::vector<std::uint8_t>` of `0`/`1`, one per cell, row-major,
length `numel()`.

## Example 1: Anchored — The Whole Cell Must Match

```cpp
const auto n = pluss::make_note({"2026", "year 2026", "none"});
const auto mask = n.regex_match(R"(\d+)");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 0, 0]
```

Only `"2026"` is entirely digits; `"year 2026"` has other characters.

## Example 2: Loosen The Pattern To Accept Extra Text

```cpp
const auto n = pluss::make_note({"2026", "year 2026", "none"});
const auto mask = n.regex_match(R"(.*\d+.*)");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 0]
```

## Example 3: An Invalid Pattern Is Rejected

```cpp
try {
  pluss::make_note({"x"}).regex_match("(");
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
regex_match: The expression contained mismatched ( and ).
```

## See Also

- [`Note::regex_search`](regex_search.md): the unanchored version — matches
  anywhere in the cell.
- [`Note::regex_findall`](regex_findall.md) /
  [`Note::regex_replace`](regex_replace.md): extract or rewrite matches.
- [`Note::is_digit`](is_digit.md) and the `is_*` family: fixed
  character-class checks with no regex engine.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="regex_findall.html">
    <span class="label">Previous</span>
    <span class="title">regex_findall</span>
  </a>
  <a class="next-link" href="regex_replace.html">
    <span class="label">Next</span>
    <span class="title">regex_replace</span>
  </a>
</div>
