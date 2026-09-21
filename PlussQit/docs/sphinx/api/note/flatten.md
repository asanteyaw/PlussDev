<!-- pluss-hand-authored -->
# Note::flatten

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::flatten() const
```

## Collapse Every Axis Into One

`flatten()` returns a rank-1 `Note` containing the same cells in the same
row-major order. It is [`reshape`](reshape.md)`({numel()})` with nothing to
compute — the shortcut you reach for before an operation that expects a flat
list, or when you no longer care about the grid structure.

No text is touched: cells are shared with the source, the
[bytes-vs-UTF-8](is_bytes.md) mode carries over, and only the shape metadata
changes.

### Behaviour by rank

| Input | Result |
|-------|--------|
| rank 2 or 3 | rank 1, `[numel()]` |
| rank 1 | returned unchanged |
| rank 0 (scalar) | rank 1, `[1]` |

An empty `Note` flattens to a rank-1 `Note` of length `0`.

## Example 1: Grid To List

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.flatten()) << "\n";
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["a", "b", "c", "d", "e", "f"]
```

Row 0 then row 1 — the same order the cells are stored in.

## Example 2: Already Rank 1

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.flatten()) << "\n";
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["a", "b", "c", "d", "e", "f"]
```

## Example 3: A Scalar Becomes A One-Element List

```cpp
std::cout << pluss::note::print(pluss::make_note("solo").flatten()) << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["solo"]
```

## See Also

- [`Note::reshape`](reshape.md): the general form — flatten is the rank-1
  case.
- [`Note::squeeze`](squeeze.md): drop only the size-1 axes, keeping the rest
  of the grid.
- `Note::join`: collapse the cells into a *single string* rather than a flat
  list.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ensure_suffix.html">
    <span class="label">Previous</span>
    <span class="title">ensure_suffix</span>
  </a>
  <a class="next-link" href="float_item.html">
    <span class="label">Next</span>
    <span class="title">float_item</span>
  </a>
</div>
