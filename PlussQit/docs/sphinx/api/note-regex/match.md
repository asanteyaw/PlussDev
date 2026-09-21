<!-- pluss-hand-authored -->
# NoteRegexAccessor::match

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note NoteRegexAccessor::match(std::string_view pattern) const
```

## Test Whether A Whole Cell Matches A Pattern

`match(pattern)` reports, for each cell, whether `pattern` matches the
**entire** cell — anchored at both ends, as if the pattern were wrapped in
`^(?:...)$`. It is the validation predicate of the regex family: use it to
check that a field has exactly the shape you require (a well-formed date, a
key, a code) before trusting it.

Because the match is whole-cell, a pattern like `\d+` passes only for a cell
that is *nothing but* digits; to accept "contains a digit run somewhere",
either loosen the pattern (`.*\d+.*`) or use
[`search`](search.md).

`pattern` is an ECMAScript regular expression. An invalid pattern throws
`pluss::Error` before any cell is tested.

### Return value

A Bool `Note` with the receiver's shape, one native truth value per cell.
Use `bools()` for a borrowed contiguous host view.

## Example 1: Anchored — The Whole Cell Must Match

```cpp
const auto n = pluss::make_note({"2026", "year 2026", "none"});
const auto mask = n.regex().match(R"(\d+)");
std::cout << "[";
for (std::size_t i = 0; i < mask.bools().size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask.bools()[i]);
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
const auto mask = n.regex().match(R"(.*\d+.*)");
std::cout << "[";
for (std::size_t i = 0; i < mask.bools().size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask.bools()[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 0]
```

## Example 3: An Invalid Pattern Is Rejected

```cpp
try {
  pluss::make_note({"x"}).regex().match("(");
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
regex().match: The expression contained mismatched ( and ).
```

## See Also

- [`NoteRegexAccessor::search`](search.md): the unanchored version; it matches
  anywhere in the cell.
- [`NoteRegexAccessor::findall`](findall.md) /
  [`NoteRegexAccessor::replace`](replace.md): extract or rewrite matches.
- [`Note::is_digit`](../note/is_digit.md) and the `is_*` family: fixed
  character-class checks with no regex engine.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="findall.html">
    <span class="label">Previous</span>
    <span class="title">findall</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>
