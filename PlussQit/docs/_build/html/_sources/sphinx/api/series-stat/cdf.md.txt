<!-- pluss-hand-authored -->

# Series::StatAccessor::cdf

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::cdf() const
Series Series::StatAccessor::cdf(double sigma) const
Series Series::StatAccessor::cdf(double mu, double sigma) const
Series SeriesStatProxy::cdf() const
Series SeriesStatProxy::cdf(double sigma) const
Series SeriesStatProxy::cdf(double mu, double sigma) const
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

## Parameters And Returns

`mu` is the threshold at which accumulated probability is one half. `sigma`
is the strictly positive scale controlling how quickly the curve transitions
from zero toward one. Evaluation is elementwise, preserves the owner's shape,
and returns `float64`; it does not reduce the data or infer a distribution.

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

The first and third probabilities add to one because `-1` and `1` are equally
far from the standard-normal mean on opposite sides.

Example 2. Translating and widening the model changes how raw inputs become
standardized thresholds.

```cpp
auto x = pluss::make_series({-1.0, 1.0, 3.0});
std::cout << x.stat.cdf(1.0, 2.0) << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.15865525, 0.5, 0.84134475]
```

Subtracting `mu=1` and dividing by `sigma=2` maps these inputs back to
`[-1,0,1]`, explaining why the probabilities match Example 1.

Example 3. Reverse mode recovers the density values.

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
- Interval probability is `cdf(high)-cdf(low)` for `low<=high`; individual CDF
  values are probabilities from negative infinity through one threshold.

## See Also

- [`pdf`](pdf.md): derivative and local density.
- [`logcdf`](logcdf.md): stable logarithmic lower-tail probability.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../series_stat_accessor_api_reference.html"><span class="label">Previous</span><span class="title">Series::StatAccessor</span></a>
  <a class="next-link" href="cdist.html"><span class="label">Next</span><span class="title">cdist</span></a>
</div>
