<!-- pluss-hand-authored -->
# Note::lstrip

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
Note Note::lstrip() const
Note Note::lstrip(std::string_view chars) const
```

## Trim Characters From The Start Of Every Cell

`lstrip()` returns a new `Note`, same shape as the input, with leading
whitespace removed from each cell; `lstrip(chars)` instead removes any
character that appears in the `chars` set from the start. It is the
left-only half of [`strip`](strip.md) — use it when trailing content must be
preserved, for example a fixed-width field padded only on the left.

The no-argument form trims the ASCII whitespace bytes space, `\t`, `\n`,
`\r`, `\f`, `\v`. `lstrip(chars)` treats `chars` as a **set of bytes**:
every position at the start whose byte is in the set is removed, and
trimming stops at the first byte that is not. Pass only ASCII characters in
`chars`. A cell made entirely of trimmable characters becomes `""`.

`lstrip` is shape-preserving, cell-independent, and returns a fresh `Note`.

## Example 1: Trim Leading Whitespace

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"  hello  ", "\tworld\t", "clean"}).lstrip())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["hello  ", "world\t", "clean"]
```

Only the left side is trimmed; the trailing spaces and tab remain.

## Example 2: Trim A Character Set From The Start

```cpp
const pluss::Note n = pluss::make_note({"xxhello", "--world"}).lstrip("x-");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["hello", "world"]
```

## See Also

- [`Note::rstrip`](rstrip.md): the right-only counterpart.
- [`Note::strip`](strip.md): both ends.
- [`Note::remove_prefix`](remove_prefix.md): remove one occurrence of a
  fixed string, not a character set.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lower.html">
    <span class="label">Previous</span>
    <span class="title">lower</span>
  </a>
  <a class="next-link" href="matrix.html">
    <span class="label">Next</span>
    <span class="title">matrix</span>
  </a>
</div>
