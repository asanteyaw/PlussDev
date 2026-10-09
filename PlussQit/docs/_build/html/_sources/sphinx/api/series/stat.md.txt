<!-- pluss-hand-authored -->

# Series::stat

[Back to Series API Reference](../../series_api_reference.md)

## Signatures

```cpp
Series::StatAccessor Series::stat
SeriesStatProxy Series::stat() const
SeriesStatProxy Series::stat(const Series& other) const
SeriesStatProxy Series::stat(std::initializer_list<Series> others) const
SeriesStatProxy Series::stat(std::span<const Series> others) const
```

## What It Does

`stat` is the entrypoint to the complete statistical method family. Accessing
it does not copy values or perform a calculation; the terminal method selects
the operation.

Open the dedicated [Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)
for the complete method index and guided comparisons.

## Direct And Proxy Calls

Most one-series operations use direct accessor syntax:

```cpp
auto rows = x.stat.mean(1);
auto local = x.stat.movstd(5, -1);
```

Calling `stat()` first creates a proxy bound to the same owner. It is
semantically equivalent for unary methods:

```cpp
auto rows = x.stat().mean(1);
```

Binary distance methods can instead bind their peer at proxy construction:

```cpp
auto direct = left.stat.cdist(right);
auto bound = left.stat(right).cdist();
```

Both produce distances from owner rows to peer rows. Binding changes call
organization, not operand order or mathematics.

## Method Families

- Descriptive reductions: [`mean`](../series-stat/mean.md),
  [`var`](../series-stat/var.md), [`std`](../series-stat/std.md),
  [`nanvar`](../series-stat/nanvar.md), [`nanstd`](../series-stat/nanstd.md),
  [`skewness`](../series-stat/skewness.md), and
  [`kurtosis`](../series-stat/kurtosis.md).
- Normal-model evaluation: [`pdf`](../series-stat/pdf.md),
  [`logpdf`](../series-stat/logpdf.md), [`cdf`](../series-stat/cdf.md), and
  [`logcdf`](../series-stat/logcdf.md).
- Trailing windows: [`movsum`](../series-stat/movsum.md),
  [`movmean`](../series-stat/movmean.md), [`movmin`](../series-stat/movmin.md),
  [`movmax`](../series-stat/movmax.md), [`movvar`](../series-stat/movvar.md),
  and [`movstd`](../series-stat/movstd.md).
- Shape-preserving transforms: [`center`](../series-stat/center.md),
  [`standardize`](../series-stat/standardize.md),
  [`normr`](../series-stat/normr.md), and [`normc`](../series-stat/normc.md).
- Pairwise geometry: [`cdist`](../series-stat/cdist.md) and
  [`pdist2`](../series-stat/pdist2.md).

## Example

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << x.stat.mean(1) << "\n";
std::cout << x.stat.standardize(1) << "\n";
std::cout << x.stat.movmean(2, 1) << "\n";
```

### Output

```text
data=[2, 5]
data=[[-1.2247449, 0, 1.2247449],
  [-1.2247449, 0, 1.2247449]]
data=[[1, 1.5, 2.5],
  [4, 4.5, 5.5]]
```

The first call reduces rows, while the latter two preserve shape. Belonging to
the same accessor does not imply the same output geometry.

## See Also

- [Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md):
  all 23 accessor methods, each with its own page.
- [`Series::linearalg`](linearalg.md): linear-algebra accessor entrypoint.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="argmin.html"><span class="label">Previous</span><span class="title">argmin</span></a>
  <a class="next-link" href="storage_version.html"><span class="label">Next</span><span class="title">storage_version</span></a>
</div>
