<!-- pluss-hand-authored -->

# Series::unbind

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::unbind(std::int64_t dim = 0) const -> std::vector<Series>
```

## What It Does

`unbind()` removes one axis and returns every slice along it as a
`std::vector<Series>`. A `[3, 2]` matrix `unbind(0)` gives three `[2]`
vectors; `unbind(1)` gives two `[3]` vectors. The unbound axis is **gone** from
each piece — this is the difference from [`chunk()`](chunk.md) /
[`split()`](split.md), which keep the axis and just shorten it.

Each piece is a `select()` **view** (zero-copy, storage-sharing,
differentiable). `unbind()` is the natural inverse of [`pile()`](pile.md):
`pile(x.unbind(d), d)` reconstructs `x`.

## Parameters

**dim** (`std::int64_t`, default `0`)

The axis to remove. Negative values count from the end.

## Returns

`std::vector<Series>`
    `size(dim)` views, each with rank one less than the input.

## Gradient

`unbind()` is differentiable. Each piece is a `select()` view, so a reverse
pass through some of the slices scatters sensitivity back into the removed axis
at the corresponding indices.

## Examples

Example 1. Unbinding the leading axis of a `3 x 2` matrix.

```cpp
auto m = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
    {5.0, 6.0},
});

auto rows = m.unbind(0);
std::cout << "pieces=" << rows.size() << "\n";
for (std::size_t i = 0; i < rows.size(); ++i) {
  std::cout << "[" << i << "] rank=" << rows[i].dim() << " -> ";
  for (double v : rows[i].to_vec<double>()) std::cout << v << " ";
  std::cout << "\n";
}
```

### Output

```text
pieces=3
[0] rank=1 -> 1 2 
[1] rank=1 -> 3 4 
[2] rank=1 -> 5 6 
```

Each piece is rank `1` — the row axis is gone.

Example 2. `unbind(1)` walks the columns instead.

```cpp
auto m = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
    {5.0, 6.0},
});

auto cols = m.unbind(1);
std::cout << "pieces=" << cols.size() << "\n";
std::cout << cols[0];
std::cout << cols[1];
```

### Output

```text
pieces=2
pluss::Series(shape=[3], strides=[2], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[1, 3, 5]
pluss::Series(shape=[3], strides=[2], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[2, 4, 6]
```

Each column view is strided (step `2`) over the original storage.

Example 3. `unbind` is the inverse of [`pile()`](pile.md).

```cpp
auto m = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
    {5.0, 6.0},
});

auto pieces = m.unbind(0);
auto rebuilt = pieces[0].pile({pieces[1], pieces[2]}, 0);

std::cout << rebuilt;
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6]]
```

Example 4. Gradient scatters back into the removed axis.

```cpp
auto m = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
    {5.0, 6.0},
});
m.tracks_slope_(true);

m.unbind(0)[1].sum().reverse();

std::cout << "dm=" << m.sensitivity();
```

### Output

```text
dm=pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0, 0],
  [1, 1],
  [0, 0]]
```

Only row `1` was reduced, so only that row receives gradient.

## Gotchas And Failure Modes

- Each piece has **one fewer axis** than the input. If you want to keep the
  axis (as a shortened one), use [`chunk()`](chunk.md) or [`split()`](split.md).
- Pieces are `select()` views; mutating one mutates the source.
- `unbind()` on a 0-dim `Series` raises — there is no axis to remove.
- The result is a `std::vector<Series>` — index with `[i]`.

## Summary

`unbind(dim)` removes an axis and returns its slices:

- `size(dim)` pieces, each rank one lower
- zero-copy `select()` views; differentiable
- the inverse of [`pile()`](pile.md)

## See Also

- [`Series::pile`](pile.md): stack along a new axis — reverses `unbind`.
- [`Series::select`](select.md): pull out a single slice with the axis removed.
- [`Series::chunk`](chunk.md) / [`Series::split`](split.md): partition an axis
  while keeping it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="trunc.html">
    <span class="label">Previous</span>
    <span class="title">trunc</span>
  </a>
  <a class="next-link" href="unfold.html">
    <span class="label">Next</span>
    <span class="title">unfold</span>
  </a>
</div>
