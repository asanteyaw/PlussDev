# PlussMa Distributions Cookbook

This cookbook is task-first and covers both functional and object-based distribution workflows.

## Recipe 1: Minimal Functional Sampling

### API Signature

- `sample(kind, params, sizes, cfg = {}, seed = 0, method = rng::Method::Pseudo) -> Series`

### Notes

Use this as the baseline entrypoint for all functional sampling.

### Inputs

- `dst::Kind`
- parameter list
- output shape

### Output

- sampled `Series`

### Options

- `cfg` for dtype/platform
- `seed` for reproducibility

### Examples

```cpp
using namespace pluss;
namespace pdt = pluss::distr;
namespace dst = pluss::subsp::stats::distr::dst;

const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
const Series draws = pdt::sample(dst::Normal, {0.0, 1.0}, {1024}, f64, 7ULL);
```

### Expected output

- `draws` has shape `{1024}` and normal-like values.

## Recipe 2: Functional Evaluation (`pdf`, `logpdf`, `cdf`, `logcdf`)

### API Signature

- `pdf(kind, x, params) -> Series`
- `logpdf(kind, x, params) -> Series`
- `cdf(kind, x, params) -> Series`
- `logcdf(kind, x, params) -> Series`

### Notes

Use these for likelihood, scoring, and probability-threshold tasks.

### Inputs

- query series `x`
- family parameters

### Output

- elementwise evaluation series

### Options

- use `logpdf`/`logcdf` for more stable extreme-tail workflows

### Examples

```cpp
const Series x = make_series(std::vector<double>{-2.0, -1.0, 0.0, 1.0, 2.0}, {5});
const Series dens = pdt::pdf(dst::Normal, x, {0.0, 1.0});
const Series logdens = pdt::logpdf(dst::Normal, x, {0.0, 1.0});
const Series dist = pdt::cdf(dst::StudentT, x, {5.0});
const Series logdist = pdt::logcdf(dst::Normal, x, {0.0, 1.0});
const Series logdens_series = pdt::logpdf(dst::Normal, x, pluss::sign_list(zeros_like(x), ones_like(x)));
const Series logdens_series_braces = pdt::logpdf(dst::Normal, x, {zeros_like(x), ones_like(x)});
```

### Expected output

- `dens` positive; `logdens` finite for valid inputs; `dist` and `logdist` monotone in `x`.

### Series Param Recipe

```cpp
const Series x = make_series(std::vector<double>{-1.0, 0.0, 1.0}, {3});
const Series lo = make_series(std::vector<double>{-1.0, -0.5, 0.0}, {3});
const Series hi = make_series(std::vector<double>{1.0, 0.5, 2.0}, {3});

const Series u = pdt::pdf(dst::Uniform, x, {lo, hi});
const Series e = pdt::logpdf(dst::Exponential, x.abs(), {make_series(std::vector<double>{1.0, 2.0, 0.5}, {3})});
const Series l = pdt::logpdf(dst::Logistic, x, pluss::sign_list(zeros_like(x), ones_like(x)));
const Series ga = pdt::logpdf(dst::Gamma, x.abs().add(0.5), pluss::sign_list(zeros_like(x), ones_like(x),
                                                                     make_series(std::vector<double>{2.0, 3.0, 4.0}, {3})));
const Series be = pdt::logpdf(dst::Beta, x.add(1.0).div(2.0), pluss::sign_list(zeros_like(x), ones_like(x),
                                                                   make_series(std::vector<double>{2.0, 3.0, 4.0}, {3}),
                                                                   make_series(std::vector<double>{5.0, 2.0, 1.5}, {3})));
const Series chi = pdt::logpdf(dst::ChiSquare, x.abs().add(1.0), {make_series(std::vector<double>{1.0, 2.0, 5.0}, {3})});
const Series g = pdt::logpdf(dst::Geometric, make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}), {make_series(std::vector<double>{0.2, 0.3, 0.4}, {3})});
const Series nb = pdt::logpdf(dst::NegativeBinomial,
                              make_series(std::vector<double>{0.0, 1.0, 2.0}, {3}),
                              pluss::sign_list(make_series(std::vector<double>{2.0, 3.0, 4.0}, {3}),
                                               make_series(std::vector<double>{0.3, 0.4, 0.5}, {3})));
const Series h = pdt::pdf(dst::Hypergeometric,
                          make_series(std::vector<double>{0.0, 1.0, 2.0}, {3}),
                          pluss::sign_list(make_series(std::vector<double>{20.0, 20.0, 20.0}, {3}),
                                           make_series(std::vector<double>{7.0, 7.0, 7.0}, {3}),
                                           make_series(std::vector<double>{5.0, 5.0, 5.0}, {3})));
const Series gh = pdt::logpdf(dst::Gh, x, pluss::sign_list(make_series(std::vector<double>{-0.4, -0.3, -0.2}, {3}),
                                                            make_series(std::vector<double>{2.0, 2.1, 2.2}, {3}),
                                                            make_series(std::vector<double>{0.3, 0.2, 0.1}, {3}),
                                                            make_series(std::vector<double>{0.0, 0.1, 0.2}, {3}),
                                                            make_series(std::vector<double>{1.0, 1.1, 1.2}, {3})));
```

