<!-- pluss-hand-authored -->

# Series::StatAccessor API Reference

[Back to Series API Reference](series_api_reference.md)

`Series::StatAccessor` groups statistical reductions, normal-distribution
functions, trailing-window calculations, normalization transforms, and
pairwise distances. Start here when the question is statistical rather than a
generic elementwise or shape operation.

## Calling Styles

Unary methods have two equivalent spellings:

```cpp
auto a = x.stat.mean(0);
auto b = x.stat().mean(0);
```

The first calls the accessor directly. The second creates a `SeriesStatProxy`
bound to `x`, then calls the same implementation. Prefer direct style for
ordinary code; recognize proxy style because it becomes useful for binary
methods:

```cpp
auto direct = left.stat.cdist(right);
auto bound = left.stat(right).cdist();
```

Here `stat(right)` binds the peer before the terminal method is selected. The
owner `left` remains the first point table in either spelling.

## Describing A Population

- [`mean`](api/series-stat/mean.md) summarizes location globally or by axis.
- [`var`](api/series-stat/var.md) measures squared spread with sample or
  population normalization.
- [`std`](api/series-stat/std.md) reports the same spread in original units.
- [`nanvar`](api/series-stat/nanvar.md) and
  [`nanstd`](api/series-stat/nanstd.md) omit NaNs under an explicit
  missing-observation policy.
- [`skewness`](api/series-stat/skewness.md) measures asymmetry.
- [`kurtosis`](api/series-stat/kurtosis.md) reports excess fourth-moment shape.

Axis choice defines the population being summarized. On a matrix, `dim=0`
groups values by column and `dim=1` groups them by row. A global statistic pools
all entries and generally cannot be reconstructed by averaging lane-wise
statistics.

## Evaluating A Normal Model

- [`pdf`](api/series-stat/pdf.md) gives local density.
- [`logpdf`](api/series-stat/logpdf.md) gives log density.
- [`cdf`](api/series-stat/cdf.md) gives accumulated lower-tail probability.
- [`logcdf`](api/series-stat/logcdf.md) preserves very small lower-tail values
  in log space.

All four preserve shape and support standard normal, scale-only, and explicit
`(mu,sigma)` overloads. These methods evaluate a chosen normal model; they do
not estimate `mu` or `sigma` from the owner.

## Following A Trailing Window

- [`movsum`](api/series-stat/movsum.md) and
  [`movmean`](api/series-stat/movmean.md) track local level.
- [`movmin`](api/series-stat/movmin.md) and
  [`movmax`](api/series-stat/movmax.md) track local bounds.
- [`movvar`](api/series-stat/movvar.md) and
  [`movstd`](api/series-stat/movstd.md) track local population spread.

Every moving method preserves shape, includes the current position, looks
backward, and uses the available prefix before a full window exists. Omitted
arguments mean window three on the last axis.

## Transforming Statistical Geometry

- [`center`](api/series-stat/center.md) subtracts a global or lane mean.
- [`standardize`](api/series-stat/standardize.md) produces population z-scores.
- [`normr`](api/series-stat/normr.md) gives nonzero rows unit L2 length.
- [`normc`](api/series-stat/normc.md) gives nonzero columns unit L2 length.

These operations all preserve shape, but they solve different problems.
Centering removes offset, standardization removes offset and spread, and norm
operations remove vector magnitude without subtracting a mean.

## Comparing Point Tables

- [`cdist`](api/series-stat/cdist.md) computes all Euclidean row-to-row
  distances between two rank-2 point tables.
- [`pdist2`](api/series-stat/pdist2.md) provides the equivalent two-table
  pairwise-distance surface; the `2` does not mean squared distance.

For owner shape `[m,d]` and peer shape `[n,d]`, both return `[m,n]` and
differentiate with respect to both point tables.

```{toctree}
:hidden:
:maxdepth: 1

api/series-stat/cdf
api/series-stat/cdist
api/series-stat/center
api/series-stat/kurtosis
api/series-stat/logcdf
api/series-stat/logpdf
api/series-stat/mean
api/series-stat/movmax
api/series-stat/movmean
api/series-stat/movmin
api/series-stat/movstd
api/series-stat/movsum
api/series-stat/movvar
api/series-stat/nanstd
api/series-stat/nanvar
api/series-stat/normc
api/series-stat/normr
api/series-stat/pdf
api/series-stat/pdist2
api/series-stat/skewness
api/series-stat/standardize
api/series-stat/std
api/series-stat/var
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="api/series/stat.html"><span class="label">Previous</span><span class="title">stat</span></a>
  <a class="next-link" href="api/series-stat/cdf.html"><span class="label">Next</span><span class="title">cdf</span></a>
</div>
