<!-- pluss-hand-authored -->

# SeriesStatAccessor::pdf

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::pdf() const
Series SeriesStatAccessor::pdf(double sigma) const
Series SeriesStatAccessor::pdf(double mu, double sigma) const
Series SeriesStatProxy::pdf() const
Series SeriesStatProxy::pdf(double sigma) const
Series SeriesStatProxy::pdf(double mu, double sigma) const
```

## What It Does

`pdf()` evaluates the normal probability density at every input value. It does
not estimate a distribution from the `Series`; `mu` and `sigma` describe the
normal distribution against which each element is evaluated.

## Height, Not Probability

A density is curve height per unit of the input axis. It can compare how
plausible nearby values are under a model, but it is not the probability of an
exact continuous value. Use `cdf()` for probability accumulated up to a
threshold.

## Overload Map

- `pdf()` uses `mu=0`, `sigma=1`.
- `pdf(sigma)` uses `mu=0` and the supplied positive scale.
- `pdf(mu,sigma)` uses the supplied center and positive scale.

All forms preserve shape and return `float64`. `sigma<=0` is rejected.

## Parameters And Returns

`mu` is the distribution mean: moving it translates the density peak without
changing its shape. `sigma` is the strictly positive standard deviation:
increasing it spreads the same total probability over a wider interval and
therefore lowers the peak. The returned `float64` `Series` has exactly the
owner's shape; this is elementwise model evaluation, not a reduction.

## Gradient

For density `f(x)`, reverse mode uses
`f'(x)=-(x-mu)f(x)/sigma^2`. Sensitivity is positive left of the mean, zero at
the mean, and negative to its right.

## Examples

Example 1. The standard normal is highest at zero and symmetric.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
std::cout << x.stat.pdf() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.24197072, 0.39894228, 0.24197072]
```

Example 2. A shifted, wider normal has its peak at `mu=1`.

```cpp
auto x = pluss::make_series({-1.0, 1.0, 3.0});
std::cout << x.stat.pdf(1.0, 2.0) << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.12098536, 0.19947114, 0.12098536]
```

Example 3. Reverse mode differentiates the density itself.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0});
x.tracks_slope_(true);
x.stat.pdf().sum().reverse();
std::cout << x.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.24197072, -0, -0.24197072]
```

## Gotchas And Failure Modes

- `pdf()` evaluates a supplied normal model; it does not fit `mu` or `sigma`.
- Density values are not cumulative probabilities.
- `sigma` is a standard deviation and must be strictly positive.
- A wider distribution may assign greater density in a far tail while still
  having a lower peak; density comparisons depend on the evaluation point.

## See Also

- [`logpdf`](logpdf.md): the same density in log space.
- [`cdf`](cdf.md): probability accumulated through each input threshold.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normr.html"><span class="label">Previous</span><span class="title">normr</span></a>
  <a class="next-link" href="pdist2.html"><span class="label">Next</span><span class="title">pdist2</span></a>
</div>
