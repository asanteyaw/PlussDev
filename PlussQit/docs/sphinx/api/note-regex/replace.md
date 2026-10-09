<!-- pluss-hand-authored -->
# Note::RegexAccessor::replace

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
Note Note::RegexAccessor::replace(std::string_view pattern,
                                std::string_view replacement,
                                std::int64_t flags = 0) const
```

## Rewrite Regex Matches In Every Cell

`replace(pattern, replacement)` returns a new `Note`, same shape as the input,
with **every** non-overlapping match of `pattern` in each cell replaced by
`replacement`. It is the pattern-based counterpart of
[`Note::replace`](../note/replace.md): reach
for it when the target is a shape (a run of digits, a date, a bracketed span)
rather than a fixed string.

`pattern` is an ECMAScript regular expression. `replacement` is an ECMAScript
replacement string, so it may reference capture groups with `$1`, `$2`, …
(and `$&` for the whole match, `$$` for a literal `$`). Replacement is
global by default — there is no "first only" mode here; use
[`Note::replace_first`](../note/replace_first.md) for a literal first-only rewrite, or
anchor the pattern.

### `flags`

`flags` is a bitmask of match options; `flags <= 0` (the default) is plain
case-sensitive ECMAScript.

| Bit (value) | Effect |
| --- | --- |
| `1 << 0` (1) | case-insensitive matching |
| `1 << 1` (2) | multiline — `^` and `$` also match at line breaks |

### Failure

An invalid `pattern` throws `pluss::Error` before any cell is processed. The
operation is shape-preserving, cell-independent, and returns a fresh `Note`;
a cell with no match is copied through unchanged.

## Example 1: Replace Every Match

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"a1b22c333", "none"}).regex().replace(R"(\d+)", "#"))
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a#b#c#", "none"]
```

Each digit run — regardless of length — becomes a single `#`.

## Example 2: Capture-Group Back-References

```cpp
const pluss::Note n = pluss::make_note({"2026-08-02"}).regex().replace(
    R"((\d{4})-(\d{2})-(\d{2}))", "$3/$2/$1");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["02/08/2026"]
```

`$1`/`$2`/`$3` are the year, month, and day groups, reordered by `repl`.

## Example 3: Case-Insensitive Matching

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({"Hello HELLO hello"}).regex().replace("hello", "hi", 1))
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["hi hi hi"]
```

With `flags = 1` all three spellings match.

## Example 4: The Same Pattern Without Flags

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Hello HELLO hello"}).regex().replace("hello", "hi"))
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Hello HELLO hi"]
```

Case-sensitive: only the exact lowercase `hello` is rewritten.

## Example 5: An Invalid Pattern Is Rejected

```cpp
try {
  pluss::make_note({"x"}).regex().replace("(", "y");
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
regex().replace: The expression contained mismatched ( and ).
```

## See Also

- [`Note::replace`](../note/replace.md): literal (non-regex) global substitution.
- [`Note::RegexAccessor::findall`](findall.md): extract the matches instead of
  rewriting them.
- [`Note::RegexAccessor::split`](split.md): split each cell at the matches.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="match.html">
    <span class="label">Previous</span>
    <span class="title">match</span>
  </a>
  <a class="next-link" href="search.html">
    <span class="label">Next</span>
    <span class="title">search</span>
  </a>
</div>
