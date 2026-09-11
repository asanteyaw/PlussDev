<!-- pluss-hand-authored -->
# Note::at_indices

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::string_view Note::at_indices(
    std::span<const std::int64_t> indices) const
```

## Borrow The Cell At A Full Coordinate

`at_indices(indices)` addresses a cell with one coordinate per axis. It is the
shape-aware counterpart to [`at()`](at.md): `[1, 2]` in a `[2, 3]` Note and
flat position `5` identify the same cell, but the coordinate form makes the
geometry explicit and avoids hand-written stride arithmetic.

The number of indices must equal `dim()` exactly. A matrix needs two, a
three-dimensional Note needs three, and a scalar needs an empty span. Every
index uses the half-open range `[0, size(axis))`; negative coordinates are not
normalized. A zero-sized axis therefore has no valid coordinate.

The result is a borrowed `std::string_view` with the same byte and lifetime
contract as `at()`. Coordinate conversion is `O(dim())`, bounded by the public
maximum rank of three, and the cell lookup is constant-time. No Note or text
buffer is allocated.

## Example 1: Matrix And Three-Dimensional Coordinates

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{
        "r0c0", "r0c1", "r0c2", "r1c0", "r1c1", "r1c2"},
    std::array<std::int64_t, 2>{2, 3});
const auto tensor = pluss::note::make(
    std::vector<std::string>{
        "000", "001", "010", "011", "100", "101", "110", "111"},
    std::array<std::int64_t, 3>{2, 2, 2});

std::cout << "matrix[1,2]="
          << matrix.at_indices(std::array<std::int64_t, 2>{1, 2}) << '\n'
          << "tensor[1,0,1]="
          << tensor.at_indices(std::array<std::int64_t, 3>{1, 0, 1}) << '\n';
```

### Output

```text
matrix[1,2]=r1c2
tensor[1,0,1]=101
```

## Example 2: Scalar Coordinates Are Empty

```cpp
const auto scalar = pluss::note::make("solo");
const std::array<std::int64_t, 0> coordinate{};
std::cout << "scalar[]=" << scalar.at_indices(coordinate) << '\n';
```

### Output

```text
scalar[]=solo
```

Passing `{0}` would be a rank mismatch: rank zero means no coordinate
components, not an axis of length one.

## Example 3: Array, Vector, And Span Call Forms

```cpp
const auto grid = pluss::note::make({{"a", "b"}, {"c", "d"}});
const std::array<std::int64_t, 2> fixed{1, 0};
const std::vector<std::int64_t> dynamic{1, 1};
const std::span<const std::int64_t> borrowed(dynamic);

std::cout << grid.at_indices(fixed) << ' '
          << grid.at_indices(dynamic) << ' '
          << grid.at_indices(borrowed) << '\n';
```

### Output

```text
c d d
```

The span is consumed during the call; the method does not retain the coordinate
container.

## Example 4: Rank And Bounds Are Checked Separately

```cpp
const auto grid = pluss::note::make({{"a", "b"}, {"c", "d"}});
auto show = [](std::string_view label, auto&& call) {
  try { call(); }
  catch (const pluss::Error&) { std::cout << label << ": rejected\n"; }
};
show("rank mismatch", [&] {
  (void)grid.at_indices(std::array<std::int64_t, 1>{0});
});
show("index out of bounds", [&] {
  (void)grid.at_indices(std::array<std::int64_t, 2>{2, 0});
});
```

### Output

```text
rank mismatch: rejected
index out of bounds: rejected
```

Undefined Notes reject before geometry is read. For an owning coordinate
result, use `cell_at()` with the same coordinate rules.

## See Also

- [`at`](at.md): borrow by flat row-major position.
- [`cell_at`](cell_at.md): coordinate access whose result keeps storage alive.
- [`select`](select.md): return an entire slab as a Note.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="at.html"><span class="label">Previous</span><span class="title">at</span></a>
  <a class="next-link" href="base64_decode.html"><span class="label">Next</span><span class="title">base64_decode</span></a>
</div>
