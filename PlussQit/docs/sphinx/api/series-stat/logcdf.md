<!-- pluss-hand-authored -->

# SeriesStatAccessor::logcdf

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::logcdf() const
Series SeriesStatAccessor::logcdf(double sigma) const
Series SeriesStatAccessor::logcdf(double mu, double sigma) const
Series SeriesStatProxy::logcdf() const
Series SeriesStatProxy::logcdf(double sigma) const
Series SeriesStatProxy::logcdf(double mu, double sigma) const
```

## What It Does

`logcdf()` returns `log(P(X<=x))` for a normal model. It preserves information
in extreme lower tails where `cdf()` may round probability to zero before a
caller can take its logarithm.

## Keeping Tail Evidence Finite

`log(cdf(x))` is mathematically equivalent, but not numerically equivalent in
the tail. `logcdf()` evaluates that quantity with tail-aware formulas, so a
very unlikely event remains a large finite negative number rather than `-inf`.

Overloads choose standard normal, zero-mean custom `sigma`, or explicit
`mu,sigma`. `sigma` must be positive.

## Parameters And Returns

`mu` locates the model and `sigma` is its strictly positive standard
deviation. The method standardizes each owner value using those parameters and
returns a shape-preserving `float64` `Series`. It performs no reduction and
does not infer either parameter from the data.

## Gradient

The derivative is the inverse Mills ratio `pdf(x)/cdf(x)`. Reverse mode
evaluates it through log-domain quantities so lower-tail sensitivities remain
available when ordinary CDF values underflow.

## Examples

Example 1. Ordinary points are natural logs of their CDF values.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
std::cout << x.stat.logcdf() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.8410216, -0.69314718, -0.17275378]
```

Example 2. Extreme tails stay represented.

```cpp
std::cout << pluss::make_series({-20.0, -10.0, 10.0}).stat.logcdf() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-203.91716, -53.231285, -7.619853e-24]
```

The value at `-20` means the lower-tail probability is roughly `exp(-203.9)`.
Keeping the logarithm finite lets that evidence participate in later sums and
comparisons even though ordinary probability space is near its limits.

Example 3. The reverse rule remains finite in that same lower tail.

```cpp
auto x = pluss::make_series({-20.0, -10.0, 0.0});
x.tracks_slope_(true);
x.stat.logcdf().sum().reverse();
std::cout << x.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[20.049753, 10.098093, 0.79788456]
```

The derivative grows in the far lower tail because a small movement in `x`
causes a large relative change in an already tiny probability. The calculation
uses `exp(logpdf-logcdf)` rather than dividing two underflowed ordinary values.

## Gotchas And Failure Modes

- Output is a log probability and is therefore non-positive.
- Use this method directly instead of calculating `cdf().log()` in tail work.
- Large lower-tail gradients are mathematically meaningful, not an overflow by
  themselves; account for them when designing an objective.
- `sigma<=0` is invalid.

## See Also

- [`cdf`](cdf.md): accumulated probability in ordinary scale.
- [`logpdf`](logpdf.md): local density in log scale.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kurtosis.html"><span class="label">Previous</span><span class="title">kurtosis</span></a>
  <a class="next-link" href="logpdf.html"><span class="label">Next</span><span class="title">logpdf</span></a>
</div>
