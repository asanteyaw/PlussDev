<!-- pluss-hand-authored -->
# Note::replace_last

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::replace_last(std::string_view from, std::string_view to) const
```

## Substitute Only The Last Occurrence

`replace_last(from, to)` returns a new `Note`, same shape as the input, with
the **rightmost** occurrence of the literal string `from` in each cell
rewritten to `to`. Earlier occurrences are left untouched. It is the mirror
of [`replace_first`](replace_first.md).

Use it when the meaningful hit is the trailing one — the final path segment,
the last delimiter before a value, the closing marker.

### Edge cases

- An **empty `from`** is a no-op.
- A `from` that does not occur leaves the cell unchanged.
- An empty `to` deletes the last `from`.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: Last Match Only

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"alpha beta beta gamma", "beta", "no match"})
        .replace_last("beta", "X"))
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["alpha beta X gamma", "X", "no match"]
```

The second `beta` in cell 0 is rewritten; the first stays. When there is only
one occurrence (cell 1), first and last coincide.

## See Also

- [`Note::replace_first`](replace_first.md): rewrite only the leftmost.
- [`Note::replace`](replace.md): rewrite **all** occurrences.
- [`Note::replace_n`](replace_n.md): rewrite the first `n`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace_first.html">
    <span class="label">Previous</span>
    <span class="title">replace_first</span>
  </a>
  <a class="next-link" href="replace_n.html">
    <span class="label">Next</span>
    <span class="title">replace_n</span>
  </a>
</div>
