<!-- pluss-hand-authored -->
# Note::transpose

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::transpose(std::int64_t dim0, std::int64_t dim1) const
```

## Swap Two Axes

`transpose()` returns a `Note` with axes `dim0` and `dim1` exchanged and all
other axes left where they are. On a `2 × 3` grid, `transpose(0, 1)` gives
the `3 × 2` grid whose rows are the original columns. It is the short form of
the common case that [`permute`](permute.md) covers in general — `a.transpose(i, j)`
is exactly `a.permute(p)` where `p` is the identity with `i` and `j` swapped.

Like `permute`, `transpose` **moves cells**: the offset table is rebuilt so
that a row-major read of the result walks the cells in transposed order. The
cell strings are shared with the source (no copy, no re-encode), the
[bytes-vs-UTF-8](is_bytes.md) mode carries over, and the cost is one
`O(numel)` pass.

### The argument contract

- Both `dim0` and `dim1` must be in `[0, dim())` — a value outside that range
  raises `transpose dim out of range`. There is **no negative-axis form**.
- `dim0 == dim1` is allowed and returns the `Note` unchanged.
- On a 1-D `Note` the only valid call is `transpose(0, 0)`; on a 0-D `Note`
  every call raises `transpose dim out of range` (there are no axes to
  address).

## Example 1: The 2-D Transpose

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.transpose(0, 1)) << "\n";
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

## Example 2: Swapping The Outer And Inner Axis Of A Rank-3 Note

```cpp
const auto t = pluss::make_note({"a", "b", "c", "d", "e", "f"}).reshape({1, 2, 3});
std::cout << pluss::note::print(t.transpose(0, 2)) << "\n";
```

### Output

```text
Note(shape=[3, 2, 1], dim=3, numel=6, encoding=utf8)
[
  [
    ["a"], 
    ["d"]
  ], 
  [
    ["b"], 
    ["e"]
  ], 
  [
    ["c"], 
    ["f"]
  ]
]
```

The middle axis (size 2) stays put; the size-1 and size-3 axes trade places.

## Example 3: No-Op Forms

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.transpose(1, 1)) << "\n";   // same axis twice

const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.transpose(0, 0)) << "\n";   // 1-D
```

### Output

```text
Note(shape=[2, 3], dim=2, numel=6, encoding=utf8)
[
  ["a", "b", "c"], 
  ["d", "e", "f"]
]
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["a", "b", "c", "d", "e", "f"]
```

## Example 4: Out-Of-Range Axis

```cpp
try {
  (void)pluss::make_note({"a", "b"}).transpose(0, 1);
} catch (const pluss::Error& e) {
  const std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
transpose dim out of range
```

The `Note` is rank 1, so axis `1` does not exist.

## See Also

- [`Note::permute`](permute.md): reorder three axes at once, or apply any
  permutation.
- [`Note::reshape`](reshape.md): regroup the same cells without moving them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transliterate_greek.html">
    <span class="label">Previous</span>
    <span class="title">transliterate_greek</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
