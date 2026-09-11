<!-- pluss-hand-authored -->
# Note::replace_first

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::replace_first(std::string_view from, std::string_view to) const
```

## Substitute Only The First Occurrence

`replace_first(from, to)` returns a new `Note`, same shape as the input, with
the **leftmost** occurrence of the literal string `from` in each cell
rewritten to `to`. Later occurrences are left untouched. It is the bounded
sibling of [`replace`](replace.md) (which rewrites all) and the
substitute-form of [`remove`](remove.md).

Use it when a token is expected once, or when only the first hit is
meaningful — a leading label, the first field separator, the first of a
series you will handle one at a time.

### Edge cases

- An **empty `from`** is a no-op — the cell is returned unchanged.
- A `from` that does not occur leaves the cell unchanged.
- An empty `to` deletes the first `from`; that is exactly
  [`remove(from)`](remove.md).

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: First Match Only

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"alpha beta beta gamma", "beta", "no match"})
        .replace_first("beta", "X"))
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["alpha X beta gamma", "X", "no match"]
```

## Example 2: An Empty `from` Is A No-Op

```cpp
const pluss::Note n = pluss::make_note({"abc"}).replace_first("", "X");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["abc"]
```

## Example 3: No Match Leaves The Cell Unchanged

```cpp
pluss::note::print(std::cout, pluss::make_note({"abc"}).replace_first("z", "X"))
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["abc"]
```

## See Also

- [`Note::replace`](replace.md): rewrite **all** occurrences.
- [`Note::replace_last`](replace_last.md): rewrite only the rightmost.
- [`Note::replace_n`](replace_n.md): rewrite the first `n`.
- [`Note::remove`](remove.md): `replace_first(from, "")`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace.html">
    <span class="label">Previous</span>
    <span class="title">replace</span>
  </a>
  <a class="next-link" href="replace_last.html">
    <span class="label">Next</span>
    <span class="title">replace_last</span>
  </a>
</div>
