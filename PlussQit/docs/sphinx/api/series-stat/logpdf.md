<!-- pluss-hand-authored -->

# Series::StatAccessor::logpdf

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::logpdf() const
Series Series::StatAccessor::logpdf(double sigma) const
Series Series::StatAccessor::logpdf(double mu, double sigma) const
Series SeriesStatProxy::logpdf() const
Series SeriesStatProxy::logpdf(double sigma) const
Series SeriesStatProxy::logpdf(double mu, double sigma) const
```

## What It Does

`logpdf()` returns the natural logarithm of normal density at each input. It is
the additive, numerically safer form used in likelihood calculations.

## Why Likelihoods Live In Log Space

Multiplying many small densities can underflow to zero. Summing log densities
represents the same product without repeatedly shrinking a floating-point
number. Differences in `logpdf()` also become log-likelihood ratios.

The overloads choose `(mu,sigma)` exactly as `pdf()` does: default `(0,1)`,
one argument as `(0,sigma)`, or two arguments explicitly. Shape is preserved,
output is `float64`, and `sigma` must be positive.

## Parameters And Returns

`mu` translates the center of the model. `sigma` is its strictly positive
standard deviation; it affects both the normalization constant and the
squared-distance penalty. The result is a shape-preserving `float64` `Series`.
No parameters are estimated from the owner.

## Gradient

The derivative is particularly simple:

```text
d logpdf(x) / dx = -(x-mu)/sigma^2
```

It points toward the distribution mean and avoids multiplying by a tiny tail
density.

## Examples

Example 1. Ordinary density ordering becomes additive log-density ordering.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
std::cout << x.stat.logpdf() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.4189385, -0.91893853, -1.4189385]
```

The center has the greatest log density. Tail values are more negative, and
adding these values across independent observations forms a log-likelihood.

Example 2. Reverse mode follows the simple score function toward the mean.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
x.tracks_slope_(true);
x.stat.logpdf().sum().reverse();
std::cout << x.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, -0, -1]
```

Each unit of displacement from `mu=0` produces an equal and opposite score.
With `sigma=2`, the same displacement would produce one quarter of this
magnitude because the derivative divides by `sigma^2`.

## Gotchas And Failure Modes

- Outputs are log densities, not probabilities and not density values.
- `logpdf()` remains finite much farther into the tails than `pdf()`.
- Summing log densities corresponds to multiplying densities only when the
  modeled observations are conditionally independent.
- `sigma<=0` is invalid.

## See Also

- [`pdf`](pdf.md): density in ordinary scale.
- [`logcdf`](logcdf.md): accumulated probability in log scale.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logcdf.html"><span class="label">Previous</span><span class="title">logcdf</span></a>
  <a class="next-link" href="mean.html"><span class="label">Next</span><span class="title">mean</span></a>
</div>
