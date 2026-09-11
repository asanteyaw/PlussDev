<!-- pluss-hand-authored -->
# Note::tensor3

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<Note> Note::tensor3() const
```

## Preserve Three-Dimensional Cell Geometry

`tensor3()` decomposes an exactly rank-three Note into rank-two slab Notes.
Unlike `matrix()`, it performs no rank promotion: scalar, vector, and matrix
receivers reject even when they contain exactly one cell. Keeping each slab as
a Note preserves dtype and its two-dimensional geometry.

For source shape `[d0, d1, d2]`, `result[i]` has shape `[d1, d2]` and begins at
source offset `i * d1 * d2`. Slabs share the source arena or aligned native
lane buffer, so construction costs `O(d0)` and copies no cells. A slab owns its
share of the backing allocation and safely outlives the source.

Zero extents are preserved exactly. For shape `[2, 0, 3]`, the result contains
two slab Notes, each with shape `[0, 3]`; the trailing extent remains available
even though there are no rows or cells.

## Example 1: Address Slabs, Rows, And Columns

```cpp
const auto cube = pluss::note::make(
    std::vector<std::string>{
        "000", "001", "010", "011", "100", "101", "110", "111"},
    std::array<std::int64_t, 3>{2, 2, 2});
const auto cells = cube.tensor3();

std::cout << "slab-shape=[" << cells[1].size(0) << ", "
          << cells[1].size(1) << "]"
          << " [1][0][1]=" << cells[1].at(1)
          << " [1][1][0]=" << cells[1].at(2) << '\n';
```

### Output

```text
slab-shape=[2, 2] [1][0][1]=101 [1][1][0]=110
```

## Example 2: Preserve Empty Outer Structure

```cpp
const auto cells = pluss::note::make(
    std::vector<std::string>{},
    std::array<std::int64_t, 3>{2, 0, 3}).tensor3();

std::cout << "empty slabs=" << cells.size()
          << " rows=" << cells[0].size(0) << ',' << cells[1].size(0) << '\n';
```

### Output

```text
empty slabs=2 rows=0,0
```

## Example 3: Lower Ranks Do Not Auto-Promote

```cpp
try { (void)pluss::note::make({"x"}).tensor3(); }
catch (const pluss::Error&) { std::cout << "tensor3 rank1: rejected\n"; }
```

### Output

```text
tensor3 rank1: rejected
```

Call `unsqueeze()` or `reshape({1, 1, 1})` explicitly when adding axes is part
of the intended data model.

## Example 4: Undefined Is Not An Empty Tensor

```cpp
try { (void)pluss::Note::undefined().tensor3(); }
catch (const pluss::Error&) { std::cout << "tensor3 undefined: rejected\n"; }
```

### Output

```text
tensor3 undefined: rejected
```

## See Also

- [`matrix`](matrix.md): preserve rank two and promote ranks zero or one.
- [`vector`](vector.md): flatten all ranks in row-major order.
- [`sizes_view`](sizes_view.md): retain exact extents beside borrowed cells.
- [`host`](host.md): create an owning flat payload with explicit shape.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html"><span class="label">Previous</span><span class="title">take</span></a>
  <a class="next-link" href="tex_escape.html"><span class="label">Next</span><span class="title">tex_escape</span></a>
</div>
