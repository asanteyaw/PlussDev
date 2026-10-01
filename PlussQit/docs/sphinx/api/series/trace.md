<!-- pluss-hand-authored -->

# Series::trace

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::trace() const -> Series
```

## What It Does

`trace()` sums the main diagonal of a 2D `Series` and returns that sum as a
scalar `Series`.

Two parts of that contract matter in real code:

- it is a reduction, so the output is scalar-shaped
- it reads only the diagonal positions, not the full matrix

`trace()` also accepts rectangular 2D input. In that case it walks only as far
as a diagonal still exists, so the effective diagonal length is
`min(rows, cols)`.

## Parameters

None.

The method always uses the main diagonal. If you need an offset diagonal, first
extract it with [`diagonal()`](diagonal.md) or [`diag()`](diag.md), then reduce
that result yourself.

## Returns

`Series`
    A scalar `Series` containing the sum of the main diagonal.

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

So this is not a "square matrices only" method. It is a "2D matrices only"
method.

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
diag/diagonal: extraction expects a 2D input
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

## Gotchas And Failure Modes

- `trace()` returns a scalar `Series`, not a raw number.
- Rectangular matrices are valid; the diagonal length is clipped naturally.
- Rank-0, rank-1, and rank-3+ inputs fail because this method is 2D-only.
- Reverse-mode only touches the diagonal entries that actually contributed.

## Summary

Use `trace()` when you want the sum of the main diagonal of a 2D `Series`.
Expect a scalar `Series` result, expect rectangular matrices to work, and
expect reverse-mode to mark only the contributing diagonal entries.

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
