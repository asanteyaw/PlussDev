<!-- pluss-hand-authored -->
# Note::remove_suffix

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::remove_suffix(std::string_view suffix) const
```

## Strip A Trailing Suffix When Present

`remove_suffix(suffix)` returns a new `Note`, same shape as the input, with
one trailing occurrence of the literal string `suffix` removed from each cell
that ends with it. Cells that do not end with `suffix` are left unchanged.
The classic use is dropping a file extension.

The match is a **literal, exact suffix check** — only when the cell's last
`suffix.size()` bytes equal `suffix` is anything removed, and only **one**
copy is removed. It is the near inverse of
[`ensure_suffix`](ensure_suffix.md), and removes only from the end; use
[`remove`](remove.md) or [`replace`](replace.md) for a match anywhere.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: Drop An Extension

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"a.md", "a.md.md", "a.txt"}).remove_suffix(".md"))
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["a", "a.md", "a.txt"]
```

`"a.md.md"` loses only the final `.md`; `"a.txt"` does not end in `.md` and
is unchanged.

## Example 2: Absent Suffix Leaves The Cell Alone

```cpp
const pluss::Note n = pluss::make_note({"report"}).remove_suffix(".md");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["report"]
```

## See Also

- [`Note::remove_prefix`](remove_prefix.md): the leading-end counterpart.
- [`Note::ensure_suffix`](ensure_suffix.md): add a suffix if missing.
- [`Note::ends_with`](ends_with.md): test for the suffix first.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_punctuation.html">
    <span class="label">Previous</span>
    <span class="title">remove_punctuation</span>
  </a>
  <a class="next-link" href="remove_symbols.html">
    <span class="label">Next</span>
    <span class="title">remove_symbols</span>
  </a>
</div>
