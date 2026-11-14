<!-- pluss-hand-authored -->
# Note::join

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
Note Note::join() const
Note Note::join(std::string_view separator) const
```

## Reduce Text Cells To One Text Cell

`join()` concatenates every cell in row-major order and returns a rank-zero,
one-cell UTF-8 `Note`. The overload without an argument uses an empty
separator; it is equivalent to `join("")`, not to [`join_flat()`](join_flat.md),
whose default separator is one space. Supply a separator when cell boundaries
must remain visible in the result.

The operation reduces the entire Note, not one axis. A matrix is read across
its first row, then its second, and a tensor follows the same flattened order.
The original shape is not encoded into the joined text. Use row selection or
another structural operation first when separate per-row reductions are
required.

Separators are inserted **between cells**, including empty cells. Three cells
`["", "x", ""]` joined by `"|"` therefore produce `"|x|"`. A Note with zero
cells produces one empty scalar cell because there are no values or boundaries
to append. A scalar returns a newly constructed scalar with the same text.

`join()` is a text operation and rejects a bytes-mode Note. It allocates one
packed result and copies every input byte plus
`separator.size() * max(numel() - 1, 0)` separator bytes. An undefined receiver
propagates as an undefined `Note`, matching shape-preserving Note
transformations; check `defined()` when absence must be rejected by the caller.

## Example 1: Join With A Visible Separator

```cpp
const auto words = pluss::note::make({"red", "green", "blue"});
std::cout << "custom separator\n"
          << pluss::note::print(words.join(" / ")) << '\n';
```

### Output

```text
custom separator
Note(shape=[], dim=0, numel=1, encoding=utf8)
"red / green / blue"
```

## Example 2: Omit The Separator

```cpp
const auto words = pluss::note::make({"red", "green", "blue"});
std::cout << "empty separator\n"
          << pluss::note::print(words.join()) << '\n';
```

### Output

```text
empty separator
Note(shape=[], dim=0, numel=1, encoding=utf8)
"redgreenblue"
```

## Example 3: Matrix Cells Use Row-Major Order

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << "matrix=" << grid.join("/").scalar() << '\n';
```

### Output

```text
matrix=a/b/c/d
```

Here `scalar()` borrows the sole output cell.

## Example 4: Empty Cells Still Create Boundaries

```cpp
std::cout << "empty cells\n"
          << pluss::note::print(
                              pluss::note::make({"", "x", ""}).join("|"))
          << '\n';
```

### Output

```text
empty cells
Note(shape=[], dim=0, numel=1, encoding=utf8)
"|x|"
```

## Example 5: Zero Cells Produce One Empty Scalar

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{},
    std::array<std::int64_t, 2>{2, 0});
std::cout << "zero cells\n" << pluss::note::print(empty.join(",")) << '\n';
```

### Output

```text
zero cells
Note(shape=[], dim=0, numel=1, encoding=utf8)
""
```

## Example 6: Undefined Propagates

```cpp
std::cout << std::boolalpha
          << "undefined join defined="
          << pluss::Note::undefined().join(",").defined() << '\n';
```

### Output

```text
undefined join defined=false
```

## Example 7: Raw Bytes Require `join_flat`

```cpp
const auto raw = pluss::note::bytes(
    std::vector<std::string>{std::string("A\0", 2), "B"},
    std::array<std::int64_t, 1>{2});
try { (void)raw.join("|"); }
catch (const pluss::Error&) { std::cout << "join bytes: rejected\n"; }
```

### Output

```text
join bytes: rejected
```

## See Also

- [`join_flat`](join_flat.md): return an owning `std::string`, including for bytes mode.
- [`vector`](vector.md): retain separate borrowed cells in flattened order.
- [`split`](split.md): divide each cell rather than combining cells.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_whitespace.html"><span class="label">Previous</span><span class="title">is_whitespace</span></a>
  <a class="next-link" href="join_flat.html"><span class="label">Next</span><span class="title">join_flat</span></a>
</div>
