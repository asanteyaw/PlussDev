# Quickstart

## Include Path

```cpp
#include <pluss/ma.h>
```

The distribution layer is exposed as `pluss::distr`. No extra include is needed when PlussMa is part of the build.

## Sample In One Line

```cpp
auto x = pluss::distr::sample("normal", {0.0, 1.0}, {1024});
auto y = pluss::distr::sample("arcsin", {1024});
auto z = pluss::distr::sample("vonmises", {0.0, 3.0}, {1024});
```

## Keep Shape And Dtype

```cpp
auto like = pluss::series::zeros({2, 4},
    pluss::Config{}
        .with_dtype(pluss::ScalarType::Float32)
        .with_platform(pluss::pCPU)
        .with_pinned(true));

auto gamma_like = pluss::distr::sample_like("gamma", {2.0}, like);
auto signs_like = pluss::distr::sample_like("rademacher", like);
```

`sample_like(...)` preserves the visible tensor contract from `like`: shape, dtype, and platform. It also forwards the pinned flag into the sampling config, but the current runtime does not surface pinned-memory state through `is_pinned()` yet.

## Evaluate Densities And CDFs

```cpp
auto grid = pluss::make_series(std::vector<double>{-2.0, -1.0, 0.0, 1.0, 2.0}, {5});

auto normal_pdf = pluss::distr::pdf("normal", grid, {0.0, 1.0});
auto t_cdf = pluss::distr::cdf("t", grid, {5.0});
auto nig_logpdf = pluss::distr::logpdf("nig", grid, {2.0, 0.5, 0.0, 1.0});
```

## Typed Wrapper For Parameter-Bound Models

```cpp
pluss::distr::Normal normal{0.0, 1.0};
pluss::distr::Bernoulli bernoulli{0.35};
pluss::distr::Gamma gamma{2.0, 1.0, 0.0};
pluss::distr::Nig nig{2.0, 0.5, 0.0, 1.0};

auto normal_logpdf = normal.logpdf(grid);
auto draws = normal.sample({1024});
auto bern_logcdf = bernoulli.logcdf(pluss::make_series({0.0, 1.0}));
auto gamma_cgf = gamma.cgf(pluss::make_series({0.0, 0.5}));
auto nig_logtail = nig.logcdf(grid);
```

## Gaussian Mixture

```cpp
auto weights = pluss::make_series(std::vector<double>{0.65, 0.35}, {2});
auto means = pluss::make_series(std::vector<double>{-1.0, 2.0}, {2});
auto stddevs = pluss::make_series(std::vector<double>{0.5, 1.25}, {2});

auto mix = pluss::distr::mixture_normal(
    weights,
    means,
    stddevs,
    {4096},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float64),
    23ULL,
    true);

pluss::distr::GaussianMixture gm{weights, means, stddevs};
auto gm_logtail = gm.logcdf(grid);
auto gm_cgf = gm.cgf(pluss::make_series({0.0, 0.1}));
```

The last argument switches the base normal generator from pseudo-random draws to Sobol-driven Gaussian draws.

## Intended Style

The user-facing model is:

- keep everything under `pluss::distr`
- pass the distribution name plus a short parameter list
- use typed wrappers like `pluss::distr::Normal`, `Bernoulli`, `Binomial`, `Gamma`, `Uniform`, `Exponential`, `Skellam`, `Nig`, and `GaussianMixture` when a bound object is clearer
- let `sample_like` carry shape and dtype when you already have a target tensor
- stay on `Series` throughout instead of bouncing between helper wrappers
