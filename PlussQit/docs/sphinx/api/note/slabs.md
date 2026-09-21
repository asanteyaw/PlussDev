<!-- pluss-hand-authored -->
# Note::slabs

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<Note> Note::slabs() const
```

## Decompose A Rank-Three Note Without Leaving Note Storage

`slabs()` separates an exactly rank-three Note along its leading axis. For a
receiver with shape `[planes, rows, columns]`, it returns `planes` Notes, each
with shape `[rows, columns]`. The axis names are illustrative rather than
prescriptive: a slab may represent a page, batch member, document grid, or any
other two-dimensional slice selected by axis zero.

This method is structural extraction, not conversion to an external tensor
type. Every result remains a `Note`, retains the receiver's dtype, and supports
the normal Note operations for that dtype. Use it when a rank-three value must
be handed to code that processes one matrix-shaped Note at a time while still
preserving the closed Note contract.

No cell payload is copied. Each returned Note is a contiguous view into the
same intrusive backing allocation, beginning at source offset
`i * rows * columns`. Creating the result therefore costs `O(planes)` small
view objects rather than `O(numel)` cell copies. Those views participate in
ownership: a retained slab keeps the shared allocation alive after the source
handle is destroyed.

The receiver must be defined and have exactly three axes. Cardinality does not
substitute for rank: a one-cell scalar, vector, or matrix is rejected, as is an
undefined handle. Add or remove axes explicitly with `unsqueeze()`, `squeeze()`,
or `reshape()` when such a structural change is part of the intended model.

Zero extents are preserved rather than collapsed. Shape `[2, 0, 3]` produces
two empty Notes of shape `[0, 3]`; shape `[0, 4, 5]` produces no slabs at all.
This distinction lets callers retain known inner geometry even when no cells
are present.

## Example 1: Address Slabs, Rows, And Columns

```cpp
const auto cube = pluss::note::make(
    std::vector<std::string>{
        "000", "001", "010", "011", "100", "101", "110", "111"},
    std::array<std::int64_t, 3>{2, 2, 2});
const auto cells = cube.slabs();

std::cout << "slab-shape=[" << cells[1].size(0) << ", "
          << cells[1].size(1) << "]"
          << " [1][0][1]=" << cells[1].at(1)
          << " [1][1][0]=" << cells[1].at(2) << '\n';
```

### Output

```text
slab-shape=[2, 2] [1][0][1]=101 [1][1][0]=110
```

`at()` is flat within each returned slab, so positions `1` and `2` correspond
to coordinates `[0, 1]` and `[1, 0]` respectively.

## Example 2: Preserve Empty Inner Geometry

```cpp
const auto cells = pluss::note::make(
    std::vector<std::string>{},
    std::array<std::int64_t, 3>{2, 0, 3}).slabs();

std::cout << "empty slabs=" << cells.size()
          << " shapes=[" << cells[0].size(0) << ", " << cells[0].size(1)
          << "],[" << cells[1].size(0) << ", " << cells[1].size(1) << "]\n";
```

### Output

```text
empty slabs=2 shapes=[0, 3],[0, 3]
```

## Example 3: Preserve Native Dtype And Shared Lifetime

```cpp
std::vector<pluss::Note> cells;
{
  const auto cube = pluss::note::make(
      std::vector<std::int64_t>{10, 11, 12, 13, 20, 21, 22, 23},
      std::array<std::int64_t, 3>{2, 2, 2});
  cells = cube.slabs();
}

std::cout << std::boolalpha
          << "int64=" << cells[1].is_int64()
          << " values=" << cells[1].integers()[0] << ','
          << cells[1].integers()[3] << '\n';
```

### Output

```text
int64=true values=20,23
```

The source handle has left scope, but `cells[1]` still owns a share of the
integer allocation. This is stronger than a borrowed `std::span`, whose
lifetime must remain nested inside that of its Note.

## Example 4: An Empty Leading Axis Produces No Views

```cpp
const auto cells = pluss::note::make(
    std::vector<std::string>{},
    std::array<std::int64_t, 3>{0, 4, 5}).slabs();

std::cout << "slabs=" << cells.size() << '\n';
```

### Output

```text
slabs=0
```

## Example 5: Lower Ranks Do Not Auto-Promote

```cpp
try {
  (void)pluss::note::make({"x"}).slabs();
} catch (const pluss::Error&) {
  std::cout << "slabs rank1: rejected\n";
}
```

### Output

```text
slabs rank1: rejected
```

## Example 6: Undefined Is Not An Empty Rank-Three Note

```cpp
try {
  (void)pluss::Note::undefined().slabs();
} catch (const pluss::Error&) {
  std::cout << "slabs undefined: rejected\n";
}
```

### Output

```text
slabs undefined: rejected
```

## See Also

- [`is_rank3`](is_rank3.md): test the rank without decomposing the Note.
- [`matrix`](matrix.md): preserve rank two and promote ranks zero or one.
- [`vector`](vector.md): copy text or byte cells into one flat host vector.
- [`sizes_view`](sizes_view.md): borrow the exact shape without allocation.
- [`host`](host.md): create an owning host payload with explicit shape.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sizes_view.html"><span class="label">Previous</span><span class="title">sizes_view</span></a>
  <a class="next-link" href="sort.html"><span class="label">Next</span><span class="title">sort</span></a>
</div>
