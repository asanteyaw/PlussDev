<!-- pluss-hand-authored -->

# Series::chunk

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::chunk(std::int64_t chunks, std::int64_t dim = 0) const
    -> std::vector<Series>
```

## What It Does

`chunk()` splits one axis into **at most** `chunks` pieces of roughly equal
size and returns them as a `std::vector<Series>` — a small list you index
with `result[i]` and iterate.

The piece size is `ceil(size(dim) / chunks)`. Every piece is that long except
possibly a shorter final one, and if the axis is short enough that the ceiling
already covers it in fewer than `chunks` slices, you get fewer pieces. For
example a length-6 axis with `chunks = 4` gives piece size `ceil(6/4) = 2` and
therefore **three** pieces of `2`, not four.

Each piece is a `slice()` **view** — it shares storage with the input, costs
nothing to produce, and is differentiable. Use [`split()`](split.md) when you
want to fix the piece *size* instead of the piece *count*, and
[`tensor_split()`](tensor_split.md) when you need exactly `chunks` pieces every
time.

## Parameters

**chunks** (`std::int64_t`)

The maximum number of pieces. Must be `> 0`.

**dim** (`std::int64_t`, default `0`)

The axis to split. Negative values count from the end.

## Returns

`std::vector<Series>`
    Between `1` and `chunks` views along `dim`. `result.size()` is the actual
    count; `result[i]` is the `i`-th piece.

## Gradient

`chunk()` is differentiable. Each piece is a view, so a reverse pass through
any subset of the pieces routes sensitivity back to exactly the positions those
pieces cover; positions in pieces you did not use receive `0`.

## Examples

Example 1. A length-6 axis into 3 equal pieces.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});

auto parts = x.chunk(3);
std::cout << "pieces=" << parts.size() << "\n";
for (std::size_t i = 0; i < parts.size(); ++i) {
  std::cout << "[" << i << "] " << parts[i].host<double>().size()
            << " elems: ";
  for (double v : parts[i].host<double>()) std::cout << v << " ";
  std::cout << "\n";
}
```

### Output

```text
pieces=3
[0] 2 elems: 0 1 
[1] 2 elems: 2 3 
[2] 2 elems: 4 5 
```

Example 2. `chunks` is an upper bound: a length-6 axis with `chunks = 4` still
gives three pieces, because `ceil(6/4) = 2` covers everything in three slices.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});

std::cout << "pieces=" << x.chunk(4).size();
```

### Output

```text
pieces=3
```

Example 3. When the axis does not divide evenly, the last piece is shorter.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

auto parts = x.chunk(3);
for (std::size_t i = 0; i < parts.size(); ++i) {
  std::cout << "[" << i << "] len=" << parts[i].sizes()[0] << "  ";
}
```

### Output

```text
[0] len=3  [1] len=3  [2] len=1  
```

Piece size is `ceil(7/3) = 3`, so the split is `3, 3, 1`.

Example 4. Splitting a non-leading axis.

```cpp
auto m = pluss::make_series({
    {1.0, 2.0, 3.0, 4.0},
    {5.0, 6.0, 7.0, 8.0},
});

auto cols = m.chunk(2, /*dim=*/1);
std::cout << cols[0];
std::cout << cols[1];
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

Each half is a non-contiguous view over the original columns.

Example 5. Gradient flows only through the pieces you use.

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
x.tracks_slope_(true);

x.chunk(3)[1].sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0, 0, 1, 1, 0, 0]
```

Only piece `1` (positions `2` and `3`) was reduced, so only those receive
gradient.

Example 6. `chunks` must be positive.

```cpp
try {
  pluss::make_series({0.0, 1.0, 2.0}).chunk(0);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
chunk: chunks must be > 0
```

## Gotchas And Failure Modes

- `chunks` is a **maximum**, not a guarantee. Read `result.size()` for the
  actual count, or use [`tensor_split()`](tensor_split.md) for an exact count.
- Pieces are views. Mutating one mutates the source; materialize with
  [`contiguous()`](contiguous.md) or [`clone()`](clone.md) if you need
  independent storage.
- The result is a `std::vector<Series>`, not a single `Series` — index it
  with `[i]`.
- `unsafe_chunk()` is the same operation here; the aliasing-safety distinction
  only matters for backends pluss does not expose.

## Summary

`chunk(chunks, dim)` splits an axis into at most `chunks` near-equal view
pieces:

- piece size `ceil(size(dim) / chunks)`; a short final piece, or fewer pieces
  than requested, when the axis does not divide evenly
- each piece is a zero-copy `slice()` view; differentiable
- returns a `std::vector<Series>` — index with `result[i]`

## See Also

- [`Series::split`](split.md): fix the piece *size* rather than the count.
- [`Series::tensor_split`](tensor_split.md): always exactly `n` pieces; also
  splits at explicit indices.
- [`Series::unbind`](unbind.md): remove the axis entirely, one piece per index.
- [`Series::unite`](unite.md): the concatenation that reverses a split.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ceil.html">
    <span class="label">Previous</span>
    <span class="title">ceil</span>
  </a>
  <a class="next-link" href="clamp.html">
    <span class="label">Next</span>
    <span class="title">clamp</span>
  </a>
</div>
