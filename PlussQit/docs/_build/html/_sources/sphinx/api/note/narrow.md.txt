<!-- pluss-hand-authored -->
# Note::narrow

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::narrow(std::int64_t dim, std::int64_t start, std::int64_t length) const
```

## Keep A Contiguous Slice Along One Axis

`narrow(dim, start, length)` returns a `Note` that keeps only positions
`[start, start + length)` along axis `dim`, with every other axis untouched.
The rank does not change — narrowing a `2 × 3` grid to `length` 2 columns
gives a `2 × 2` grid. Use it to take the first *n* rows, a window of columns,
or a middle span, without collapsing the axis the way [`select`](select.md)
does.

Nothing about the text changes: the kept cells are shared with the source,
the [bytes-vs-UTF-8](is_bytes.md) mode carries over, and the cost is one
`O(result numel)` copy of the offset table.

### The arguments

| Argument | Meaning |
|----------|---------|
| `dim` | Which axis to slice. `0` is the outermost; a negative value counts from the back (`-1` is the last axis). After that adjustment it must be in `[0, dim())`, otherwise `dim out of range`. |
| `start` | The first position to keep along `dim`. Must be in `[0, size(dim)]` — non-negative, and `size(dim)` itself is allowed (paired only with `length` 0). Otherwise `narrow start out of range`. |
| `length` | How many positions to keep. Must be `≥ 0` (`narrow length must be non-negative`), and `start + length` must not exceed `size(dim)` (`narrow range out of bounds`). |

`length` 0 is valid and yields an empty `Note` (that axis becomes size 0).
`start` 0 with `length` equal to `size(dim)` returns the `Note` unchanged.
There is no negative `start` and no step; for scattered or reordered
positions use [`take`](take.md).

## Example 1: A Window Of A 1-D Note

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.narrow(0, 1, 3)) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["b", "c", "d"]
```

Positions 1, 2, 3 — `start` is inclusive, `start + length` is exclusive.

## Example 2: Rows Versus Columns Of A Grid

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.narrow(0, 1, 1)) << "\n";   // one row, axis kept
std::cout << pluss::note::print(m.narrow(1, 0, 2)) << "\n";   // first two columns
```

### Output

```text
Note(shape=[1, 3], dim=2, numel=3, encoding=utf8)
[
  ["d", "e", "f"]
]
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a", "b"], 
  ["d", "e"]
]
```

`narrow(0, 1, 1)` keeps row 1 but stays rank 2 (`1 × 3`), unlike
`select(0, 1)` which would give a rank-1 `[3]`.

## Example 3: Negative `dim`

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.narrow(-1, 1, 2)) << "\n";   // last axis, columns 1..2
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["b", "c"], 
  ["e", "f"]
]
```

## Example 4: Zero Length And Full Range

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.narrow(0, 2, 0)) << "\n";   // empty
std::cout << pluss::note::print(v.narrow(0, 0, 6)) << "\n";   // unchanged
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["a", "b", "c", "d", "e", "f"]
```

## Example 5: Rejections

```cpp
auto first_line = [](const pluss::Error& e) {
  const std::string m = e.what();
  return m.substr(0, m.find('\n'));
};

try { (void)pluss::make_note({"a", "b", "c"}).narrow(0, 2, 2); }
catch (const pluss::Error& e) { std::cout << first_line(e) << "\n"; }

try { (void)pluss::make_note({"a", "b", "c"}).narrow(0, -1, 1); }
catch (const pluss::Error& e) { std::cout << first_line(e) << "\n"; }
```

### Output

```text
narrow range out of bounds
narrow start out of range
```

`start + length` = `4` exceeds the axis size `3`; and `start` may not be
negative.

## See Also

- [`Note::select`](select.md): pick one slab along an axis and drop the axis.
- [`Note::take`](take.md): gather arbitrary, possibly repeated or reordered
  positions.
- [`Note::split`](split.md): cut each *cell's text* into pieces (a different
  kind of slicing).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matrix.html">
    <span class="label">Previous</span>
    <span class="title">matrix</span>
  </a>
  <a class="next-link" href="nlp.html">
    <span class="label">Next</span>
    <span class="title">nlp</span>
  </a>
</div>
