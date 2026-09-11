<!-- pluss-hand-authored -->
# Note::reshape

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::reshape(pluss::RangeView<std::int64_t> sizes) const
```

`sizes` accepts any contiguous integer range — a braced list `{2, 3}`, a
`std::vector<std::int64_t>`, a `std::array`, or a `std::span`.

## Reinterpret The Same Cells Under A New Shape

`reshape()` returns a `Note` that holds **exactly the same cells in the same
row-major order** but presents them with a different rank and axis sizes. It
is the tool for turning a flat list of strings into a grid, collapsing a grid
back to a list, or adding a leading batch axis before a shape-sensitive
operation.

Nothing about the text is touched: no cell is copied, concatenated, or
re-encoded, and the [bytes-vs-UTF-8](is_bytes.md) mode carries over. Only the
shape metadata changes, so the call is O(`numel`) at most (it duplicates the
small offset table) and never does per-string work.

### The `sizes` contract

- **Rank 0 to 3.** A four-or-more-element `sizes` raises `Note supports up to
  3D`. An empty `sizes` (`{}`) targets a 0-D scalar and requires
  `numel() == 1`.
- **The product must equal [`numel`](../../note_api_reference.md).** A
  mismatch raises `reshape numel mismatch`.
- **One dimension may be `-1`**, and is inferred as
  `numel() / (product of the other dimensions)`. Two `-1`s raise `reshape
  allows at most one inferred (-1) dimension`; a `-1` whose partner product
  does not divide `numel()` raises `reshape cannot infer the -1 dimension:
  total element count is not divisible by the other dimensions`.
- Every other dimension must be `≥ 0`; a `0` is allowed and yields an empty
  `Note`.

### Row-major reinterpretation

Cells are stored last-axis-fastest. `reshape` walks that same linear sequence
and re-groups it: `["a","b","c","d","e","f"]` reshaped to `{2, 3}` fills row
0 with `a b c` and row 1 with `d e f`. It does **not** transpose — for that
use [`transpose`](transpose.md) or [`permute`](permute.md).

## Example 1: List To Grid

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.reshape({2, 3})) << "\n";
```

### Output

```text
Note(shape=[2, 3], dim=2, numel=6, encoding=utf8)
[
  ["a", "b", "c"], 
  ["d", "e", "f"]
]
```

## Example 2: Adding A Leading Axis

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.reshape({1, 2, 3})) << "\n";
```

### Output

```text
Note(shape=[1, 2, 3], dim=3, numel=6, encoding=utf8)
[
  [
    ["a", "b", "c"], 
    ["d", "e", "f"]
  ]
]
```

## Example 3: Regrouping A Grid

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.reshape({3, 2})) << "\n";
```

### Output

```text
Note(shape=[3, 2], dim=2, numel=6, encoding=utf8)
[
  ["a", "b"], 
  ["c", "d"], 
  ["e", "f"]
]
```

The `2 × 3` grid and the `3 × 2` grid are the same six cells read in the same
order — only the row boundaries move.

## Example 4: Inferring A Dimension With `-1`

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.reshape({-1, 3})) << "\n";   // rows inferred as 2
std::cout << pluss::note::print(v.reshape({2, -1})) << "\n";   // cols inferred as 3
```

### Output

```text
Note(shape=[2, 3], dim=2, numel=6, encoding=utf8)
[
  ["a", "b", "c"], 
  ["d", "e", "f"]
]
Note(shape=[2, 3], dim=2, numel=6, encoding=utf8)
[
  ["a", "b", "c"], 
  ["d", "e", "f"]
]
```

## Example 5: Reshaping To A 0-D Scalar

```cpp
const auto s = pluss::make_note("solo");
std::cout << s.reshape({1}).reshape(std::vector<std::int64_t>{}) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"solo"
```

A one-element `Note` round-trips between rank 0 and rank 1 freely.

## Example 6: Rejections

```cpp
auto first_line = [](const pluss::Error& e) {
  const std::string m = e.what();
  return m.substr(0, m.find('\n'));
};

try { (void)pluss::make_note({"a", "b", "c"}).reshape({2, 2}); }
catch (const pluss::Error& e) { std::cout << first_line(e) << "\n"; }

try { (void)pluss::make_note({"a", "b", "c", "d"}).reshape({-1, -1}); }
catch (const pluss::Error& e) { std::cout << first_line(e) << "\n"; }

try { (void)pluss::make_note({"a", "b", "c", "d", "e"}).reshape({-1, 2}); }
catch (const pluss::Error& e) { std::cout << first_line(e) << "\n"; }

try { (void)pluss::make_note({"a", "b"}).reshape({1, 1, 1, 2}); }
catch (const pluss::Error& e) { std::cout << first_line(e) << "\n"; }
```

### Output

```text
reshape numel mismatch
reshape allows at most one inferred (-1) dimension
reshape cannot infer the -1 dimension: total element count is not divisible by the other dimensions
Note supports up to 3D
```

## See Also

- [`Note::flatten`](flatten.md): the common case — reshape to rank 1.
- [`Note::squeeze`](squeeze.md) / [`Note::unsqueeze`](unsqueeze.md): drop or
  add a single size-1 axis without restating the whole shape.
- [`Note::transpose`](transpose.md) / [`Note::permute`](permute.md): reorder
  axes (which *does* move cells) rather than regroup them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace_n.html">
    <span class="label">Previous</span>
    <span class="title">replace_n</span>
  </a>
  <a class="next-link" href="rstrip.html">
    <span class="label">Next</span>
    <span class="title">rstrip</span>
  </a>
</div>
