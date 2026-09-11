<!-- pluss-hand-authored -->
# Note::replace_n

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::replace_n(std::string_view from, std::string_view to,
                     std::int64_t n) const
```

## Substitute Up To The First `n` Occurrences

`replace_n(from, to, n)` returns a new `Note`, same shape as the input, with
the first `n` occurrences of the literal string `from` in each cell rewritten
to `to`, scanning left to right. Occurrences past the `n`-th are left
untouched. It generalizes [`replace_first`](replace_first.md) (`n == 1`) and
[`replace`](replace.md) (`n` large enough to cover every hit).

### `n`

- `n <= 0` — a no-op; the cell is returned unchanged.
- `n` greater than the number of occurrences — every occurrence is
  rewritten, same as [`replace`](replace.md).
- Otherwise — exactly the first `n`, non-overlapping, are rewritten.

An empty `from` is a no-op regardless of `n`. The operation is
shape-preserving, cell-independent, and returns a fresh `Note`.

## Example 1: The First Two

```cpp
const auto n = pluss::make_note({"alpha beta beta beta", "beta"});
std::cout << pluss::note::print(n.replace_n("beta", "X", 2)) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["alpha X X beta", "X"]
```

Two of the three `beta`s in cell 0 are rewritten; cell 1 has only one and it
is rewritten.

## Example 2: `n` Of Zero Is A No-Op

```cpp
const auto n = pluss::make_note({"alpha beta beta beta", "beta"});
std::cout << pluss::note::print(n.replace_n("beta", "X", 0)) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["alpha beta beta beta", "beta"]
```

## Example 3: `n` Above The Count Rewrites Everything

```cpp
const auto n = pluss::make_note({"alpha beta beta beta", "beta"});
std::cout << pluss::note::print(n.replace_n("beta", "X", 99)) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["alpha X X X", "X"]
```

## See Also

- [`Note::replace_first`](replace_first.md): `replace_n(from, to, 1)`.
- [`Note::replace`](replace.md): rewrite every occurrence.
- [`Note::replace_last`](replace_last.md): rewrite only the rightmost.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace_last.html">
    <span class="label">Previous</span>
    <span class="title">replace_last</span>
  </a>
  <a class="next-link" href="reshape.html">
    <span class="label">Next</span>
    <span class="title">reshape</span>
  </a>
</div>
