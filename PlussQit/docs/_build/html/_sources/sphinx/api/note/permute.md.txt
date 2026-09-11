<!-- pluss-hand-authored -->
# Note::permute

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::permute(pluss::RangeView<std::int64_t> dims) const
```

`dims` accepts a braced list `{1, 0}`, a `std::vector<std::int64_t>`, a
`std::array`, or a `std::span`.

## Reorder All Axes At Once

`permute()` returns a `Note` whose axes are the input's axes in the order
`dims` names: `dims[i]` is the input axis that becomes output axis `i`. For a
`2 × 3` grid, `permute({1, 0})` produces the `3 × 2` transpose; for a rank-3
`Note`, `permute({2, 0, 1})` is a full axis rotation that a single
[`transpose`](transpose.md) cannot express.

Unlike [`reshape`](reshape.md), `permute` **moves cells**: the offset table
is rebuilt so that reading the result in row-major order visits the cells in
their new arrangement. The cell strings themselves are still shared with the
source — no text is copied or re-encoded, and the
[bytes-vs-UTF-8](is_bytes.md) mode carries over — so the cost is one
`O(numel)` pass over the offset table.

### The `dims` contract

`dims` must be a **complete permutation** of `0 .. rank-1`:

- its length must equal [`dim()`](../../note_api_reference.md) — otherwise
  `permute expects dims.size() == dim`;
- every value must be in `[0, rank)` and appear exactly once — otherwise
  `permute dims must be a permutation`.

There is no negative-axis form and no partial permutation; name every axis.
On a 0-D or 1-D `Note` the only valid call is `permute({})` or
`permute({0})`, both of which return the `Note` unchanged.

## Example 1: The 2-D Transpose

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.permute({1, 0})) << "\n";
```

### Output

```text
Note(shape=[3, 2], dim=2, numel=6, encoding=utf8)
[
  ["a", "d"], 
  ["b", "e"], 
  ["c", "f"]
]
```

Output axis 0 is input axis 1 (the old columns), output axis 1 is input
axis 0 (the old rows).

## Example 2: The Identity Permutation

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.permute({0, 1})) << "\n";
```

### Output

```text
Note(shape=[2, 3], dim=2, numel=6, encoding=utf8)
[
  ["a", "b", "c"], 
  ["d", "e", "f"]
]
```

## Example 3: A Rank-3 Rotation

```cpp
const auto t = pluss::make_note({"a", "b", "c", "d", "e", "f"}).reshape({1, 2, 3});
std::cout << pluss::note::print(t.permute({2, 0, 1})) << "\n";
```

### Output

```text
Note(shape=[3, 1, 2], dim=3, numel=6, encoding=utf8)
[
  [
    ["a", "d"]
  ], 
  [
    ["b", "e"]
  ], 
  [
    ["c", "f"]
  ]
]
```

The `1 × 2 × 3` shape becomes `3 × 1 × 2`: old axis 2 (size 3) moves to the
front, old axes 0 and 1 follow.

## Example 4: Rejections

```cpp
auto first_line = [](const pluss::Error& e) {
  const std::string m = e.what();
  return m.substr(0, m.find('\n'));
};

try { (void)pluss::make_note({{"a", "b"}, {"c", "d"}}).permute({0, 0}); }
catch (const pluss::Error& e) { std::cout << first_line(e) << "\n"; }

try { (void)pluss::make_note({{"a", "b"}, {"c", "d"}}).permute({0}); }
catch (const pluss::Error& e) { std::cout << first_line(e) << "\n"; }
```

### Output

```text
permute dims must be a permutation
permute expects dims.size() == dim
```

## See Also

- [`Note::transpose`](transpose.md): swap just two axes — the common case,
  with a shorter call.
- [`Note::reshape`](reshape.md): regroup the same cells without reordering
  them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="owned.html">
    <span class="label">Previous</span>
    <span class="title">owned</span>
  </a>
  <a class="next-link" href="phonetic_metaphone.html">
    <span class="label">Next</span>
    <span class="title">phonetic_metaphone</span>
  </a>
</div>
