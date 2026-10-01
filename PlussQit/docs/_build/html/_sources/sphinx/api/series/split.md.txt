<!-- pluss-hand-authored -->

# Series::split

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::split(std::int64_t split_size, std::int64_t dim = 0) const
    -> std::vector<Series>

```

## What It Does

`split()` cuts one axis into consecutive pieces and returns them as a
`std::vector<Series>` (index with `result[i]`, iterate directly).

- **`split(split_size, dim)`** makes every piece exactly `split_size` long,
  except a possibly shorter final one. A length-10 axis with `split_size = 3`
  gives `3, 3, 3, 1`.
For explicit per-piece lengths, use
[`split_with_sizes()`](split_with_sizes.md).

Where [`chunk()`](chunk.md) fixes the *number* of pieces and lets their size
fall out, `split()` fixes the *size* (or the exact sizes) and lets the count
fall out. Every piece is a `slice()` **view** — zero-copy, storage-sharing, and
differentiable.

## Parameters

**split_size** (`std::int64_t`)

The length of each piece (the last may be shorter). Must be `> 0`.

**dim** (`std::int64_t`, default `0`)

The axis to split. Negative values count from the end.

## Returns

`std::vector<Series>`
    Consecutive views along `dim`.

## Gradient

`split()` is differentiable. Each piece is a view, so a reverse pass through
some of the pieces sends sensitivity back to exactly the positions they cover.

## Examples

Example 1. A fixed piece size, with a short final piece.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});

auto parts = x.split(3);
std::cout << "pieces=" << parts.size() << "\n";
for (std::size_t i = 0; i < parts.size(); ++i) {
  std::cout << "[" << i << "] len=" << parts[i].sizes()[0] << "  ";
}
```

### Output

```text
pieces=4
[0] len=3  [1] len=3  [2] len=3  [3] len=1  
```

Example 2. `split(split_size)` versus [`chunk(n)`](chunk.md) — same number,
different meaning.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});

auto by_size  = x.split(3);   // "pieces of 3"
auto by_count = x.chunk(3);   // "3 pieces"

std::cout << "split(3): ";
for (auto& p : by_size)  std::cout << p.sizes()[0] << " ";
std::cout << "\nchunk(3): ";
for (auto& p : by_count) std::cout << p.sizes()[0] << " ";
```

### Output

```text
split(3): 3 3 3 1 
chunk(3): 4 4 2 
```

`split(3)` cuts pieces of length `3` (four of them, plus the remainder);
`chunk(3)` makes exactly three pieces (length `ceil(10/3) = 4`, then the
remainder).

Example 3. Splitting a chosen axis.

```cpp
auto m = pluss::make_series({
    {1.0, 2.0, 3.0, 4.0},
    {5.0, 6.0, 7.0, 8.0},
});

auto halves = m.split(2, /*dim=*/1);
std::cout << halves[0];
std::cout << halves[1];
```

### Output

```text
pluss::Series(shape=[2,2], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=false)
data=[[1, 2],
  [5, 6]]
pluss::Series(shape=[2,2], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=false)
data=[[3, 4],
  [7, 8]]
```

Example 4. Gradient reaches only the pieces you reduce.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
x.tracks_slope_(true);

auto parts = x.split(2);
parts[0].sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[1, 1, 0, 0, 0, 0]
```

## Gotchas And Failure Modes

- `split(split_size)` fixes the piece size; the count depends on the axis
  length. For a fixed count use [`chunk()`](chunk.md) or
  [`tensor_split()`](tensor_split.md).
- Pieces are views over the source storage. Materialize before mutating.

## Summary

`split()` cuts an axis into consecutive view pieces:

- `split(split_size)` — uniform size, short final piece, count derived
- zero-copy views; differentiable
- returns a `std::vector<Series>`

## See Also

- [`Series::chunk`](chunk.md): fix the number of pieces instead of the size.
- [`Series::tensor_split`](tensor_split.md): exact count, or split at explicit
  indices.
- [`Series::unbind`](unbind.md): one slice per index, with the axis removed.
- [`Series::unite`](unite.md): the concatenation that reverses a split.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort_pair.html">
    <span class="label">Previous</span>
    <span class="title">sort_pair</span>
  </a>
  <a class="next-link" href="split_with_sizes.html">
    <span class="label">Next</span>
    <span class="title">split_with_sizes</span>
  </a>
</div>
