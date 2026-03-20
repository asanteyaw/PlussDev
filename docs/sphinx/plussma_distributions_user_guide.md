# PlussMa Distributions User Guide

This guide covers the full PlussMa distributions surface:

- functional API (`pluss::distr`)
- object-based API (`pluss::distr::<Distribution>`)
- MLE fit result objects (`pluss::distr::<Distribution>Fit`)

## Include And Namespace

### API Signature

- `#include <pluss/ma.h>`
- `namespace pdt = pluss::distr`
- `namespace dst = pluss::subsp::stats::distr::dst`

### Notes

With `#include <pluss/ma.h>`, distributions are available from `pluss::distr`.

### Inputs

- PlussMa in your build

### Output

- functional and object-based distribution APIs in one include

### Options

- use explicit `Config` for dtype/platform control

### Examples

```cpp
#include <pluss/ma.h>

namespace pdt = pluss::distr;
namespace dst = pluss::subsp::stats::distr::dst;
```

### Expected output

- distribution symbols resolve without extra includes.

## Functional API (Core)

### API Signature

- `supports(kind: dst::Kind) -> bool`
- `sample(kind: dst::Kind, params: span<double>|init_list<double>, sizes: span<int64>|init_list<int64>, cfg: Config = {}, seed: uint64 = 0, method: rng::Method = rng::Method::Pseudo) -> Series`
- `sample(kind: dst::Kind, sizes: span<int64>|init_list<int64>, cfg: Config = {}, seed: uint64 = 0, method: rng::Method = rng::Method::Pseudo) -> Series`
- `sample_like(kind: dst::Kind, params: span<double>|init_list<double>, like: Series, seed: uint64 = 0, method: rng::Method = rng::Method::Pseudo) -> Series`
- `sample_like(kind: dst::Kind, like: Series, seed: uint64 = 0, method: rng::Method = rng::Method::Pseudo) -> Series`
- `pdf(kind: dst::Kind, x: Series, params: span<double>|init_list<double> = {}) -> Series`
- `logpdf(kind: dst::Kind, x: Series, params: span<double>|init_list<double> = {}) -> Series`
- `cdf(kind: dst::Kind, x: Series, params: span<double>|init_list<double> = {}) -> Series`
- `logcdf(kind: dst::Kind, x: Series, params: span<double>|init_list<double> = {}) -> Series`

### Notes

This is the widest API surface and covers all supported distribution kinds.

### Inputs

- `dst::Kind` family selector
- family parameters
- output shape or `like` tensor

### Output

- sampled or evaluated `Series`

### Options

- `rng::Method`: `Pseudo`, `Sobol`, `Halton`, `Torus`

### Examples

```cpp
using namespace pluss;

const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
const Series x = make_series(std::vector<double>{-2.0, -1.0, 0.0, 1.0, 2.0}, {5}, f64);

const Series draws = pdt::sample(dst::Normal, {0.0, 1.0}, {1024}, f64, 7ULL);
const Series draws_like = pdt::sample_like(dst::Gamma, {2.0}, zeros({2, 4}, f64), 11ULL);
const Series dens = pdt::pdf(dst::Normal, x, {0.0, 1.0});
const Series logdens = pdt::logpdf(dst::Nig, x, {2.0, 0.5, 0.0, 1.0});
const Series dist = pdt::cdf(dst::StudentT, x, {5.0});
const Series logdist = pdt::logcdf(dst::Normal, x, {0.0, 1.0});
```

### Expected output

- `draws`/`draws_like` contain random samples.
- `dens`, `logdens`, `dist`, `logdist` contain elementwise evaluation on `x`.

## Gaussian Mixture (Functional)

### API Signature

