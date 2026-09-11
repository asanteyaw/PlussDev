<!-- pluss-hand-authored -->
# Note::rstrip

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
Note Note::rstrip() const
Note Note::rstrip(std::string_view chars) const
```

## Trim Characters From The End Of Every Cell

`rstrip()` returns a new `Note`, same shape as the input, with trailing
whitespace removed from each cell; `rstrip(chars)` instead removes any
character that appears in the `chars` set from the end. It is the right-only
half of [`strip`](strip.md) — use it to drop a trailing newline, a padding
run, or a stray delimiter while leaving the start of the cell intact.

The no-argument form trims the ASCII whitespace bytes space, `\t`, `\n`,
`\r`, `\f`, `\v`. `rstrip(chars)` treats `chars` as a **set of bytes**:
positions at the end whose byte is in the set are removed until a byte that
is not is reached. Pass only ASCII characters in `chars`. A cell made
entirely of trimmable characters becomes `""`.

`rstrip` is shape-preserving, cell-independent, and returns a fresh `Note`.

## Example 1: Trim Trailing Whitespace

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"  hello  ", "\tworld\t", "clean"}).rstrip())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["  hello", "\tworld", "clean"]
```

The leading whitespace is kept; only the right side is trimmed.

## See Also

- [`Note::lstrip`](lstrip.md): the left-only counterpart.
- [`Note::strip`](strip.md): both ends.
- [`Note::remove_suffix`](remove_suffix.md): remove one occurrence of a
  fixed string, not a character set.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reshape.html">
    <span class="label">Previous</span>
    <span class="title">reshape</span>
  </a>
  <a class="next-link" href="scalar.html">
    <span class="label">Next</span>
    <span class="title">scalar</span>
  </a>
</div>
