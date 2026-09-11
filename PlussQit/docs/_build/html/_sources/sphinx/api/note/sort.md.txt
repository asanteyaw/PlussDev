<!-- pluss-hand-authored -->
# Note::sort

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
Note Note::sort() const
Note Note::sort(std::int64_t dim) const
```

## Order Each Lane Lexicographically

`sort()` returns a Note with cells ordered independently along one axis. The
no-argument form sorts the last axis, so `sort()` and `sort(-1)` are
equivalent. On a vector this orders the whole vector. On a matrix it sorts
each row; `sort(0)` instead sorts each column. Higher-rank Notes follow the
same lane rule while preserving all extents.

Comparison is lexicographic over exact stored bytes, not linguistic collation,
case folding, or numeric interpretation. Thus `"10"` precedes `"2"`, uppercase
and lowercase remain distinct, and UTF-8 text is ordered by its encoded bytes.
Normalize or transform cells first when a different ordering key is required.
Bytes-mode Notes use the same bytewise contract and retain bytes mode.

Sorting is stable: equal cell byte sequences retain their relative positions.
The method reuses the packed text arena and constructs new cell-span metadata;
it does not copy cell contents. Scalar and lanes of length zero or one are
already ordered. Undefined input remains undefined. An explicit axis accepts
negative normalization and rejects values outside `[-dim(), dim()-1]`.

## Example 1: Sort The Last Axis Of A Matrix

```cpp
const auto values = pluss::note::make(
    std::vector<std::string>{"b", "2", "a", "3", "b", "1"},
    std::array<std::int64_t, 2>{3, 2});
std::cout << "default last axis\n" << pluss::note::print(values.sort()) << '\n';
```

### Output

```text
default last axis
Note(shape=[3, 2], dim=2, numel=6, encoding=utf8)
[
  ["2", "b"], 
  ["3", "a"], 
  ["1", "b"]
]
```

Each two-cell row is sorted independently; rows themselves do not move.

## Example 2: Sort Down Axis Zero

```cpp
const auto values = pluss::note::make(
    std::vector<std::string>{"b", "2", "a", "3", "b", "1"},
    std::array<std::int64_t, 2>{3, 2});
std::cout << "axis zero\n" << pluss::note::print(values.sort(0)) << '\n';
```

### Output

```text
axis zero
Note(shape=[3, 2], dim=2, numel=6, encoding=utf8)
[
  ["a", "1"], 
  ["b", "2"], 
  ["b", "3"]
]
```

Here the left column `b,a,b` and right column `2,3,1` are separate lanes.

## Example 3: Use A Negative Axis

```cpp
const auto values = pluss::note::make(
    std::vector<std::string>{"b", "2", "a", "3", "b", "1"},
    std::array<std::int64_t, 2>{3, 2});
std::cout << std::boolalpha
          << "explicit -1 matches default="
          << (values.sort(-1).vector() == values.sort().vector()) << '\n';
```

### Output

```text
explicit -1 matches default=true
```

## Example 4: Scalar And Empty Geometry Are Unchanged

```cpp
std::cout << "scalar\n" << pluss::note::print(pluss::note::make("x").sort()) << '\n';
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
std::cout << "empty matrix\n" << pluss::note::print(empty.sort()) << '\n';
```

### Output

```text
scalar
Note(shape=[], dim=0, numel=1, encoding=utf8)
"x"
empty matrix
Note(shape=[2, 0], dim=2, numel=0, encoding=utf8)
[]
```

## Example 5: Sort Raw Bytes Without Transcoding

```cpp
const auto raw = pluss::note::bytes(
    std::vector<std::string>{"b", "a", "b"},
    std::array<std::int64_t, 1>{3});
std::cout << "raw bytes\n" << pluss::note::print(raw.sort()) << '\n';
```

### Output

```text
raw bytes
Note(shape=[3], dim=1, numel=3, encoding=bytes)
[b"a", b"b", b"b"]
```

## Example 6: Undefined Propagates

```cpp
std::cout << std::boolalpha << "undefined sort defined="
          << pluss::Note::undefined().sort().defined() << '\n';
```

### Output

```text
undefined sort defined=false
```

## Example 7: Invalid Axes Reject

```cpp
const auto values = pluss::note::make(
    std::vector<std::string>{"b", "2", "a", "3", "b", "1"},
    std::array<std::int64_t, 2>{3, 2});
try { (void)values.sort(2); }
catch (const pluss::Error&) { std::cout << "sort axis 2: rejected\n"; }
```

### Output

```text
sort axis 2: rejected
```

## See Also

- [`unique`](unique.md): emit sorted distinct values or slices.
- [`deduplicate`](deduplicate.md): retain first-seen distinct values or slices.
- `Note::argsort`: return ordering positions when cell positions are needed.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sizes_view.html"><span class="label">Previous</span><span class="title">sizes_view</span></a>
  <a class="next-link" href="split.html"><span class="label">Next</span><span class="title">split</span></a>
</div>
