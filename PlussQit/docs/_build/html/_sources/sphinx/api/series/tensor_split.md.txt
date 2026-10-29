<!-- pluss-hand-authored -->

# Series::tensor_split

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::tensor_split(std::int64_t sections, std::int64_t dim = 0) const
    -> std::vector<Series>

Series::tensor_split(pluss::RangeView::Int indices, std::int64_t dim = 0) const
    -> std::vector<Series>
```

## What It Does

`tensor_split()` partitions one axis and returns the pieces as a
`std::vector<Series>`. It has two modes:

- **`tensor_split(sections, dim)`** always produces **exactly `sections`**
  pieces. When `size(dim)` is not divisible by `sections`, the first
  `size(dim) % sections` pieces are one element longer than the rest — so a
  length-8 axis into `3` gives `3, 3, 2`, and a length-6 axis into `4` gives
  `2, 2, 1, 1` (empty pieces appear if `sections > size(dim)`).
- **`tensor_split(indices, dim)`** cuts at the given boundary indices. Indices
  `{2, 5, 8}` on a length-10 axis produce the four ranges `[0,2)`, `[2,5)`,
  `[5,8)`, `[8,10)`.

The contrast with [`chunk()`](chunk.md): `chunk(n)` targets a *piece size* of
`ceil(size/n)` and may return **fewer** than `n` pieces; `tensor_split(n)`
always returns exactly `n`, balancing the remainder across the front. Every
piece is a `slice()` **view** — zero-copy and differentiable.

## Parameters

**sections** (`std::int64_t`)

The exact number of pieces. Must be `> 0`.

**indices** (`pluss::RangeView::Int`)

Boundary positions along `dim` — a `std::vector<std::int64_t>` or a brace list.
`n` indices produce `n + 1` pieces. Values are clamped into range and treated
in order.

**dim** (`std::int64_t`, default `0`)

The axis to split. Negative values count from the end.

## Returns

`std::vector<Series>`
    Exactly `sections` views (integer form), or `indices.size() + 1` views
    (index form).

## Gradient

`tensor_split()` is differentiable — each piece is a view, so a reverse pass
routes sensitivity to exactly the positions the reduced pieces cover.

## Examples

Example 1. `sections` form on an axis that does not divide evenly.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0});

auto parts = x.tensor_split(3);
std::cout << "pieces=" << parts.size() << "  sizes: ";
for (auto& p : parts) std::cout << p.sizes()[0] << " ";
```

### Output

```text
pieces=3  sizes: 3 3 2 
```

`8 = 3 + 3 + 2`: the first `8 % 3 = 2` pieces get the extra element.

Example 2. Exactly `sections` pieces even when some come out empty.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});

auto parts = x.tensor_split(4);
std::cout << "pieces=" << parts.size() << "  sizes: ";
for (auto& p : parts) std::cout << p.sizes()[0] << " ";
```

### Output

```text
pieces=4  sizes: 2 2 1 1 
```

Compare [`chunk(4)`](chunk.md) on the same input, which returns **three**
pieces of `2`.

Example 3. Splitting at explicit indices.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});

auto parts = x.tensor_split({2, 5, 8});
for (auto& p : parts) std::cout << p;
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 3, 4]
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5, 6, 7]
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[8, 9]
```

Three cut points, four pieces.

Example 4. Gradient reaches only reduced pieces.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
x.tracks_slope_(true);

x.tensor_split(3)[2].sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0, 0, 0, 0, 1, 1]
```

`tensor_split(3)` of length 6 is `2, 2, 2`; piece `2` is positions `4` and `5`.

## Gotchas And Failure Modes

- Integer form: always exactly `sections` pieces; the remainder is spread over
  the **front**, not stacked into a short final piece as [`chunk()`](chunk.md)
  does.
- If `sections > size(dim)`, the trailing pieces are empty (shape `0` along
  `dim`).
- Index form: `n` indices give `n + 1` pieces; a leading `0` or a trailing
  `size(dim)` in the list just yields an empty piece.
- Pieces are views over the source storage.

## Summary

`tensor_split()` partitions an axis into view pieces:

- `tensor_split(n)` — exactly `n` pieces, front-loaded remainder
- `tensor_split(indices)` — cut at explicit boundaries, `n + 1` pieces
- zero-copy; differentiable
- differs from [`chunk()`](chunk.md), which may return fewer than `n` pieces

## See Also

- [`Series::chunk`](chunk.md): targets a piece size; may return fewer pieces.
- [`Series::split`](split.md): fix the piece size, or give explicit lengths.
- [`Series::hsplit`](hsplit.md) / [`Series::vsplit`](vsplit.md) /
  [`Series::dsplit`](dsplit.md): `tensor_split` pinned to axis 1 / 0 / 2.
- [`Series::unite`](unite.md): the concatenation that reverses a split.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tensor.html">
    <span class="label">Previous</span>
    <span class="title">tensor</span>
  </a>
  <a class="next-link" href="tensordot.html">
    <span class="label">Next</span>
    <span class="title">tensordot</span>
  </a>
</div>
