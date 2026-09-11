<!-- pluss-hand-authored -->
# Note::deduplicate

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
Note Note::deduplicate() const
Note Note::deduplicate(std::int64_t dim) const
```

## Keep The First Occurrence Of Each Value Or Slice

`deduplicate()` removes repeated cells from the flattened row-major sequence
while preserving the first-seen order of the survivors. `deduplicate(dim)`
instead compares complete slices along one axis, keeps the first occurrence of
each slice, and preserves rank plus all unaffected extents.

This is not an alias for [`unique()`](unique.md). Both use exact byte equality,
but `unique` sorts its survivors whereas `deduplicate` preserves encounter
order. Choose `deduplicate` when order itself carries meaning, such as a
first-observed vocabulary or an ordered sequence of records.

On a matrix, axis zero means whole rows and axis one means whole columns.
Negative axes normalize normally; `deduplicate(-1)` is axis-aware, while the
no-argument overload flattens. Equality is collision-safe and compares every
cell in a candidate slice. The result shares cell storage, supports text and
bytes mode, and allocates only result metadata and distinctness bookkeeping.

Undefined input remains undefined. Invalid axes reject. Empty-slice behavior
matches set identity: several structurally empty slices are equal and reduce
to their first representative.

## Example 1: Flatten And Preserve First-Seen Order

```cpp
const auto rows = pluss::note::make(
    std::vector<std::string>{"b", "2", "a", "1", "b", "2"},
    std::array<std::int64_t, 2>{3, 2});
std::cout << "flattened\n"
          << pluss::note::print(rows.deduplicate()) << '\n';
```

### Output

```text
flattened
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["b", "2", "a", "1"]
```

The second `"b"` and `"2"` disappear without reordering the first four cells.

## Example 2: Keep First-Seen Whole Rows

```cpp
const auto rows = pluss::note::make(
    std::vector<std::string>{"b", "2", "a", "1", "b", "2"},
    std::array<std::int64_t, 2>{3, 2});
std::cout << "whole rows\n"
          << pluss::note::print(rows.deduplicate(0)) << '\n';
```

### Output

```text
whole rows
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["b", "2"], 
  ["a", "1"]
]
```

## Example 3: Keep First-Seen Whole Columns

```cpp
const auto columns = pluss::note::make(
    std::vector<std::string>{"b", "a", "b", "2", "1", "2"},
    std::array<std::int64_t, 2>{2, 3});
std::cout << "whole columns\n"
          << pluss::note::print(columns.deduplicate(-1)) << '\n';
```

### Output

```text
whole columns
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["b", "a"], 
  ["2", "1"]
]
```

The first and third columns match, so the first is retained before the middle
column.

## Example 4: Structurally Empty Slices Are Equal

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 3>{2, 3, 0});
std::cout << "three equal empty slices\n"
          << pluss::note::print(empty.deduplicate(1)) << '\n';
```

### Output

```text
three equal empty slices
Note(shape=[2, 1, 0], dim=3, numel=0, encoding=utf8)
[]
```

## Example 5: Bytes Mode Retains Encounter Order

```cpp
const auto raw = pluss::note::bytes(
    std::vector<std::string>{"b", "a", "b"},
    std::array<std::int64_t, 1>{3});
std::cout << "raw bytes\n"
          << pluss::note::print(raw.deduplicate()) << '\n';
```

### Output

```text
raw bytes
Note(shape=[2], dim=1, numel=2, encoding=bytes)
[b"b", b"a"]
```

## Example 6: Undefined Propagates

```cpp
std::cout << std::boolalpha << "undefined deduplicate defined="
          << pluss::Note::undefined().deduplicate().defined() << '\n';
```

### Output

```text
undefined deduplicate defined=false
```

## Example 7: Invalid Axes Reject

```cpp
const auto rows = pluss::note::make(
    std::vector<std::string>{"b", "2", "a", "1", "b", "2"},
    std::array<std::int64_t, 2>{3, 2});
try { (void)rows.deduplicate(2); }
catch (const pluss::Error&) {
  std::cout << "deduplicate axis 2: rejected\n";
}
```

### Output

```text
deduplicate axis 2: rejected
```

## See Also

- [`unique`](unique.md): sort distinct values or slices.
- [`sort`](sort.md): order lanes without removing repetitions.
- [`vector`](vector.md): flatten to borrowed views without removing repetitions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dedent.html"><span class="label">Previous</span><span class="title">dedent</span></a>
  <a class="next-link" href="defined.html"><span class="label">Next</span><span class="title">defined</span></a>
</div>
