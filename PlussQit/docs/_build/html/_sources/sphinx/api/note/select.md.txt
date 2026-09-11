<!-- pluss-hand-authored -->
# Note::select

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::select(std::int64_t dim, std::int64_t index) const
```

## Take One Slab Along An Axis And Drop That Axis

`select(dim, index)` returns the sub-`Note` at position `index` along axis
`dim`, with that axis **removed** — so the result has one lower rank than the
input. Selecting row 1 of a `2 × 3` grid gives a rank-1 `Note` of three
cells; selecting an element of a rank-1 `Note` gives a 0-D scalar. Reach for
it to pull a single row, column, or plane out of a grid by its index.

Nothing about the text changes: the picked cells are shared with the source,
the [bytes-vs-UTF-8](is_bytes.md) mode carries over, and the cost is one
`O(result numel)` copy of the offset table.

### The arguments

| Argument | Meaning |
|----------|---------|
| `dim` | Which axis to index. `0` is the outermost. A negative value counts from the back: `-1` is the last axis. After that adjustment it must be in `[0, dim())`, otherwise `dim out of range`. |
| `index` | The position along `dim` to keep. A negative value counts from the end of that axis: `-1` is its last slab. After adjustment it must be in `[0, size(dim))`, otherwise `select index out of range`. |

`select` needs at least one axis: on a 0-D scalar it raises `dim out of
range`. To pick cells from a rank-1 `Note` by an arbitrary list of positions
rather than one, use [`take`](take.md); to keep a contiguous *range* along an
axis without dropping it, use [`narrow`](narrow.md).

## Example 1: An Element Of A 1-D Note

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << v.select(0, 2) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"c"
```

The rank-1 `Note` becomes a 0-D scalar holding just `"c"`.

## Example 2: A Row And A Column Of A Grid

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.select(0, 1)) << "\n";   // row 1
std::cout << pluss::note::print(m.select(1, 0)) << "\n";   // column 0
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["d", "e", "f"]
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a", "d"]
```

`select(0, 1)` keeps axis 1 (the columns) and drops axis 0; `select(1, 0)`
keeps axis 0 (the rows) and drops axis 1.

## Example 3: Negative `dim` And Negative `index`

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.select(-1, 2)) << "\n";   // last axis, last column

const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << v.select(0, -1) << "\n";                       // last element
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["c", "f"]
Note(shape=[], dim=0, numel=1, encoding=utf8)
"f"
```

## Example 4: Dropping The Middle Axis Of A Rank-3 Note

```cpp
const auto t = pluss::make_note({"a", "b", "c", "d", "e", "f"}).reshape({1, 2, 3});
std::cout << pluss::note::print(t.select(1, 0)) << "\n";
```

### Output

```text
Note(shape=[1, 3], dim=2, numel=3, encoding=utf8)
[
  ["a", "b", "c"]
]
```

The `1 × 2 × 3` shape loses axis 1, leaving `1 × 3`.

## Example 5: Rejections

```cpp
auto first_line = [](const pluss::Error& e) {
  const std::string m = e.what();
  return m.substr(0, m.find('\n'));
};

try { (void)pluss::make_note({"a", "b"}).select(0, 5); }
catch (const pluss::Error& e) { std::cout << first_line(e) << "\n"; }

try { (void)pluss::make_note("x").select(0, 0); }
catch (const pluss::Error& e) { std::cout << first_line(e) << "\n"; }
```

### Output

```text
select index out of range
dim out of range
```

A 0-D scalar has no axis `0` to select along.

## See Also

- [`Note::narrow`](narrow.md): keep a contiguous range of slabs along an axis
  and **keep** the axis.
- [`Note::take`](take.md): gather cells by an arbitrary index list (rank-1
  result).
- [`Note::squeeze`](squeeze.md): drop a size-1 axis without indexing into it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scalar.html">
    <span class="label">Previous</span>
    <span class="title">scalar</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