- `mixture_normal(weights: Series, means: Series, stddevs: Series, sizes: span<int64>|init_list<int64>, cfg: Config = {}, seed: uint64 = 0, use_sobol: bool = false) -> Series`
- `mixture_normal_pdf(x: Series, weights: Series, means: Series, stddevs: Series) -> Series`
- `mixture_normal_logpdf(x: Series, weights: Series, means: Series, stddevs: Series) -> Series`
- `mixture_normal_cdf(x: Series, weights: Series, means: Series, stddevs: Series) -> Series`
- `mixture_normal_logcdf(x: Series, weights: Series, means: Series, stddevs: Series) -> Series`

### Notes

This is a dedicated functional surface for normal-mixture sampling/evaluation.

### Inputs

- `weights`, `means`, `stddevs` (aligned component vectors)
- output size and config

### Output

- sampled mixture draws or mixture density/cdf evaluations

### Options

- `use_sobol` switches component-normal draws from pseudo-random to Sobol-driven draws

### Examples

```cpp
const Series weights = pluss::make_series(std::vector<double>{0.65, 0.35}, {2});
const Series means = pluss::make_series(std::vector<double>{-1.0, 2.0}, {2});
const Series stddevs = pluss::make_series(std::vector<double>{0.5, 1.25}, {2});
const Series x = pluss::make_series(std::vector<double>{-2.0, -0.5, 1.0, 3.0}, {4});

const Series mix = pdt::mixture_normal(weights, means, stddevs, {2048}, {}, 23ULL, true);
const Series mix_pdf = pdt::mixture_normal_pdf(x, weights, means, stddevs);
const Series mix_cdf = pdt::mixture_normal_cdf(x, weights, means, stddevs);
```

### Expected output

- `mix` contains Gaussian-mixture samples.
- `mix_pdf` and `mix_cdf` evaluate the same mixture at query points.

## Object-Based API (Typed Models)

### API Signature

For each object type:

- `sample(sizes, cfg = {}, seed = 0) -> Series`
- `sample_like(like, seed = 0) -> Series`
- `pdf(x) -> Series`
- `logpdf(x) -> Series`
- `cdf(x) -> Series`
- `logcdf(x) -> Series`
- `cgf(t) -> Series`
- `cumulant(order: int64) -> Series`

Object types:

- `Normal`
- `Bernoulli`
- `Binomial`
- `Poisson`
- `Geometric`
- `NegativeBinomial`
- `Gamma`
- `ChiSquare`
- `InverseGaussian`
- `Lognormal`
- `Uniform`
- `Exponential`
- `Laplace`
- `Logistic`
- `Rayleigh`
- `NormalTrunc`
- `StudentT`
- `FDistribution`
- `Weibull`
- `Beta`
- `Skellam`
- `Nig`
- `Gh`
- `GaussianMixture`

### Notes

Use object wrappers when a distribution with fixed parameters is reused repeatedly.

### Inputs

- constructor parameters for the selected object
- evaluation/sampling inputs

### Output

- sampled and evaluated `Series`

### Options

- same output-shape/config/seed controls as functional sampling

### Examples

```cpp
const pdt::Normal normal{0.0, 1.0};
const pdt::Bernoulli bernoulli{0.35};
const pdt::Gamma gamma{2.0, 1.0, 0.0};

const Series x = pluss::make_series(std::vector<double>{-1.0, 0.0, 1.0}, {3});
const Series t = pluss::make_series(std::vector<double>{-0.25, 0.0, 0.25}, {3});

const Series normal_draws = normal.sample({512}, {}, 7ULL);
const Series normal_logpdf = normal.logpdf(x);
const Series normal_cgf = normal.cgf(t);
const Series bern_logcdf = bernoulli.logcdf(pluss::make_series(std::vector<double>{0.0, 1.0}, {2}));
const Series gamma_cumulant2 = gamma.cumulant(2);
```

### Expected output

- object methods return the same semantics as functional API with bound parameters.

## Object MLE Surface

### API Signature

Object MLE method:

- `<Distribution>::MLE(x: Series) -> <Distribution>Fit`

Fit result methods (shared pattern):

