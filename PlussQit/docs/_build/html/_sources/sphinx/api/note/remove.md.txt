<!-- pluss-hand-authored -->
# Note::remove

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::remove(std::string_view needle) const
```

## Delete The First Occurrence Of A Literal Substring

`remove(needle)` returns a new `Note`, same shape as the input, with the
**first** occurrence of the literal string `needle` deleted from each cell.
It is the first-only sibling of [`remove_all`](remove_all.md), and the
delete-form of [`replace_first`](replace_first.md) (`replace_first(needle,
"")`).

Use it when a marker is expected once — a leading tag that survived an
earlier step, one delimiter you want gone, a single boilerplate fragment.
The match is literal (no regex, no case folding); only the leftmost match is
removed, and the bytes around it are not otherwise touched, so a mid-string
removal can leave a double space.

A cell that does not contain `needle` is copied through unchanged. The
operation is shape-preserving, cell-independent, and returns a fresh `Note`.

## Example 1: Only The First Match Goes

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"alpha beta beta", "beta", "none"}).remove("beta"))
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["alpha  beta", "", "none"]
```

The first `beta` in cell 0 is deleted (leaving two spaces); the second
remains. `"none"` has no match and is unchanged.

## Example 2: Contrast With `remove_all`

```cpp
const pluss::Note n = pluss::make_note({"a-b-c"}).remove("-");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["ab-c"]
```

Only the first `-` is removed; [`remove_all("-")`](remove_all.md) would give
`"abc"`.

## See Also

- [`Note::remove_all`](remove_all.md): delete **every** occurrence.
- [`Note::replace_first`](replace_first.md): the general form — substitute
  instead of delete.
- [`Note::remove_prefix`](remove_prefix.md) /
  [`Note::remove_suffix`](remove_suffix.md): delete at a boundary only.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="regex.html">
    <span class="label">Previous</span>
    <span class="title">regex</span>
  </a>
  <a class="next-link" href="remove_all.html">
    <span class="label">Next</span>
    <span class="title">remove_all</span>
  </a>
</div>
