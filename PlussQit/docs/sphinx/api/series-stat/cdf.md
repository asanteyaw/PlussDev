<!-- pluss-hand-authored -->

# SeriesStatAccessor::cdf

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::cdf() const
Series SeriesStatAccessor::cdf(double sigma) const
Series SeriesStatAccessor::cdf(double mu, double sigma) const
```

## What It Does

`cdf()` evaluates `P(X<=x)` for a normal random variable at every input `x`.
Unlike `pdf()`, its output is accumulated probability and therefore lies from
zero to one.

## Reading Threshold Probability

For the standard normal, `cdf(0)=0.5`: half the mass lies at or below the mean.
Moving right increases accumulated mass; moving left decreases it. Differences
of two CDF values give probability over an interval.

The overloads use `(mu,sigma)=(0,1)`, `(0,sigma)`, or explicit values. Shape is
preserved and `sigma` must be positive.

## Gradient

The derivative of the CDF is the corresponding density:

```text
d cdf(x) / dx = pdf(x)
```

## Examples

Example 1. Standard-normal anchors show symmetry around `0.5`.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
std::cout << x.stat.cdf() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.15865525, 0.5, 0.84134475]
```

Example 2. Reverse mode recovers the density values.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
x.tracks_slope_(true);
x.stat.cdf().sum().reverse();
std::cout << x.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.24197072, 0.39894228, 0.24197072]
```

## Gotchas And Failure Modes

- `cdf()` is cumulative; `pdf()` is local curve height.
- Far-tail probabilities can round to exactly zero or one; use `logcdf()` when
  the lower tail must remain distinguishable.
- `sigma<=0` is rejected.

## See Also

- [`pdf`](pdf.md): derivative and local density.
- [`logcdf`](logcdf.md): stable logarithmic lower-tail probability.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../series_stat_accessor_api_reference.html"><span class="label">Previous</span><span class="title">SeriesStatAccessor</span></a>
  <a class="next-link" href="cdist.html"><span class="label">Next</span><span class="title">cdist</span></a>
</div>
