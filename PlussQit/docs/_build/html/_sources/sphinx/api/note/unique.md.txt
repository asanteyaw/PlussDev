<!-- pluss-hand-authored -->
# Note::unique

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
Note Note::unique() const
Note Note::unique(std::int64_t dim) const
```

## Return Sorted Distinct Values Or Slices

The two overloads answer related but different questions. `unique()` flattens
all cells, removes repeated byte-identical values, sorts the survivors, and
returns a rank-one Note. `unique(dim)` treats each complete slice along `dim`
as one record, removes duplicate slices, sorts those slices lexicographically,
and preserves the receiver's rank and every other extent.

For a matrix, `unique(0)` compares whole rows and `unique(1)` compares whole
columns. Two slices are equal only when every corresponding cell contains the
same bytes. Slice ordering compares their cells in row-major sequence until
the first difference. An explicit negative axis is normalized, so
`unique(-1)` means distinct slices along the last axis; unlike the no-argument
form, it does **not** flatten.

The result shares the packed text arena and creates new span metadata, avoiding
cell-byte copies. Text and bytes modes are both supported and preserved.
Undefined input remains undefined. `unique()` on a scalar returns a rank-one
singleton, while `unique(dim)` on a scalar rejects because a scalar has no
axis.

Empty slices have precise set semantics. If the selected axis has entries but
another extent makes every slice empty, all those slices compare equal and one
survives. If the selected axis itself has length zero, no slice survives and
the original zero extent remains zero.

## Example 1: Flatten, Sort, And Remove Repeats

```cpp
const auto rows = pluss::note::make(
    std::vector<std::string>{"b", "2", "a", "1", "b", "2"},
    std::array<std::int64_t, 2>{3, 2});
std::cout << "flattened\n" << pluss::note::print(rows.unique()) << '\n';
```

### Output

```text
flattened
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["1", "2", "a", "b"]
```

The matrix shape is intentionally gone in this overload.

## Example 2: Compare Whole Rows

```cpp
const auto rows = pluss::note::make(
    std::vector<std::string>{"b", "2", "a", "1", "b", "2"},
    std::array<std::int64_t, 2>{3, 2});
std::cout << "whole rows\n" << pluss::note::print(rows.unique(0)) << '\n';
```

### Output

```text
whole rows
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a", "1"], 
  ["b", "2"]
]
```

The repeated row `["b", "2"]` appears once, after the lexicographically
smaller row.

## Example 3: Compare Whole Columns With `-1`

```cpp
const auto columns = pluss::note::make(
    std::vector<std::string>{"b", "a", "b", "2", "1", "2"},
    std::array<std::int64_t, 2>{2, 3});
std::cout << "whole columns\n"
          << pluss::note::print(columns.unique(-1)) << '\n';
```

### Output

```text
whole columns
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a", "b"], 
  ["1", "2"]
]
```

Columns `["b", "2"]`, `["a", "1"]`, `["b", "2"]` become the two sorted
columns `["a", "1"]`, `["b", "2"]`.

## Example 4: Equal Empty Slices Collapse To One

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 3>{2, 3, 0});
std::cout << "three equal empty slices\n"
          << pluss::note::print(empty.unique(1)) << '\n';
```

### Output

```text
three equal empty slices
Note(shape=[2, 1, 0], dim=3, numel=0, encoding=utf8)
[]
```

## Example 5: An Empty Selected Axis Stays Empty

```cpp
const auto empty_axis = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 2>{0, 2});
std::cout << "empty selected axis\n"
          << pluss::note::print(empty_axis.unique(0)) << '\n';
```

### Output

```text
empty selected axis
Note(shape=[0, 2], dim=2, numel=0, encoding=utf8)
[]
```

## Example 6: Scalar Flattening Adds A Rank-One Axis

```cpp
std::cout << "scalar flattened\n"
          << pluss::note::print(pluss::note::make("x").unique()) << '\n';
```

### Output

```text
scalar flattened
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["x"]
```

## Example 7: Bytes Mode Is Preserved

```cpp
const auto raw = pluss::note::bytes(
    std::vector<std::string>{"b", "a", "b"},
    std::array<std::int64_t, 1>{3});
std::cout << "raw bytes\n" << pluss::note::print(raw.unique()) << '\n';
```

### Output

```text
raw bytes
Note(shape=[2], dim=1, numel=2, encoding=bytes)
[b"a", b"b"]
```

## Example 8: Absence Propagates But A Scalar Axis Rejects

```cpp
try { (void)pluss::note::make("x").unique(0); }
catch (const pluss::Error&) { std::cout << "unique scalar axis: rejected\n"; }
std::cout << std::boolalpha << "undefined unique defined="
          << pluss::Note::undefined().unique().defined() << '\n';
```

### Output

```text
unique scalar axis: rejected
undefined unique defined=false
```

## See Also

- [`deduplicate`](deduplicate.md): retain first-seen order instead of sorting.
- [`sort`](sort.md): order lanes without removing repeated cells.
- [`flatten`](flatten.md): flatten while retaining every cell.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transpose.html"><span class="label">Previous</span><span class="title">transpose</span></a>
  <a class="next-link" href="unquote.html"><span class="label">Next</span><span class="title">unquote</span></a>
</div>
