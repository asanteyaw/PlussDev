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

## Examples

```cpp
auto x = pluss::make_series({{3.0, 4.0}, {0.0, 5.0}});
std::cout << x.stat.normc() << "\n";
std::cout << x.stat.normc().square().sum(0) << "\n";
```

### Output

```text
data=[[1, 0.62469505],
  [0, 0.78086881]]
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