## Recipe 3: `sample_like` For Shape/Dtype/Platform Inheritance

### API Signature

- `sample_like(kind, params, like, seed = 0, method = rng::Method::Pseudo) -> Series`
- `sample_like(kind, like, seed = 0, method = rng::Method::Pseudo) -> Series`

### Notes

Preferred when your tensor contract is already defined by another series.

### Inputs

- `like` series
- distribution selection and parameters

### Output

- sampled series matching `like` contract

### Options

- `method` can switch to quasi-random families

### Examples

```cpp
const Series like = zeros({2, 4}, Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU));
const Series gamma_like = pdt::sample_like(dst::Gamma, {2.0}, like, 19ULL);
const Series sign_like = pdt::sample_like(dst::Rademacher, like, 21ULL);
```

### Expected output

- output sizes match `{2, 4}`, dtype and platform match `like`.

## Recipe 4: Quasi-Random Distribution Sampling

### API Signature

- `sample(..., method = rng::Method::Sobol|Halton|Torus) -> Series`
- `sample_like(..., method = rng::Method::Sobol|Halton|Torus) -> Series`

### Notes

Use quasi-random methods for lower-discrepancy sampling patterns.

### Inputs

- same as baseline sampling plus method choice

### Output

- distribution draws generated with selected randomization family

### Options

- `rng::Method::Pseudo`
- `rng::Method::Sobol`
- `rng::Method::Halton`
- `rng::Method::Torus`

### Examples

```cpp
const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
const Series n_sobol = pdt::sample(dst::Normal, {0.0, 1.0}, {256}, f64, 123ULL, pluss::subsp::stats::rng::Method::Sobol);
const Series u_torus = pdt::sample(dst::Uniform, {0.0, 1.0}, {256}, f64, 77ULL, pluss::subsp::stats::rng::Method::Torus);
```

### Expected output

- same output shape and semantics as pseudo-random sampling with a different draw engine.

## Recipe 5: Functional Gaussian Mixture

### API Signature

- `mixture_normal(...) -> Series`
- `mixture_normal_pdf(...) -> Series`
- `mixture_normal_logpdf(...) -> Series`
- `mixture_normal_cdf(...) -> Series`
- `mixture_normal_logcdf(...) -> Series`

### Notes

Use these helpers when the model is a Gaussian mixture without needing object wrappers.

### Inputs

- component weights/means/stddevs
- sample sizes or query points

### Output

- sampled or evaluated mixture outputs

### Options

- `use_sobol` for Sobol-driven mixture sampling

### Examples

```cpp
const Series weights = make_series(std::vector<double>{0.65, 0.35}, {2});
const Series means = make_series(std::vector<double>{-1.0, 2.0}, {2});
const Series stddevs = make_series(std::vector<double>{0.5, 1.25}, {2});
const Series x = make_series(std::vector<double>{-2.0, -0.5, 1.0, 3.0}, {4});

const Series mix = pdt::mixture_normal(weights, means, stddevs, {1024}, {}, 23ULL, true);
const Series mix_pdf = pdt::mixture_normal_pdf(x, weights, means, stddevs);
const Series mix_logcdf = pdt::mixture_normal_logcdf(x, weights, means, stddevs);
```

### Expected output

- coherent mixture sampling and density/CDF evaluations under one parameterization.

