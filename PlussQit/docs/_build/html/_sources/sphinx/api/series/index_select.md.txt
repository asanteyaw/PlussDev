<!-- pluss-hand-authored -->

# Series::index_select

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::index_select(std::int64_t dim, const Series& index) const -> Series
```

## What It Does

Selects positions along one axis using one shared index list.

This is the uniform-index sibling of [`gather()`](gather.md). The same `index`
list is reused everywhere across the non-selected axes.

That makes `index_select()` the natural spelling for:

- "pick these rows"
- "pick these columns"
- embedding-table row lookup
- reorder one axis uniformly
- repeat one axis uniformly

If the lookup pattern should vary from row to row, column to column, or lane to
lane, that is no longer `index_select()`. That is `gather()`.

## Parameters

**dim** (`std::int64_t`)

The axis to index into.

Negative dims are allowed and normalize from the back.

**index** (`const Series&`)

Integer positions along `dim`.

Practical contract:

- `index` is the list of positions to select
- its order becomes the order of the selected axis in the result
- repeated positions are legal
- unselected positions simply do not appear in the output

This is the key distinction from `gather()`: `index_select()` does not ask for
a full lane-by-lane index tensor. It asks for one shared axis list.

## Returns

`Series`
    A new series shaped like `self`, except the size of `dim` becomes
    `index.numel()`.

## Gradient

`index_select()` is differentiable in `self`.

Its backward follows the same accumulation rule as [`gather`](gather.md):

- positions never selected receive zero gradient
- positions selected once receive that one contribution
- positions selected repeatedly receive the sum of every contribution

That behavior is exactly what makes repeated token/category lookup train
correctly in an embedding-table-style use case.

## Examples

Example 1. Select rows `0`, `2`, and `3` from a `4 x 3` series.

```cpp
auto weight = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
    {7.0, 8.0, 9.0},
    {10.0, 11.0, 12.0},
});

auto row_index = pluss::make_series({0, 2, 3}, {3});
std::cout << weight.index_select(0, row_index);
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 2, 3],
  [7, 8, 9],
  [10, 11, 12]]
```

The chosen order is preserved exactly. `index_select()` is not limited to
keeping source order.

Example 2. The same idea works on columns by changing `dim`.

```cpp
auto column_index = pluss::make_series({2, 0}, {2});
std::cout << weight.index_select(1, column_index);
```

### Output

```text
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[3, 1],
  [6, 4],
  [9, 7],
  [12, 10]]
```

The same `column_index` list is reused for every row. That is exactly the
uniform-axis contract.

Example 3. Negative dims are just another spelling for the same chosen axis.

```cpp
std::cout << weight.index_select(-1, column_index);
```

### Output

```text
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[3, 1],
  [6, 4],
  [9, 7],
  [12, 10]]
```

For this rank-2 series, `-1` means the last axis, so it matches
`index_select(1, column_index)`.

Example 4. Sparse gradient: rows never looked up receive exactly zero.

```cpp
auto tracked_weight = pluss::make_series({
    {1.0, 1.0, 1.0},
    {2.0, 2.0, 2.0},
    {3.0, 3.0, 3.0},
    {4.0, 4.0, 4.0},
    {5.0, 5.0, 5.0},
});
tracked_weight.tracks_slope_(true);

auto lookup = pluss::make_series({0, 2, 4}, {3});
auto embedded = tracked_weight.index_select(0, lookup);
embedded.sum().reverse();
std::cout << "out=" << embedded << "\n";
std::cout << "dw=" << tracked_weight.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 1, 1],
  [3, 3, 3],
  [5, 5, 5]]
dw=pluss::Series(shape=[5,3], strides=[3,1], dtype=float64, platform=cpu, numel=15, contiguous=true)
data=[[1, 1, 1],
  [0, 0, 0],
  [1, 1, 1],
  [0, 0, 0],
  [1, 1, 1]]
```

Rows `1` and `3` were never selected, so they stay exactly zero in the
backward pass.

Example 5. Repeated indices repeat data in the forward pass and accumulate
gradient in the backward pass.

```cpp
auto repeated_lookup = pluss::make_series({0, 0, 3}, {3});
std::cout << weight.index_select(0, repeated_lookup) << "\n";

auto repeated_weight = weight.clone();
repeated_weight.tracks_slope_(true);
auto repeated_out = repeated_weight.index_select(0, repeated_lookup);
repeated_out.sum().reverse();
std::cout << "out=" << repeated_out << "\n";
std::cout << "dw=" << repeated_weight.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 2, 3],
  [1, 2, 3],
  [10, 11, 12]]
out=pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 2, 3],
  [1, 2, 3],
  [10, 11, 12]]
dw=pluss::Series(shape=[4,3], strides=[3,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[2, 2, 2],
  [0, 0, 0],
  [0, 0, 0],
  [1, 1, 1]]
```

Row `0` was selected twice, so every element in that row receives gradient `2`.

## Notes

- `index_select()` is the right API when one index list should be reused
  uniformly.
- It is one of the most common lookup primitives in learned-table code.
- If you find yourself wanting a different index pattern per row, move to
  [`gather`](gather.md).
- Output rank matches `self`, but the selected axis length becomes
  `index.numel()`.

## See Also

- [`Series::gather`](gather.md): the more general varying-pattern primitive.
- [`Series::copy_`](copy_.md): common parameter initialization partner for
  lookup-table code.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gather.html">
    <span class="label">Previous</span>
    <span class="title">gather</span>
  </a>
  <a class="next-link" href="inner.html">
    <span class="label">Next</span>
    <span class="title">inner</span>
  </a>
</div>
