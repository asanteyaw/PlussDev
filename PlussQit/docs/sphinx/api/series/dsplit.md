<!-- pluss-hand-authored -->

# Series::dsplit

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
std::vector<Series> Series::dsplit(std::int64_t sections) const
std::vector<Series> Series::dsplit(pluss::RangeView::Int indices) const
```

## What It Does

`dsplit()` is [`tensor_split()`](tensor_split.md) pinned to the **depth** axis —
axis `2`. It requires a `Series` of rank `>= 3`. Otherwise it matches
[`hsplit()`](hsplit.md) and [`vsplit()`](vsplit.md): `dsplit(sections)` needs
axis `2` to be exactly divisible by `sections` and returns `sections` equal
slabs; `dsplit(indices)` cuts at the given depth boundaries. Each piece is a
view.

## Parameters

**sections** (`std::int64_t`)

Number of equal slabs; axis `2` length must be a multiple of it.

**indices** (`pluss::RangeView::Int`)

Depth boundary positions; `n` of them give `n + 1` pieces.

## Returns

`std::vector<Series>`
    Depth-slab views.

## Gradient

Differentiable, exactly as [`hsplit()`](hsplit.md).

## Example

```cpp
std::vector<double> d(2 * 2 * 4);
for (int i = 0; i < 16; ++i) d[i] = i;
auto cube = pluss::make_series(d, {2, 2, 4});

auto parts = cube.dsplit(2);
std::cout << "pieces=" << parts.size()
          << "  each shape [" << parts[0].sizes()[0] << ","
          << parts[0].sizes()[1] << "," << parts[0].sizes()[2] << "]";
```

### Output

```text
pieces=2  each shape [2,2,2]
```

## Gotchas And Failure Modes

- Requires rank `>= 3`; lower-rank input raises.
- The integer form demands exact divisibility of axis `2`.
- Pieces are views over the source storage.

## Summary

`dsplit()` splits axis `2` (depth) of a rank-`>= 3` `Series`:

- `dsplit(n)` — `n` equal slabs, exact divisibility required
- `dsplit(indices)` — cut at depth boundaries
- zero-copy views; differentiable

## See Also

- [`Series::hsplit`](hsplit.md) / [`Series::vsplit`](vsplit.md): the horizontal
  and vertical counterparts.
- [`Series::tensor_split`](tensor_split.md): the general form on any axis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="div.html">
    <span class="label">Previous</span>
    <span class="title">div</span>
  </a>
  <a class="next-link" href="dtype.html">
    <span class="label">Next</span>
    <span class="title">dtype</span>
  </a>
</div>
