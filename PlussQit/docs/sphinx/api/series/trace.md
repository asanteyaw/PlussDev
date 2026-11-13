<!-- pluss-hand-authored -->

# Series::trace

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::trace() const
Series Series::trace(std::int64_t offset) const
Series Series::trace(std::int64_t offset, std::int64_t dim1, std::int64_t dim2) const
```

## What It Does

`trace()` sums a diagonal of a `Series` and returns that sum. On a 2D matrix
the result is a scalar `Series`. On higher-rank input the diagonal is taken
over a chosen pair of axes (by default the first two) and the remaining axes
are kept, so a stack of matrices gives one trace per matrix.

Two parts of that contract matter in real code:

- it is a reduction over the diagonal axis only
- it reads only the diagonal positions, not the full matrix

`trace()` also accepts rectangular 2D input. In that case it walks only as far
as a diagonal still exists, so the effective diagonal length is
`min(rows, cols)`.

## Parameters

**offset** (`std::int64_t`, default `0`)

Which diagonal to sum. Positive values move toward later columns (above the
main diagonal), negative values toward later rows.

**dim1**, **dim2** (`std::int64_t`, defaults `0` and `1`)

The two axes the diagonal is read from; negative values count from the end.
They must differ. Every other axis is carried into the result in order. The
offset is measured from `dim1` toward `dim2`.

## Returns

`Series`
    A scalar `Series` for 2D input. For rank `r > 2`, a `Series` of rank
    `r - 2` holding one diagonal sum per position of the remaining axes.

That scalar result still behaves like an ordinary `Series`. You can print it,
chain more `Series` methods from it, or extract the contained value with
`item<T>()` when you truly need a plain scalar.

## Gradient

`trace()` propagates reverse-mode sensitivity to the diagonal positions that
participated in the reduction.

That is the important behavior to expect in practice:

- diagonal entries receive sensitivity
- off-diagonal entries do not
- the result is the same diagonal mask you would expect from "extract the
  diagonal, then sum it"

## Examples

Example 1. On a square matrix, `trace()` adds the main diagonal entries.

```cpp
auto square = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

std::cout << square.trace();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=15
```

Example 2. Rectangular input is still valid. Only the existing diagonal is
summed.

```cpp
auto rectangular = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, rectangular.trace());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=6
```

So this is not a "square matrices only" method.

Example 3. Rank-1 input fails because there is no matrix diagonal to reduce.

```cpp
try {
  std::cout << pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64).trace() << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
diagonal: expects a Series with at least 2 dimensions
```

The message comes from the shared diagonal extraction path, which is why it
mentions diagonal extraction rather than `trace()` by name.

Example 4. Reverse-mode sensitivity routes only to the diagonal positions that
contributed to the trace.

```cpp
auto trace_grad = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
    {7.0, 8.0, 9.0},
});
trace_grad.tracks_slope_(true);
trace_grad.trace().reverse();

std::cout << trace_grad.sensitivity();
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0],
  [0, 0, 1]]
```

Example 5. An offset sums a shifted diagonal.

```cpp
auto square = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

std::cout << square.trace(1) << "\n";   // 2 + 6
std::cout << square.trace(-1);          // 4 + 8
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=8
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=12
```

Example 6. Higher-rank input: pick the two axes, and the rest are kept.

```cpp
auto cube = pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                                13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24}, {2, 3, 4});

std::cout << cube.trace(0, 1, 2);   // trace of each 3 x 4 slice
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[18, 54]
```

Axis 0 is kept, so there is one trace per slice: `1 + 6 + 11` and
`13 + 18 + 23`.

## Gotchas And Failure Modes

- `trace()` returns a scalar `Series`, not a raw number.
- Rectangular matrices are valid; the diagonal length is clipped naturally.
- Rank-0 and rank-1 inputs fail because there is no pair of axes to take a
  diagonal over.
- With the default axes, rank-3+ input is a batched trace over axes `0` and
  `1`; pass `dim1`/`dim2` (for example `-2, -1`) to trace the trailing matrices
  instead.
- Reverse-mode only touches the diagonal entries that actually contributed.

## Summary

Use `trace()` when you want the sum of a diagonal. A 2D `Series` gives a scalar
`Series`; higher-rank input keeps its other axes; `offset`, `dim1` and `dim2`
select which diagonal. Rectangular matrices work, and reverse-mode marks only
the contributing diagonal entries.

## See Also

- [`Series::diag`](diag.md): dual-mode diagonal extract-or-embed surface.
- [`Series::diagonal`](diagonal.md): extraction-only diagonal surface.
- [`Series::sum`](sum.md): full reduction instead of diagonal-only reduction.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="topk_pair.html">
    <span class="label">Previous</span>
    <span class="title">topk_pair</span>
  </a>
  <a class="next-link" href="tracks_slope.html">
    <span class="label">Next</span>
    <span class="title">tracks_slope</span>
  </a>
</div>