- `model() -> <Distribution>`
- `loglik(x) -> Series`
- `loglik_per_obs(x) -> Series`
- `nll(x) -> Series`
- `score(x) -> Series`
- `fisher_inf(x) -> Series`
- `observed_inf(x) -> Series`
- `vcov(x) -> Series`
- `stderr(x) -> Series`
- `confint(x, level = 0.95) -> Series`

Fit result types:

- `NormalFit`
- `ExponentialFit`
- `PoissonFit`
- `BernoulliFit`
- `BinomialFit`
- `GeometricFit`
- `LognormalFit`
- `UniformFit`
- `RayleighFit`
- `LaplaceFit`
- `InverseGaussianFit`
- `GammaFit`
- `BetaFit`
- `NegativeBinomialFit`
- `NigFit`
- `GhFit`
- `GaussianMixtureFit`

### Notes

MLE is exposed from the object API and returns fit objects with inference diagnostics.

### Inputs

- observed data series `x`

### Output

- fitted parameters + diagnostics methods

### Options

- `confint(level)` confidence level, default `0.95`

### Examples

```cpp
const Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4});
const auto nfit = pdt::Normal{0.0, 1.0}.MLE(x);

const double mean_hat = nfit.mean;
const double std_hat = nfit.stddev;
const Series ll = nfit.loglik(x);
const Series score = nfit.score(x);
const Series info = nfit.fisher_inf(x);
const Series ci = nfit.confint(x, 0.95);
```

### Expected output

- fit object exposes both estimated parameters and standard inference diagnostics.

## Functional Kind Coverage

### API Signature

- `dst::Kind` enum family selector

### Notes

The functional API supports these distribution kinds:

- `Alpha`, `Anglit`, `Arcsin`, `Bessel`, `Normal`, `Lognormal`, `Uniform`, `Exponential`, `Bernoulli`, `Beta`, `Binomial`, `Bradford`, `Burr`, `Cauchy`, `Chi`, `ChiSquare`, `ChiSquareNoncentral`, `Cosine`, `Erlang`, `ExtremeValue`, `F`, `FNoncentral`, `Laplace`, `Logistic`, `Geometric`, `Gamma`, `Gompertz`, `HalfCosine`, `HyperbolicSecant`, `Hypergeometric`, `InverseGaussian`, `Maxwell`, `Nakagami`, `NegativeBinomial`, `NormalTrunc`, `Planck`, `Poisson`, `Quadratic`, `Rademacher`, `StudentT`, `Triangular`, `Pareto`, `Lomax`, `Rayleigh`, `Rice`, `Semicircle`, `Skellam`, `TukeyLambda`, `UShape`, `VonMises`, `Wald`, `Weibull`, `Yule`, `Zeta`, `Gig`, `Gh`, `Nig`

### Inputs

- selected kind + valid parameterization for that family

### Output

- support check (`supports`), samples, density/CDF evaluations

### Options

- choose object wrappers when you need bound-parameter model reuse

### Examples

```cpp
const bool ok_normal = pdt::supports(dst::Normal);
const bool ok_zeta = pdt::supports(dst::Zeta);
```

### Expected output

- `supports(...)` returns `true` for implemented enum values.

## Reproducibility, Dtype, And Platform

### API Signature

- `sample(..., cfg, seed, method)`
- `sample_like(..., like, seed, method)`

### Notes

- Seed controls reproducibility.
- `cfg` controls dtype/platform for `sample`.
- `sample_like` inherits shape/dtype/platform from `like`.

### Inputs

- fixed seed, explicit config, or `like` reference tensor

### Output

- deterministic draw sequences for fixed method/seed/options

### Options

- use `ScalarType::Float64` for numerically sensitive density/CDF evaluation

### Examples

```cpp
const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
const Series a = pdt::sample(dst::Normal, {0.0, 1.0}, {16}, f64, 123ULL, pluss::rng::Method::Pseudo);
const Series b = pdt::sample(dst::Normal, {0.0, 1.0}, {16}, f64, 123ULL, pluss::rng::Method::Pseudo);
```

### Expected output

- `a` and `b` match under identical settings.

