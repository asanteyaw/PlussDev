<!-- pluss-hand-authored -->

# Series::index_select

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::index_select(std::int64_t dim, const Series& index) const -> Series
```

## What It Does

Selects a list of positions along `dim`, applying the **same** `index` list to
every row/column/... along every other axis — the classic "pick these rows" or
"pick these columns" operation, and the everyday shape behind a learned embedding
table lookup: one row of weights selected per category code.

`index_select` is a special case of [`gather`](gather.md): internally, it expands
`index` to `gather`'s shape (broadcasting the same list across every other axis)
and delegates to it. If you ever need a *different* index list per row, that's
exactly the situation [`gather`](gather.md) itself is for, and `index_select`
isn't.

The classic use is an embedding table lookup — see [`Series::copy_`](copy_.md)'s
own Example 2 for how such a table typically gets its initial values without
disturbing its later gradient.

## Parameters

**dim** (`std::int64_t`)

Which axis to select along.

**index** (`const Series&`)

A 1-D (or flattenable) list of integer positions along `dim`, each in
`[0, self.sizes()[dim])`. The same list is applied uniformly across every other
axis. Positions may repeat.

## Returns

`Series`
    A new `Series` shaped like `self`, except `dim`'s length is replaced by
    `index.numel()`.

## Gradient

`index_select` is differentiable in `self`, inheriting [`gather`](gather.md)'s
`scatter_add`-based backward: positions never selected get gradient `0`; positions
selected more than once get the **sum** of every selection's contribution, not
just the last one.

```{note} This is the mechanism behind a correctly-trained embedding table
A learned embedding's weight matrix is `[cardinality, hidden_size]`, and a forward
pass is `weight.index_select(0, category_codes)`. This only trains correctly if
the sparsity in Example 2 holds exactly: a category that never appears in a batch
must get *zero* gradient that step, not some small nonzero leftover, and a category
that appears twice must get the sum of both occurrences' gradient, not just one of
them. Both are guaranteed by `index_select` inheriting `gather`'s exact backward
rather than approximating it.
```

## Examples

Example 1. Select rows `0`, `2`, and `3` from a 4×3 matrix, in that order — note
that the output can reorder and repeat, it isn't limited to `self`'s original row
order.

```cpp
auto input = pluss::make_series(
    std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {4, 3});
auto index = pluss::make_series(std::vector<std::int64_t>{0, 2, 3}, {3});
std::cout << input.index_select(0, index);
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 2, 3],
  [7, 8, 9],
  [10, 11, 12]]
```

Output row 0 is input row 0, output row 1 is input row 2, output row 2 is input
row 3 — row 1 of the input (`[4, 5, 6]`) was never selected and is simply absent.

Example 2. Sparse gradient through an embedding-table-style lookup: a `[5, 3]`
weight table, looking up rows `0`, `2`, and `4` — rows `1` and `3` should get
exactly zero gradient.

```cpp
auto weight = pluss::make_series(
    std::vector<double>{1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5}, {5, 3});
weight.tracks_slope_(true);

auto index = pluss::make_series(std::vector<std::int64_t>{0, 2, 4}, {3});
auto out = weight.index_select(0, index);
out.sum().reverse();
std::cout << "out=" << out << "\n";
std::cout << "weight.sensitivity()=" << weight.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 1, 1],
  [3, 3, 3],
  [5, 5, 5]]
weight.sensitivity()=pluss::Series(shape=[5,3], strides=[3,1], dtype=float64, platform=cpu, numel=15, contiguous=true)
data=[[1, 1, 1],
  [0, 0, 0],
  [1, 1, 1],
  [0, 0, 0],
  [1, 1, 1]]
```

Rows 0, 2, and 4 — the ones actually looked up — get gradient `1` per element
(each contributed to exactly one summed output position). Rows 1 and 3, never
looked up, are exactly `0`, not some small nonzero residual.

## Notes

- `index_select` broadcasts one index list across every non-`dim` axis; per-row
  variation requires [`gather`](gather.md) directly.
- Repeated indices are not an error; the gradient for a repeated position is the
  sum of every selection's contribution (same rule as [`gather`](gather.md)).

## See Also

- [`Series::gather`](gather.md): the more general op `index_select` is built from.
- [`Series::copy_`](copy_.md): how the weight table in Example 2 typically gets its
  initial values without disturbing its later gradient.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gather.html">
    <span class="label">Previous</span>
    <span class="title">gather</span>
  </a>
  <a class="next-link" href="maximum.html">
    <span class="label">Next</span>
    <span class="title">maximum</span>
  </a>
</div>
