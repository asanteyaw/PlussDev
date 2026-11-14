<!-- pluss-hand-authored -->

# Series::StatAccessor::icdf

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::icdf() const
Series Series::StatAccessor::icdf(double sigma) const
Series Series::StatAccessor::icdf(double mu, double sigma) const
```

## What It Does

`icdf()` treats the owner as probabilities and returns the corresponding
**normal quantiles**. This Series accessor is always the inverse CDF of a
normal distribution; the owner contains probabilities and does not identify a
distribution family.

The no-argument form computes the standard-normal quantile function
`Phi^-1(p)`. The other overloads use `(mu, sigma) = (0, sigma)` or the supplied
normal location and scale.

```cpp
const pluss::Series probability = pluss::make_series({0.025, 0.5, 0.975});
const pluss::Series quantile = probability.stat.icdf();

std::cout << quantile << "\n";
```

Output:

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.95996398, 0, 1.95996398]
```

The implementation is vectorized through existing Series operations:

```text
mu + sigma * sqrt(2) * erfinv(2p - 1)
```

Probabilities `0` and `1` map to negative and positive infinity. Values outside
`[0,1]` produce invalid results, and `sigma` must be positive.

## Non-Normal Distributions

Use PlussMa's generic distribution surface when the target distribution is not
normal, or when you want the family to be explicit at the call site:

```cpp
const pluss::Series poisson_quantile = pluss::distr::icdf(
    pluss::distr::dst::Poisson,
    probability,
    {4.0});
```

Here `probability` still supplies cumulative probabilities, while
`dst::Poisson` and `{4.0}` select the Poisson family and its rate. The same
pattern applies to every family supported by the generic distribution layer.

## See Also

- [`cdf`](cdf.md): maps normal values to lower-tail probabilities.
- PlussMa `distr::icdf(...)`: generic inverse CDF dispatch across distribution families.
