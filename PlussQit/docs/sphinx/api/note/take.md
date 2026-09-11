<!-- pluss-hand-authored -->
# Note::take

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::take(pluss::RangeView<std::int64_t> indices) const
```

`indices` accepts a braced list `{0, 2, 4}`, a `std::vector<std::int64_t>`, a
`std::array`, or a `std::span`.

## Gather Cells By An Index List

`take(indices)` treats the `Note` as its flat row-major sequence of cells and
returns a **rank-1** `Note` containing the cells at the given positions, in
the order listed. Unlike [`select`](select.md) and [`narrow`](narrow.md),
which take a contiguous slab along one axis, `take` picks arbitrary
positions — so it can also **reorder**, **repeat**, **filter**, or
**subsample** cells in a single call.

Nothing about the text changes: the picked cells are shared with the source,
the [bytes-vs-UTF-8](is_bytes.md) mode carries over, and the cost is
`O(len(indices))`.

### `indices`

- Each index addresses a cell in the **flattened** `Note`: for a `2 × 3`
  grid the valid range is `0 .. 5`, row-major (`(0,0) (0,1) (0,2) (1,0) …`).
  Input of any rank is accepted; the *result* is always rank 1.
- A **negative** index counts from the end: `-1` is the last cell. After that
  adjustment it must be in `[0, numel())`, otherwise `Note::take index out of
  range`.
- Indices may **repeat** and appear in **any order** — the result mirrors the
  list exactly.
- An **empty** `indices` returns a rank-1 `Note` of length 0.

## Example 1: Gather A Subset

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.take({0, 2, 4})) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["a", "c", "e"]
```

## Example 2: Negative Indices And Repeats

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.take({-1, -1, 0})) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["f", "f", "a"]
```

`-1` resolves to `"f"` and may be used more than once.

## Example 3: Reversing With A Full Index List

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.take({5, 4, 3, 2, 1, 0})) << "\n";
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["f", "e", "d", "c", "b", "a"]
```

## Example 4: Higher-Rank Input Is Flattened First

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.take({0, 3, 5})) << "\n";     // (0,0) (1,0) (1,2)

const auto t = pluss::make_note({"a", "b", "c", "d", "e", "f"}).reshape({1, 2, 3});
std::cout << pluss::note::print(t.take({1, 4})) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["a", "d", "f"]
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["b", "e"]
```

The `2 × 3` and `1 × 2 × 3` inputs are read row-major; the result is a plain
list regardless.

## Example 5: Scalar Input And An Empty List

```cpp
std::cout << pluss::note::print(pluss::make_note("solo").take({0, 0})) << "\n";

const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.take(std::vector<std::int64_t>{})) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["solo", "solo"]
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
```

A 0-D scalar has one cell at index `0`; an empty list yields an empty
rank-1 `Note`.

## Example 6: Out-Of-Range Index

```cpp
try {
  (void)pluss::make_note({"a", "b"}).take({5});
} catch (const pluss::Error& e) {
  const std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
Note::take index out of range
```

## See Also

- [`Note::select`](select.md): one slab along one axis, that axis dropped.
- [`Note::narrow`](narrow.md): a contiguous range along one axis, rank kept.
- [`Note::flatten`](flatten.md): the row-major reading order `take` indexes
  into.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="swapcase.html">
    <span class="label">Previous</span>
    <span class="title">swapcase</span>
  </a>
  <a class="next-link" href="tensor3.html">
    <span class="label">Next</span>
    <span class="title">tensor3</span>
  </a>
</div>
