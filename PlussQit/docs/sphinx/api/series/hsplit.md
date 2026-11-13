<!-- pluss-hand-authored -->

# Series::hsplit

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
std::vector<Series> Series::hsplit(std::int64_t sections) const
std::vector<Series> Series::hsplit(pluss::RangeView::Int indices) const
```

## What It Does

`hsplit()` is [`tensor_split()`](tensor_split.md) pinned to the **horizontal**
axis — column direction. It splits axis `1` for a 2-D-or-higher `Series`, and
axis `0` for a 1-D `Series`. The result is a `std::vector<Series>` of
views.

- **`hsplit(sections)`** requires that axis to be **exactly divisible** by
  `sections` (unlike `tensor_split`, which balances an uneven remainder). It
  then returns `sections` equal pieces.
- **`hsplit(indices)`** cuts at the given column boundaries, exactly like
  `tensor_split(indices, dim=1)`.

The companions are [`vsplit()`](vsplit.md) (axis `0`, rows) and
[`dsplit()`](dsplit.md) (axis `2`, depth).

## Parameters

**sections** (`std::int64_t`)

Number of equal pieces. The split axis length must be a multiple of it.

**indices** (`pluss::RangeView::Int`)

Column boundary positions; `n` of them produce `n + 1` pieces.

## Returns

`std::vector<Series>`
    Views along the horizontal axis.

## Gradient

Differentiable — each piece is a view, so a reverse pass routes sensitivity to
the columns the reduced pieces cover.

## Examples

Example 1. A `2 x 4` matrix into two column blocks.

```cpp
auto m = pluss::make_series({
    {1.0, 2.0, 3.0, 4.0},
    {5.0, 6.0, 7.0, 8.0},
});

auto parts = m.hsplit(2);
std::cout << parts[0];
std::cout << parts[1];
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

Example 2. An axis that is not divisible by `sections` is rejected.

```cpp
auto m = pluss::make_series({
    {1.0, 2.0, 3.0, 4.0},
    {5.0, 6.0, 7.0, 8.0},
});

try {
  m.hsplit(3);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
hsplit: axis is not divisible by sections
```

Use [`tensor_split(3, 1)`](tensor_split.md) when an uneven split is acceptable.

Example 3. Cutting at explicit column indices.

```cpp
auto m = pluss::make_series({
    {1.0, 2.0, 3.0, 4.0, 5.0},
    {6.0, 7.0, 8.0, 9.0, 10.0},
});

auto parts = m.hsplit({1, 3});
for (auto& p : parts) std::cout << "cols=" << p.sizes()[1] << " ";
```

### Output

```text
cols=1 cols=2 cols=2 
```

Columns `[0,1)`, `[1,3)`, `[3,5)`.

## Gotchas And Failure Modes

- The integer form demands exact divisibility; the index form does not.
- On a 1-D `Series`, `hsplit` acts on axis `0`.
- Pieces are views over the source storage.

## Summary

`hsplit()` splits the horizontal axis (`1`, or `0` for 1-D):

- `hsplit(n)` — `n` equal pieces, exact-divisibility required
- `hsplit(indices)` — cut at column boundaries
- zero-copy views; differentiable

## See Also

- [`Series::vsplit`](vsplit.md) / [`Series::dsplit`](dsplit.md): the vertical
  and depth counterparts.
- [`Series::tensor_split`](tensor_split.md): the general form, remainder
  balanced instead of rejected.
- [`Series::chunk`](chunk.md) / [`Series::split`](split.md): axis chosen
  explicitly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="histogram.html">
    <span class="label">Previous</span>
    <span class="title">histogram</span>
  </a>
  <a class="next-link" href="hypot.html">
    <span class="label">Next</span>
    <span class="title">hypot</span>
  </a>
</div>