## Recipe 6: Object Wrapper Baseline (`Normal`)

### API Signature

- `Normal{mean, stddev}`
- `Normal::sample(...)`
- `Normal::pdf/logpdf/cdf/logcdf/cgf/cumulant(...)`

### Notes

Use object wrappers when model parameters are stable across repeated calls.

### Inputs

- object constructor parameters
- sampling/evaluation inputs

### Output

- same output types as functional API

### Options

- object methods still accept `cfg` and `seed` where relevant

### Examples

```cpp
const pdt::Normal normal{0.0, 1.0};
const Series x = make_series(std::vector<double>{-1.0, 0.0, 1.0}, {3});
const Series t = make_series(std::vector<double>{-0.25, 0.0, 0.25}, {3});

const Series draws = normal.sample({64}, {}, 20260303ULL);
const Series logpdf = normal.logpdf(x);
const Series cgf = normal.cgf(t);
const Series k2 = normal.cumulant(2);
```

### Expected output

- compact parameter-bound calls with normal sampling and analytic moments/cumulants.

## Recipe 7: Object Wrappers For Discrete Families

### API Signature

- `Bernoulli`, `Binomial`, `Poisson`, `Geometric`, `NegativeBinomial`, `Skellam`

### Notes

Discrete-family object wrappers are convenient in repeated PMF/CDF loops.

### Inputs

- family-specific constructor parameters

### Output

- PMF/logPMF and CDF/logCDF outputs as `Series`

### Options

- `cumulant(order)` available for all these wrappers

### Examples

```cpp
const Series x = make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {4});
const pdt::Bernoulli bern{0.25};
const pdt::Binomial binom{4, 0.25};
const pdt::Poisson pois{2.5};

const Series bern_pmf = bern.pdf(x);
const Series binom_cdf = binom.cdf(x);
const Series pois_logpmf = pois.logpdf(x);
```

### Expected output

- discrete mass and cumulative evaluations over integer-like support points.

## Recipe 8: Heavy-Tail And Special Objects

### API Signature

- `Nig`, `Gh`, `StudentT`, `FDistribution`, `NormalTrunc`, `Weibull`, `Beta`, `Logistic`, `Laplace`, `Rayleigh`

### Notes

These wrappers keep advanced parameter sets explicit and reusable.

### Inputs

- constructor parameter sets per family

### Output

- sampling + density/CDF + cumulant pipelines

### Options

- use `logpdf`/`logcdf` for tail-sensitive workflows

### Examples

```cpp
const Series x = make_series(std::vector<double>{-2.0, -0.5, 0.0, 1.0, 2.0}, {5});
const pdt::Nig nig{2.0, 0.5, 0.0, 1.0};
const pdt::Gh gh{1.0, 2.0, 0.5, 0.0, 1.0};

const Series nig_logpdf = nig.logpdf(x);
const Series gh_cdf = gh.cdf(x);
```

### Expected output

- stable heavy-tail evaluations with bound parameter sets.

## Recipe 9: Object Gaussian Mixture

### API Signature

- `GaussianMixture{weights, means, stddevs}`
- `GaussianMixture::sample/sample_like/pdf/logpdf/cdf/logcdf/cgf/cumulant`

### Notes

Object form is useful when the same mixture definition is reused across training/inference loops.

### Inputs

- component vectors

### Output

- repeated mixture calls without restating parameters

### Options

- sampling methods include `use_sobol` toggle

### Examples

```cpp
const Series w = make_series(std::vector<double>{0.7, 0.3}, {2});
const Series m = make_series(std::vector<double>{-1.0, 2.0}, {2});
const Series s = make_series(std::vector<double>{0.5, 1.25}, {2});
const pdt::GaussianMixture gm{w, m, s};

const Series draws = gm.sample({512}, {}, 23ULL, true);
const Series dens = gm.pdf(make_series(std::vector<double>{-2.0, 0.0, 2.0}, {3}));
```

### Expected output

- reusable mixture model object with concise repeated calls.

## Recipe 10: Closed-Form MLE (Core Families)

### API Signature

- `<Distribution>::MLE(x) -> <Distribution>Fit`
- `<Distribution>Fit::{loglik, score, fisher_inf, vcov, std_err, confint, confint_view, ...}`

