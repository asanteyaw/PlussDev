<!-- pluss-hand-authored -->
# Note::vector

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::string_view> Note::vector() const
```

## Flatten Cells Into A Borrowed Sequence

`vector()` returns one `std::string_view` per cell in row-major order. It
accepts every defined rank from zero through three and deliberately discards
axis boundaries: a scalar becomes one entry, a matrix becomes its rows laid
end-to-end, and a tensor becomes its final axis changing fastest. If geometry
must survive extraction, use [`matrix()`](matrix.md), [`slabs()`](slabs.md),
or the owning rank-aware [`host()`](host.md).

The outer `std::vector` is newly allocated, but the cell bytes are not copied.
Each element is a borrowed view into the source arena. The operation therefore
costs `O(numel())` view construction and `O(numel())` container storage, not
`O(total text bytes)`. Copy an individual view to `std::string`, use `cell()` /
`cell_at()`, or materialize `host()` when the result must outlive all Notes that
share the arena.

A zero-element Note of any rank produces an empty vector. That result cannot
encode whether its source shape was `[0]`, `[2, 0]`, or `[2, 0, 4]`; inspect
[`sizes_view()`](sizes_view.md) before flattening when that distinction matters.
Undefined Notes reject because absence has neither cells nor a valid storage
contract.

## Example 1: Promote A Scalar To One Entry

```cpp
const auto cells = pluss::note::make("alpha").vector();
std::cout << "scalar=[\"" << cells[0] << "\"]\n";
```

### Output

```text
scalar=["alpha"]
```

## Example 2: Flatten A Matrix In Row-Major Order

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{
        "r0c0", "r0c1", "r0c2", "r1c0", "r1c1", "r1c2"},
    std::array<std::int64_t, 2>{2, 3});
const auto cells = grid.vector();

std::cout << "grid=[";
for (std::size_t i = 0; i < cells.size(); ++i)
  std::cout << (i ? ", \"" : "\"") << cells[i] << '"';
std::cout << "]\n";
```

### Output

```text
grid=["r0c0", "r0c1", "r0c2", "r1c0", "r1c1", "r1c2"]
```

The same order is used by `at(linear)`, iterators, and `take()`.

## Example 3: Empty Geometry Flattens To No Entries

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{},
    std::array<std::int64_t, 3>{2, 0, 4});
const auto shape = empty.sizes_view();

std::cout << "empty shape=[" << shape[0] << ", " << shape[1] << ", "
          << shape[2] << "] vector size=" << empty.vector().size() << '\n';
```

### Output

```text
empty shape=[2, 0, 4] vector size=0
```

## Example 4: Raw Cells Remain Length-Aware

```cpp
const std::array<std::byte, 3> raw{
    std::byte{'A'}, std::byte{0}, std::byte{'B'}};
const auto cell = pluss::note::bytes(raw).vector().front();

std::cout << "size=" << cell.size() << " values="
          << static_cast<int>(static_cast<unsigned char>(cell[0])) << ','
          << static_cast<int>(static_cast<unsigned char>(cell[1])) << ','
          << static_cast<int>(static_cast<unsigned char>(cell[2])) << '\n';
```

### Output

```text
size=3 values=65,0,66
```

An embedded zero is a byte inside the cell, not a terminator.

## Example 5: Make An Independent Standard String

```cpp
std::string retained;
{
  const auto temporary = pluss::note::make({"keep", "copy"});
  retained = std::string(temporary.vector()[1]);
}
std::cout << "owned copy after source=" << retained << '\n';
```

### Output

```text
owned copy after source=copy
```

## Example 6: Undefined Is Not An Empty Sequence

```cpp
try { (void)pluss::Note::undefined().vector(); }
catch (const pluss::Error&) { std::cout << "vector undefined: rejected\n"; }
```

### Output

```text
vector undefined: rejected
```

## See Also

- [`matrix`](matrix.md): preserve or promote to two-dimensional nesting.
- [`slabs`](slabs.md): preserve exact three-dimensional nesting.
- [`host`](host.md): copy cells into owning, rank-aware standard containers.
- [`sizes_view`](sizes_view.md): inspect borrowed geometry without allocation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="url_encode.html"><span class="label">Previous</span><span class="title">url_encode</span></a>
  <a class="next-link" href="../../note_api_reference.html"><span class="label">Next</span><span class="title">Note API Reference</span></a>
</div>
