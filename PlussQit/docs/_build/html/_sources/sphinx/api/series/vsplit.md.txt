<!-- pluss-hand-authored -->

# Series::vsplit

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::vsplit(std::int64_t sections) const -> std::vector<Series>
Series::vsplit(pluss::RangeView::Int indices) const -> std::vector<Series>
```

## What It Does

`vsplit()` is [`tensor_split()`](tensor_split.md) pinned to the **vertical**
axis — row direction, axis `0`. It requires a `Series` of rank `>= 2`. In every
other respect it behaves like [`hsplit()`](hsplit.md): `vsplit(sections)` needs
axis `0` to be exactly divisible by `sections` and returns `sections` equal
row blocks; `vsplit(indices)` cuts at the given row boundaries. Each piece is a
view.

## Parameters

**sections** (`std::int64_t`)

Number of equal row blocks; axis `0` length must be a multiple of it.

**indices** (`pluss::RangeView::Int`)

Row boundary positions; `n` of them give `n + 1` pieces.

## Returns

`std::vector<Series>`
    Row-block views.

## Gradient

Differentiable, exactly as [`hsplit()`](hsplit.md).

## Example

```cpp
auto m = pluss::make_series({
    {1.0, 2.0, 3.0, 4.0},
    {5.0, 6.0, 7.0, 8.0},
});

auto parts = m.vsplit(2);
std::cout << parts[0];
std::cout << parts[1];
```

### Output

```text
pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2, 3, 4]]
pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[5, 6, 7, 8]]
```

## Gotchas And Failure Modes

- Requires rank `>= 2`; a 1-D `Series` raises.
- The integer form demands exact divisibility of axis `0`.
- Pieces are views over the source storage.

## Summary

`vsplit()` splits axis `0` (rows) of a rank-`>= 2` `Series`:

- `vsplit(n)` — `n` equal row blocks, exact divisibility required
- `vsplit(indices)` — cut at row boundaries
- zero-copy views; differentiable

## See Also

- [`Series::hsplit`](hsplit.md) / [`Series::dsplit`](dsplit.md): the horizontal
  and depth counterparts.
- [`Series::tensor_split`](tensor_split.md): the general form.
- [`Series::unbind`](unbind.md): one row per piece, with the axis removed.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="view.html">
    <span class="label">Previous</span>
    <span class="title">view</span>
  </a>
  <a class="next-link" href="where.html">
    <span class="label">Next</span>
    <span class="title">where</span>
  </a>
</div>
