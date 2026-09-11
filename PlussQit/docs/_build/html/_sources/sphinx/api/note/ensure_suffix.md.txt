<!-- pluss-hand-authored -->
# Note::ensure_suffix

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::ensure_suffix(std::string_view suffix) const
```

## Guarantee Each Cell Ends With A Suffix

`ensure_suffix(suffix)` returns a new `Note`, same shape as the input, in
which every cell is guaranteed to end with the literal string `suffix`. A
cell that already ends with the full `suffix` is left unchanged; any other
cell has `suffix` appended. Use it to normalize a trailing separator (a
directory `/`), a required extension, or a line terminator.

The check is a **literal, exact suffix match** — only a cell whose last
`suffix.size()` bytes equal `suffix` counts as already-suffixed; a cell
ending in a partial match still gets the whole `suffix` appended.
`ensure_suffix` never removes anything and is not the inverse of
[`remove_suffix`](remove_suffix.md). It is idempotent, shape-preserving, and
returns a fresh `Note`.

## Example 1: Add A Trailing Slash Where Missing

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"dir", "dir/", "d"}).ensure_suffix("/"))
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["dir/", "dir/", "d/"]
```

## Example 2: Shape Is Preserved

```cpp
const pluss::Note n =
    pluss::make_note({{"a", "b.txt"}}).ensure_suffix(".txt");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1, 2], dim=2, numel=2, encoding=utf8)
[
  ["a.txt", "b.txt"]
]
```

`"b.txt"` already has the extension; only `"a"` gains one.

## See Also

- [`Note::ensure_prefix`](ensure_prefix.md): the leading-end counterpart.
- [`Note::remove_suffix`](remove_suffix.md): strip a suffix if present.
- [`Note::ends_with`](ends_with.md): test for a suffix without adding one.
- [`Note::surround`](surround.md): add a prefix and a suffix unconditionally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ensure_prefix.html">
    <span class="label">Previous</span>
    <span class="title">ensure_prefix</span>
  </a>
  <a class="next-link" href="flatten.html">
    <span class="label">Next</span>
    <span class="title">flatten</span>
  </a>
</div>
