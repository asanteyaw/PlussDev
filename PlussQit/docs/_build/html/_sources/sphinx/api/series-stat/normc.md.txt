<!-- pluss-hand-authored -->

# SeriesStatAccessor::normc

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::normc() const
Series SeriesStatProxy::normc() const
```

## What It Does

`normc()` scales each column to Euclidean length one. For matrix column `c`, it
computes `c / ||c||_2`; zero columns stay zero. This is useful when columns are
feature vectors whose direction matters but raw magnitude should not.

For a vector, there is only one lane, so `normc()` and `normr()` both normalize
the whole vector. Their distinction becomes meaningful at rank two and above.

## Returns And Call Styles

The output is floating-point, out-of-place, and shape-preserving. The direct
`x.stat.normc()` and explicit proxy `x.stat().normc()` calls select the same
operation.

## Examples

Example 1. Columns are normalized independently.

```cpp
auto x = pluss::make_series({{3.0, 4.0}, {0.0, 5.0}});
std::cout << x.stat.normc() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 0.62469505],
  [0, 0.78086881]]
```

Column zero divides by `3`, while column one divides by `sqrt(41)`. This is
why the same matrix produces different numbers under `normr()`.

Example 2. Summing squared components down `dim=0` verifies unit column norms.

```cpp
std::cout << x.stat.normc().square().sum(0) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

Column zero has norm `3`; column one has norm `sqrt(41)`. Dividing by those
separate lengths produces the displayed values.

## Choosing Rows Or Columns

Use `normr()` when each row is an observation or embedding to normalize as a
unit. Use `normc()` when each column is the independent vector. The operation
is identical mathematically; only lane membership changes.

## Gotchas And Failure Modes

- `normc()` does not remove column means.
- Zero columns remain zero and cannot become unit vectors.
- Transposing, applying `normr()`, and transposing back expresses the same geometry.

## See Also

- [`normr`](normr.md): row-wise Euclidean normalization.
- [`center`](center.md): remove offsets without changing spread.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nanvar.html"><span class="label">Previous</span><span class="title">nanvar</span></a>
  <a class="next-link" href="normr.html"><span class="label">Next</span><span class="title">normr</span></a>
</div>
