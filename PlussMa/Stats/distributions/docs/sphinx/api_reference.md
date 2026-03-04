# API Reference

## `supports`

### Signature

```cpp
bool pluss::distr::supports(std::string_view name) noexcept;
```

### Notes

Checks whether the runtime resolver recognizes a family name or alias before you call `sample`, `pdf`, `logpdf`, or `cdf`.

### Examples

```cpp
std::cout << pluss::distr::supports("gauss") << "\n";
std::cout << pluss::distr::supports("zipf") << "\n";
std::cout << pluss::distr::supports("not-a-dist") << "\n";
```

## `sample`

### Signature

```cpp
pluss::Series pluss::distr::sample(
    std::string_view name,
    std::span<const double> params,
    std::span<const std::int64_t> sizes,
    pluss::Config cfg = {},
    std::uint64_t seed = 0);
```

### Notes

Samples a new `Series` from the chosen family. Use the parameter list required by that distribution, pass the output shape through `sizes`, and use `cfg` when you need to control dtype or platform.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);

auto draws = pluss::distr::sample("normal", {0.0, 1.0}, {2, 3}, cfg, 20260301ULL);
```

## `sample_like`

### Signature

```cpp
pluss::Series pluss::distr::sample_like(
    std::string_view name,
    std::span<const double> params,
    const pluss::Series& like,
    std::uint64_t seed = 0);
```

### Notes

Uses the same distribution parameters as `sample(...)`, but takes shape, dtype, and platform from `like`. This is the right helper when another `Series` already defines the tensor contract you want.

### Examples

```cpp
auto like = pluss::zeros(
    {2, 4},
    pluss::Config{}
        .with_dtype(pluss::ScalarType::Float32)
        .with_platform(pluss::pCPU));

auto out = pluss::distr::sample_like("gamma", {2.0}, like, 77ULL);
```

## `pdf`

### Signature

```cpp
pluss::Series pluss::distr::pdf(
    std::string_view name,
    const pluss::Series& x,
    std::span<const double> params = {});
```

### Notes

Evaluates the probability density function for continuous families or the mass function for discrete families at every point in `x`.

### Examples

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto dens = pluss::distr::pdf("normal", x, {0.0, 1.0});
```

## `logpdf`

### Signature

```cpp
pluss::Series pluss::distr::logpdf(
    std::string_view name,
    const pluss::Series& x,
    std::span<const double> params = {});
```

### Notes

Evaluates the natural log of the density or mass function. Use it when you are building log-likelihood objectives or want to avoid manually taking `log(pdf(...))`.

### Examples

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto logdens = pluss::distr::logpdf("nig", x, {2.0, 0.5, 0.0, 1.0});
```

## `cdf`

### Signature

```cpp
pluss::Series pluss::distr::cdf(
    std::string_view name,
    const pluss::Series& x,
    std::span<const double> params = {});
```

### Notes

Evaluates the cumulative distribution function at every point in `x`. Use it when your downstream logic is expressed in tail probabilities or threshold probabilities rather than densities.

### Examples

```cpp
auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto dist = pluss::distr::cdf("t", x, {5.0});
```

## `Normal`

### Signature

```cpp
struct pluss::distr::Normal {
  double mean = 0.0;
  double stddev = 1.0;
};
```

### Notes

Typed wrappers bind parameters once and delegate to the same functional distribution engine. Start here when you want torch-like ergonomics for common distributions without giving up the catalog-style API.

### Examples

```cpp
pluss::distr::Normal normal{0.0, 1.0};

auto draws = normal.sample({8});
auto logdens = normal.logpdf(pluss::make_series({-1.0, 0.0, 1.0}));
auto logtail = normal.logcdf(pluss::make_series({-1.0, 0.0, 1.0}));
auto cgf = normal.cgf(pluss::make_series({0.0, 1.0}));
```

The same pattern now exists for `pluss::distr::Bernoulli`, `Binomial`, `Gamma`, `Uniform`, and `Exponential`.

## `mixture_normal`

### Signature

```cpp
pluss::Series pluss::distr::mixture_normal(
    const pluss::Series& weights,
    const pluss::Series& means,
    const pluss::Series& stddevs,
    std::span<const std::int64_t> sizes,
    pluss::Config cfg = {},
    std::uint64_t seed = 0,
    bool use_sobol = false);
```

### Notes

Samples from a Gaussian mixture defined by component weights, means, and standard deviations. Set `use_sobol=true` when you want Sobol-driven quasi-random normal draws instead of pseudo-random draws.

### Examples

```cpp
auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
auto weights = pluss::make_series({0.65, 0.35}, cfg);
auto means = pluss::make_series({-1.0, 2.0}, cfg);
auto stddevs = pluss::make_series({0.5, 1.25}, cfg);

auto mix = pluss::distr::mixture_normal(weights, means, stddevs, {8}, cfg, 101ULL, true);
```

## `mixture_normal_pdf`

### Signature

```cpp
pluss::Series pluss::distr::mixture_normal_pdf(
    const pluss::Series& x,
    const pluss::Series& weights,
    const pluss::Series& means,
    const pluss::Series& stddevs);
```

### Notes

Evaluates the density of a Gaussian mixture that you have already parameterized as component `Series` values.

### Examples

```cpp
auto x = pluss::make_series({-2.0, -0.5, 1.0, 3.0}).to(pluss::ScalarType::Float64);
auto weights = pluss::make_series({0.65, 0.35}).to(pluss::ScalarType::Float64);
auto means = pluss::make_series({-1.0, 2.0}).to(pluss::ScalarType::Float64);
auto stddevs = pluss::make_series({0.5, 1.25}).to(pluss::ScalarType::Float64);

auto dens = pluss::distr::mixture_normal_pdf(x, weights, means, stddevs);
```

## `mixture_normal_logpdf`

### Signature

```cpp
pluss::Series pluss::distr::mixture_normal_logpdf(
    const pluss::Series& x,
    const pluss::Series& weights,
    const pluss::Series& means,
    const pluss::Series& stddevs);
```

### Notes

Evaluates the log-density of the same Gaussian mixture parameterization used by `mixture_normal_pdf(...)`.

### Examples

```cpp
auto x = pluss::make_series({-2.0, -0.5, 1.0, 3.0}).to(pluss::ScalarType::Float64);
auto weights = pluss::make_series({0.65, 0.35}).to(pluss::ScalarType::Float64);
auto means = pluss::make_series({-1.0, 2.0}).to(pluss::ScalarType::Float64);
auto stddevs = pluss::make_series({0.5, 1.25}).to(pluss::ScalarType::Float64);

auto logdens = pluss::distr::mixture_normal_logpdf(x, weights, means, stddevs);
```

## `mixture_normal_cdf`

### Signature

```cpp
pluss::Series pluss::distr::mixture_normal_cdf(
    const pluss::Series& x,
    const pluss::Series& weights,
    const pluss::Series& means,
    const pluss::Series& stddevs);
```

### Notes

Evaluates the mixture CDF using the same component parameterization as the other Gaussian-mixture helpers.

### Examples

```cpp
auto x = pluss::make_series({-2.0, -0.5, 1.0, 3.0}).to(pluss::ScalarType::Float64);
auto weights = pluss::make_series({0.65, 0.35}).to(pluss::ScalarType::Float64);
auto means = pluss::make_series({-1.0, 2.0}).to(pluss::ScalarType::Float64);
auto stddevs = pluss::make_series({0.5, 1.25}).to(pluss::ScalarType::Float64);

auto dist = pluss::distr::mixture_normal_cdf(x, weights, means, stddevs);
```