### Notes

Use this pattern for fast parameter estimation + inference diagnostics.

### Inputs

- observed sample series

### Output

- fit object with parameter estimates and statistical diagnostics

### Options

- `confint(level)` confidence level

### Examples

```cpp
const Series xn = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4});
const auto nfit = pdt::Normal{0.0, 1.0}.MLE(xn);

const double mu = nfit.mean;
const double sigma = nfit.stddev;
const Series ll = nfit.loglik(xn);
const Series score = nfit.score(xn);
const Series se = nfit.std_err(xn);
const Series ci95 = nfit.confint(xn, 0.95);
const auto ci95_view = nfit.confint_view(xn, 0.95); // MSharp::RangeView::Double
// read ci95_view immediately (lightweight host view)
```

### Expected output

- estimated parameters and diagnostics available immediately from fit object.

## Recipe 11: Approximate/Iterative MLE Families

### API Signature

- `Gamma::MLE`, `Beta::MLE`, `NegativeBinomial::MLE`, `Nig::MLE`, `Gh::MLE`, `GaussianMixture::MLE`

### Notes

Use these when closed-form MLE is unavailable and iterative estimation is required.

### Inputs

- sample series per modeled family

### Output

- fitted parameter objects + diagnostics

### Options

- evaluate `loglik`, `score`, and `confint` for fit quality checks

### Examples

```cpp
const Series xheavy = make_series(std::vector<double>{-2.4, -1.1, -0.3, 0.2, 0.9, 1.8, 3.1}, {7});
const auto nig_fit = pdt::Nig{1.5, 0.1, 0.0, 1.0}.MLE(xheavy);
const auto gh_fit = pdt::Gh{1.0, 2.0, 0.1, 0.0, 1.0}.MLE(xheavy);
```

### Expected output

- fitted heavy-tail parameters with objective/information diagnostics.

## Recipe 12: Kind-Catalog Sweep With `dst::Kind`

### API Signature

- `sample(kind, ...)`, `pdf(kind, ...)`, `cdf(kind, ...)`, `supports(kind)`

### Notes

Useful for catalog tests, benchmarks, and parity checks.

### Inputs

- list of `dst::Kind` + valid per-kind parameter vectors

### Output

- programmatic cross-family evaluation

### Options

- vary seed by kind index for reproducible catalog demos

### Examples

```cpp
std::vector<dst::Kind> kinds = {dst::Normal, dst::Gamma, dst::StudentT, dst::Nig, dst::Zeta};
for (std::size_t i = 0; i < kinds.size(); ++i) {
  if (!pdt::supports(kinds[i])) continue;
  // For each kind, pass a valid parameter vector for that family.
}
```

### Expected output

- repeatable catalog-style family checks across many distributions.

## Functional Kind Atlas

### API Signature

- `dst::Kind`

### Notes

Functional API supports:

- `Alpha`, `Anglit`, `Arcsin`, `Bessel`, `Normal`, `Lognormal`, `Uniform`, `Exponential`, `Bernoulli`, `Beta`, `Binomial`, `Bradford`, `Burr`, `Cauchy`, `Chi`, `ChiSquare`, `ChiSquareNoncentral`, `Cosine`, `Erlang`, `ExtremeValue`, `F`, `FNoncentral`, `Laplace`, `Logistic`, `Geometric`, `Gamma`, `Gompertz`, `HalfCosine`, `HyperbolicSecant`, `Hypergeometric`, `InverseGaussian`, `Maxwell`, `Nakagami`, `NegativeBinomial`, `NormalTrunc`, `Planck`, `Poisson`, `Quadratic`, `Rademacher`, `StudentT`, `Triangular`, `Pareto`, `Lomax`, `Rayleigh`, `Rice`, `Semicircle`, `Skellam`, `TukeyLambda`, `UShape`, `VonMises`, `Wald`, `Weibull`, `Yule`, `Zeta`, `Gig`, `Gh`, `Nig`

### Inputs

- valid parameters for the selected kind

### Output

- support, sampling, and evaluation through the same functional entrypoints

### Options

- use object wrappers when one family is repeatedly reused

### Examples

```cpp
const bool ok = pdt::supports(dst::Gig);
```

### Expected output

- `ok` is `true` for implemented kind constants.
