# Distribution Catalog

Each section below documents one supported family with the same reference layout used throughout the docs: `Signature`, `Notes`, and `Examples`.

## `normal`

### Signature

```cpp
pluss::distr::sample("normal", {mu, sigma}, sizes, cfg, seed);
pluss::distr::sample("normal", {}, sizes, cfg, seed);
pluss::distr::sample_like("normal", {mu, sigma}, like, seed);
pluss::distr::sample_like("normal", {}, like, seed);
pluss::distr::pdf("normal", x, {mu, sigma});
pluss::distr::pdf("normal", x, {});
pluss::distr::logpdf("normal", x, {mu, sigma});
pluss::distr::logpdf("normal", x, {});
pluss::distr::cdf("normal", x, {mu, sigma});
pluss::distr::cdf("normal", x, {});
```

### Notes

Gaussian family. Use the empty parameter list for the standard normal, or pass mean and standard deviation for a shifted/scaled normal.

Aliases: `norm, gaussian, gauss`.

Parameters: `{mu, sigma} or {} for standard normal`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("normal", {0.0, 1.0}, {6}, cfg, 11ULL);
auto like_draws = pluss::distr::sample_like("normal", {0.0, 1.0}, like, 11ULL);
auto dens = pluss::distr::pdf("normal", x, {0.0, 1.0});
auto logprob = pluss::distr::logpdf("normal", x, {0.0, 1.0});
auto dist = pluss::distr::cdf("normal", x, {0.0, 1.0});
```

## `lognormal`

### Signature

```cpp
pluss::distr::sample("lognormal", {mu, sigma}, sizes, cfg, seed);
pluss::distr::sample_like("lognormal", {mu, sigma}, like, seed);
pluss::distr::pdf("lognormal", x, {mu, sigma});
pluss::distr::logpdf("lognormal", x, {mu, sigma});
pluss::distr::cdf("lognormal", x, {mu, sigma});
```

### Notes

Samples and densities are computed in log-space, then mapped back to the positive real line.

Aliases: `lognorm, cobbdouglas, antilognormal`.

Parameters: `{mu, sigma}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("lognorm", {0.0, 0.5}, {6}, cfg, 12ULL);
auto like_draws = pluss::distr::sample_like("lognorm", {0.0, 0.5}, like, 12ULL);
auto dens = pluss::distr::pdf("lognorm", x, {0.0, 0.5});
auto logprob = pluss::distr::logpdf("lognorm", x, {0.0, 0.5});
auto dist = pluss::distr::cdf("lognorm", x, {0.0, 0.5});
```

## `uniform`

### Signature

```cpp
pluss::distr::sample("uniform", {a, b}, sizes, cfg, seed);
pluss::distr::sample_like("uniform", {a, b}, like, seed);
pluss::distr::pdf("uniform", x, {a, b});
pluss::distr::logpdf("uniform", x, {a, b});
pluss::distr::cdf("uniform", x, {a, b});
```

### Notes

Draws uniformly on the closed interval defined by the lower and upper bounds.

Aliases: `unif`.

Parameters: `{a, b}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.0, 0.25, 0.5, 0.75, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("uniform", {0.0, 1.0}, {6}, cfg, 13ULL);
auto like_draws = pluss::distr::sample_like("uniform", {0.0, 1.0}, like, 13ULL);
auto dens = pluss::distr::pdf("uniform", x, {0.0, 1.0});
auto logprob = pluss::distr::logpdf("uniform", x, {0.0, 1.0});
auto dist = pluss::distr::cdf("uniform", x, {0.0, 1.0});
```

## `exponential`

### Signature

```cpp
pluss::distr::sample("exponential", {lambda}, sizes, cfg, seed);
pluss::distr::sample_like("exponential", {lambda}, like, seed);
pluss::distr::pdf("exponential", x, {lambda});
pluss::distr::logpdf("exponential", x, {lambda});
pluss::distr::cdf("exponential", x, {lambda});
```

### Notes

Uses the rate parameter lambda. Larger lambda concentrates more mass near zero.

Aliases: `exp`.

Parameters: `{lambda}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.0, 0.5, 1.0, 1.5}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("exp", {1.5}, {6}, cfg, 14ULL);
auto like_draws = pluss::distr::sample_like("exp", {1.5}, like, 14ULL);
auto dens = pluss::distr::pdf("exp", x, {1.5});
auto logprob = pluss::distr::logpdf("exp", x, {1.5});
auto dist = pluss::distr::cdf("exp", x, {1.5});
```

## `bernoulli`

### Signature

```cpp
pluss::distr::sample("bernoulli", {p}, sizes, cfg, seed);
pluss::distr::sample_like("bernoulli", {p}, like, seed);
pluss::distr::pdf("bernoulli", x, {p});
pluss::distr::logpdf("bernoulli", x, {p});
pluss::distr::cdf("bernoulli", x, {p});
```

### Notes

Discrete 0/1 family. `sample(...)` naturally produces a boolean Series unless you override dtype.

Aliases: `bern`.

Parameters: `{p}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("bern", {0.35}, {8}, cfg, 15ULL);
auto like_draws = pluss::distr::sample_like("bern", {0.35}, like, 15ULL);
auto pmf = pluss::distr::pdf("bern", x, {0.35});
auto logprob = pluss::distr::logpdf("bern", x, {0.35});
auto dist = pluss::distr::cdf("bern", x, {0.35});
```

## `bessel`

### Signature

```cpp
pluss::distr::sample("bessel", {nu, a}, sizes, cfg, seed);
pluss::distr::sample_like("bessel", {nu, a}, like, seed);
pluss::distr::pdf("bessel", x, {nu, a});
pluss::distr::logpdf("bessel", x, {nu, a});
pluss::distr::cdf("bessel", x, {nu, a});
```

### Notes

Discrete non-negative count family normalized by the modified Bessel function `I_nu(a)`. `nu` must stay above `-1` and `a` must be positive. This is the right model when the count law itself is defined through a Bessel-function normalization rather than through a Poisson or negative-binomial construction.

Aliases: none.

Parameters: `{nu, a}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("bessel", {0.0, 2.5}, {6}, cfg, 15ULL);
auto like_draws = pluss::distr::sample_like("bessel", {0.0, 2.5}, like, 15ULL);
auto pmf = pluss::distr::pdf("bessel", x, {0.0, 2.5});
auto logprob = pluss::distr::logpdf("bessel", x, {0.0, 2.5});
auto dist = pluss::distr::cdf("bessel", x, {0.0, 2.5});
```

## `beta`

### Signature

```cpp
pluss::distr::sample("beta", {a, b}, sizes, cfg, seed);
pluss::distr::sample("beta", {m, n, a, b}, sizes, cfg, seed);
pluss::distr::sample_like("beta", {a, b}, like, seed);
pluss::distr::sample_like("beta", {m, n, a, b}, like, seed);
pluss::distr::pdf("beta", x, {a, b});
pluss::distr::pdf("beta", x, {m, n, a, b});
pluss::distr::logpdf("beta", x, {a, b});
pluss::distr::logpdf("beta", x, {m, n, a, b});
pluss::distr::cdf("beta", x, {a, b});
pluss::distr::cdf("beta", x, {m, n, a, b});
```

### Notes

Use the two-parameter form for the standard beta on [0,1]. The four-parameter form exposes the shifted/scaled legacy convention.

Aliases: `powerfunction, powerfunc`.

Parameters: `{a, b} or {m, n, a, b}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.25, 0.5, 0.75}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("beta", {2.0, 5.0}, {6}, cfg, 16ULL);
auto like_draws = pluss::distr::sample_like("beta", {2.0, 5.0}, like, 16ULL);
auto dens = pluss::distr::pdf("beta", x, {2.0, 5.0});
auto logprob = pluss::distr::logpdf("beta", x, {2.0, 5.0});
auto dist = pluss::distr::cdf("beta", x, {2.0, 5.0});
```

## `binomial`

### Signature

```cpp
pluss::distr::sample("binomial", {n, p}, sizes, cfg, seed);
pluss::distr::sample_like("binomial", {n, p}, like, seed);
pluss::distr::pdf("binomial", x, {n, p});
pluss::distr::logpdf("binomial", x, {n, p});
pluss::distr::cdf("binomial", x, {n, p});
```

### Notes

Counts successes across `n` Bernoulli trials.

Aliases: `binom`.

Parameters: `{n, p}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("binom", {10.0, 0.3}, {6}, cfg, 17ULL);
auto like_draws = pluss::distr::sample_like("binom", {10.0, 0.3}, like, 17ULL);
auto pmf = pluss::distr::pdf("binom", x, {10.0, 0.3});
auto logprob = pluss::distr::logpdf("binom", x, {10.0, 0.3});
auto dist = pluss::distr::cdf("binom", x, {10.0, 0.3});
```

## `bradford`

### Signature

```cpp
pluss::distr::sample("bradford", {c}, sizes, cfg, seed);
pluss::distr::sample_like("bradford", {c}, like, seed);
pluss::distr::pdf("bradford", x, {c});
pluss::distr::logpdf("bradford", x, {c});
pluss::distr::cdf("bradford", x, {c});
```

### Notes

Bounded skewed family controlled by the shape parameter `c`.

Aliases: none.

Parameters: `{c}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.1, 0.4, 0.8}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("bradford", {1.5}, {6}, cfg, 18ULL);
auto like_draws = pluss::distr::sample_like("bradford", {1.5}, like, 18ULL);
auto dens = pluss::distr::pdf("bradford", x, {1.5});
auto logprob = pluss::distr::logpdf("bradford", x, {1.5});
auto dist = pluss::distr::cdf("bradford", x, {1.5});
```

## `burr`

### Signature

```cpp
pluss::distr::sample("burr", {c, k}, sizes, cfg, seed);
pluss::distr::sample_like("burr", {c, k}, like, seed);
pluss::distr::pdf("burr", x, {c, k});
pluss::distr::logpdf("burr", x, {c, k});
pluss::distr::cdf("burr", x, {c, k});
```

### Notes

Heavy-tailed positive family. Both parameters control tail shape.

Aliases: `fisk`.

Parameters: `{c, k}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({1.0, 2.0, 3.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("burr", {2.0, 3.0}, {6}, cfg, 19ULL);
auto like_draws = pluss::distr::sample_like("burr", {2.0, 3.0}, like, 19ULL);
auto dens = pluss::distr::pdf("burr", x, {2.0, 3.0});
auto logprob = pluss::distr::logpdf("burr", x, {2.0, 3.0});
auto dist = pluss::distr::cdf("burr", x, {2.0, 3.0});
```

## `cauchy`

### Signature

```cpp
pluss::distr::sample("cauchy", {location, scale}, sizes, cfg, seed);
pluss::distr::sample_like("cauchy", {location, scale}, like, seed);
pluss::distr::pdf("cauchy", x, {location, scale});
pluss::distr::logpdf("cauchy", x, {location, scale});
pluss::distr::cdf("cauchy", x, {location, scale});
```

### Notes

Heavy-tailed location-scale family with undefined mean/variance in the classical sense.

Aliases: `lorentz, caushy`.

Parameters: `{location, scale}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("cauchy", {0.0, 1.0}, {6}, cfg, 20ULL);
auto like_draws = pluss::distr::sample_like("cauchy", {0.0, 1.0}, like, 20ULL);
auto dens = pluss::distr::pdf("cauchy", x, {0.0, 1.0});
auto logprob = pluss::distr::logpdf("cauchy", x, {0.0, 1.0});
auto dist = pluss::distr::cdf("cauchy", x, {0.0, 1.0});
```

## `chi`

### Signature

```cpp
pluss::distr::sample("chi", {nu}, sizes, cfg, seed);
pluss::distr::sample("chi", {a, b, nu}, sizes, cfg, seed);
pluss::distr::sample_like("chi", {nu}, like, seed);
pluss::distr::sample_like("chi", {a, b, nu}, like, seed);
pluss::distr::pdf("chi", x, {nu});
pluss::distr::pdf("chi", x, {a, b, nu});
pluss::distr::logpdf("chi", x, {nu});
pluss::distr::logpdf("chi", x, {a, b, nu});
pluss::distr::cdf("chi", x, {nu});
pluss::distr::cdf("chi", x, {a, b, nu});
```

### Notes

Chi family. The short form is the standard chi distribution; the longer legacy form exposes location/scale/shape.

Aliases: none.

Parameters: `{nu} or {a, b, nu}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("chi", {3.0}, {6}, cfg, 21ULL);
auto like_draws = pluss::distr::sample_like("chi", {3.0}, like, 21ULL);
auto dens = pluss::distr::pdf("chi", x, {3.0});
auto logprob = pluss::distr::logpdf("chi", x, {3.0});
auto dist = pluss::distr::cdf("chi", x, {3.0});
```

## `chisquare`

### Signature

```cpp
pluss::distr::sample("chisquare", {nu}, sizes, cfg, seed);
pluss::distr::sample_like("chisquare", {nu}, like, seed);
pluss::distr::pdf("chisquare", x, {nu});
pluss::distr::logpdf("chisquare", x, {nu});
pluss::distr::cdf("chisquare", x, {nu});
```

### Notes

Standard chi-square family parameterized by degrees of freedom.

Aliases: `chisq, chi2`.

Parameters: `{nu}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0, 4.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("chisq", {4.0}, {6}, cfg, 22ULL);
auto like_draws = pluss::distr::sample_like("chisq", {4.0}, like, 22ULL);
auto dens = pluss::distr::pdf("chisq", x, {4.0});
auto logprob = pluss::distr::logpdf("chisq", x, {4.0});
auto dist = pluss::distr::cdf("chisq", x, {4.0});
```

## `chisqnoncentral`

### Signature

```cpp
pluss::distr::sample("chisqnoncentral", {lambda, nu}, sizes, cfg, seed);
pluss::distr::sample_like("chisqnoncentral", {lambda, nu}, like, seed);
pluss::distr::pdf("chisqnoncentral", x, {lambda, nu});
pluss::distr::logpdf("chisqnoncentral", x, {lambda, nu});
pluss::distr::cdf("chisqnoncentral", x, {lambda, nu});
```

### Notes

Non-central chi-square family with non-centrality `lambda` and degrees of freedom `nu`.

Aliases: `chisqnc, chisqnoncentral, chisqnoncentr, chi2noncentral`.

Parameters: `{lambda, nu}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({1.0, 2.0, 3.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("chisqnc", {1.5, 4.0}, {6}, cfg, 23ULL);
auto like_draws = pluss::distr::sample_like("chisqnc", {1.5, 4.0}, like, 23ULL);
auto dens = pluss::distr::pdf("chisqnc", x, {1.5, 4.0});
auto logprob = pluss::distr::logpdf("chisqnc", x, {1.5, 4.0});
auto dist = pluss::distr::cdf("chisqnc", x, {1.5, 4.0});
```

## `cosine`

### Signature

```cpp
pluss::distr::sample("cosine", {}, sizes, cfg, seed);
pluss::distr::sample_like("cosine", {}, like, seed);
pluss::distr::pdf("cosine", x, {});
pluss::distr::logpdf("cosine", x, {});
pluss::distr::cdf("cosine", x, {});
```

### Notes

Compact-support cosine density centered at zero.

Aliases: none.

Parameters: `{}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("cosine", {}, {6}, cfg, 24ULL);
auto like_draws = pluss::distr::sample_like("cosine", like, 24ULL);
auto dens = pluss::distr::pdf("cosine", x, {});
auto logprob = pluss::distr::logpdf("cosine", x, {});
auto dist = pluss::distr::cdf("cosine", x, {});
```

## `erlang`

### Signature

```cpp
pluss::distr::sample("erlang", {scale, k}, sizes, cfg, seed);
pluss::distr::sample_like("erlang", {scale, k}, like, seed);
pluss::distr::pdf("erlang", x, {scale, k});
pluss::distr::logpdf("erlang", x, {scale, k});
pluss::distr::cdf("erlang", x, {scale, k});
```

### Notes

Integer-shape gamma special case. Useful when you want the classic Erlang parameterization.

Aliases: none.

Parameters: `{scale, k}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("erlang", {1.25, 3.0}, {6}, cfg, 25ULL);
auto like_draws = pluss::distr::sample_like("erlang", {1.25, 3.0}, like, 25ULL);
auto dens = pluss::distr::pdf("erlang", x, {1.25, 3.0});
auto logprob = pluss::distr::logpdf("erlang", x, {1.25, 3.0});
auto dist = pluss::distr::cdf("erlang", x, {1.25, 3.0});
```

## `extremevalue`

### Signature

```cpp
pluss::distr::sample("extremevalue", {location, scale}, sizes, cfg, seed);
pluss::distr::sample_like("extremevalue", {location, scale}, like, seed);
pluss::distr::pdf("extremevalue", x, {location, scale});
pluss::distr::logpdf("extremevalue", x, {location, scale});
pluss::distr::cdf("extremevalue", x, {location, scale});
```

### Notes

Gumbel-type extreme-value distribution.

Aliases: `extrval, extrvalue, gumbel`.

Parameters: `{location, scale}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("extrval", {0.0, 1.0}, {6}, cfg, 26ULL);
auto like_draws = pluss::distr::sample_like("extrval", {0.0, 1.0}, like, 26ULL);
auto dens = pluss::distr::pdf("extrval", x, {0.0, 1.0});
auto logprob = pluss::distr::logpdf("extrval", x, {0.0, 1.0});
auto dist = pluss::distr::cdf("extrval", x, {0.0, 1.0});
```

## `f`

### Signature

```cpp
pluss::distr::sample("f", {df1, df2}, sizes, cfg, seed);
pluss::distr::sample_like("f", {df1, df2}, like, seed);
pluss::distr::pdf("f", x, {df1, df2});
pluss::distr::logpdf("f", x, {df1, df2});
pluss::distr::cdf("f", x, {df1, df2});
```

### Notes

Classical F family with numerator and denominator degrees of freedom.

Aliases: `fdistribution, fdistrib, fdistr, fdist, fdis`.

Parameters: `{df1, df2}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("f", {4.0, 6.0}, {6}, cfg, 27ULL);
auto like_draws = pluss::distr::sample_like("f", {4.0, 6.0}, like, 27ULL);
auto dens = pluss::distr::pdf("f", x, {4.0, 6.0});
auto logprob = pluss::distr::logpdf("f", x, {4.0, 6.0});
auto dist = pluss::distr::cdf("f", x, {4.0, 6.0});
```

## `fnoncentral`

### Signature

```cpp
pluss::distr::sample("fnoncentral", {lambda, df1, df2}, sizes, cfg, seed);
pluss::distr::sample_like("fnoncentral", {lambda, df1, df2}, like, seed);
pluss::distr::pdf("fnoncentral", x, {lambda, df1, df2});
pluss::distr::logpdf("fnoncentral", x, {lambda, df1, df2});
pluss::distr::cdf("fnoncentral", x, {lambda, df1, df2});
```

### Notes

Non-central F family with non-centrality and two degree-of-freedom parameters.

Aliases: `fnc, fnoncentral, fnoncentr`.

Parameters: `{lambda, df1, df2}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("fnc", {1.5, 4.0, 6.0}, {6}, cfg, 28ULL);
auto like_draws = pluss::distr::sample_like("fnc", {1.5, 4.0, 6.0}, like, 28ULL);
auto dens = pluss::distr::pdf("fnc", x, {1.5, 4.0, 6.0});
auto logprob = pluss::distr::logpdf("fnc", x, {1.5, 4.0, 6.0});
auto dist = pluss::distr::cdf("fnc", x, {1.5, 4.0, 6.0});
```

## `laplace`

### Signature

```cpp
pluss::distr::sample("laplace", {location, scale}, sizes, cfg, seed);
pluss::distr::sample_like("laplace", {location, scale}, like, seed);
pluss::distr::pdf("laplace", x, {location, scale});
pluss::distr::logpdf("laplace", x, {location, scale});
pluss::distr::cdf("laplace", x, {location, scale});
```

### Notes

Two-sided exponential family centered at `location`.

Aliases: `doubleexponential, doubleexp, bilateralexponential, bilateralexp`.

Parameters: `{location, scale}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("laplace", {0.0, 1.0}, {6}, cfg, 29ULL);
auto like_draws = pluss::distr::sample_like("laplace", {0.0, 1.0}, like, 29ULL);
auto dens = pluss::distr::pdf("laplace", x, {0.0, 1.0});
auto logprob = pluss::distr::logpdf("laplace", x, {0.0, 1.0});
auto dist = pluss::distr::cdf("laplace", x, {0.0, 1.0});
```

## `logistic`

### Signature

```cpp
pluss::distr::sample("logistic", {location, scale}, sizes, cfg, seed);
pluss::distr::sample_like("logistic", {location, scale}, like, seed);
pluss::distr::pdf("logistic", x, {location, scale});
pluss::distr::logpdf("logistic", x, {location, scale});
pluss::distr::cdf("logistic", x, {location, scale});
```

### Notes

Logistic location-scale family.

Aliases: none.

Parameters: `{location, scale}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("logistic", {0.0, 1.0}, {6}, cfg, 30ULL);
auto like_draws = pluss::distr::sample_like("logistic", {0.0, 1.0}, like, 30ULL);
auto dens = pluss::distr::pdf("logistic", x, {0.0, 1.0});
auto logprob = pluss::distr::logpdf("logistic", x, {0.0, 1.0});
auto dist = pluss::distr::cdf("logistic", x, {0.0, 1.0});
```

## `geometric`

### Signature

```cpp
pluss::distr::sample("geometric", {p}, sizes, cfg, seed);
pluss::distr::sample_like("geometric", {p}, like, seed);
pluss::distr::pdf("geometric", x, {p});
pluss::distr::logpdf("geometric", x, {p});
pluss::distr::cdf("geometric", x, {p});
```

### Notes

Counts trials until first success under the runtime convention implemented in `distr.cpp`.

Aliases: `geom, furry`.

Parameters: `{p}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({1.0, 2.0, 3.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("geom", {0.35}, {6}, cfg, 31ULL);
auto like_draws = pluss::distr::sample_like("geom", {0.35}, like, 31ULL);
auto dens = pluss::distr::pdf("geom", x, {0.35});
auto logprob = pluss::distr::logpdf("geom", x, {0.35});
auto dist = pluss::distr::cdf("geom", x, {0.35});
```

## `gamma`

### Signature

```cpp
pluss::distr::sample("gamma", {a}, sizes, cfg, seed);
pluss::distr::sample("gamma", {m, b, a}, sizes, cfg, seed);
pluss::distr::sample_like("gamma", {a}, like, seed);
pluss::distr::sample_like("gamma", {m, b, a}, like, seed);
pluss::distr::pdf("gamma", x, {a});
pluss::distr::pdf("gamma", x, {m, b, a});
pluss::distr::logpdf("gamma", x, {a});
pluss::distr::logpdf("gamma", x, {m, b, a});
pluss::distr::cdf("gamma", x, {a});
pluss::distr::cdf("gamma", x, {m, b, a});
```

### Notes

Gamma family. Use the one-parameter shortcut for the standard shape-only form, or the legacy location/scale/shape form when needed.

Aliases: none.

Parameters: `{a} or {m, b, a}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("gamma", {2.5}, {6}, cfg, 32ULL);
auto like_draws = pluss::distr::sample_like("gamma", {2.5}, like, 32ULL);
auto dens = pluss::distr::pdf("gamma", x, {2.5});
auto logprob = pluss::distr::logpdf("gamma", x, {2.5});
auto dist = pluss::distr::cdf("gamma", x, {2.5});
```

## `gompertz`

### Signature

```cpp
pluss::distr::sample("gompertz", {eta, b}, sizes, cfg, seed);
pluss::distr::sample_like("gompertz", {eta, b}, like, seed);
pluss::distr::pdf("gompertz", x, {eta, b});
pluss::distr::logpdf("gompertz", x, {eta, b});
pluss::distr::cdf("gompertz", x, {eta, b});
```

### Notes

Positive-support Gompertz family.

Aliases: none.

Parameters: `{eta, b}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.25, 0.5, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("gompertz", {1.0, 2.0}, {6}, cfg, 33ULL);
auto like_draws = pluss::distr::sample_like("gompertz", {1.0, 2.0}, like, 33ULL);
auto dens = pluss::distr::pdf("gompertz", x, {1.0, 2.0});
auto logprob = pluss::distr::logpdf("gompertz", x, {1.0, 2.0});
auto dist = pluss::distr::cdf("gompertz", x, {1.0, 2.0});
```

## `halfcosine`

### Signature

```cpp
pluss::distr::sample("halfcosine", {}, sizes, cfg, seed);
pluss::distr::sample_like("halfcosine", {}, like, seed);
pluss::distr::pdf("halfcosine", x, {});
pluss::distr::logpdf("halfcosine", x, {});
pluss::distr::cdf("halfcosine", x, {});
```

### Notes

Half-cosine density on bounded support.

Aliases: `hcosine, hcos`.

Parameters: `{}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.0, 0.25, 0.5}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("hcos", {}, {6}, cfg, 34ULL);
auto like_draws = pluss::distr::sample_like("hcos", like, 34ULL);
auto dens = pluss::distr::pdf("hcos", x, {});
auto logprob = pluss::distr::logpdf("hcos", x, {});
auto dist = pluss::distr::cdf("hcos", x, {});
```

## `hyperbolicsecant`

### Signature

```cpp
pluss::distr::sample("hyperbolicsecant", {}, sizes, cfg, seed);
pluss::distr::sample_like("hyperbolicsecant", {}, like, seed);
pluss::distr::pdf("hyperbolicsecant", x, {});
pluss::distr::logpdf("hyperbolicsecant", x, {});
pluss::distr::cdf("hyperbolicsecant", x, {});
```

### Notes

Symmetric hyperbolic-secant family.

Aliases: `hsecant, hsec`.

Parameters: `{}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("hsec", {}, {6}, cfg, 35ULL);
auto like_draws = pluss::distr::sample_like("hsec", like, 35ULL);
auto dens = pluss::distr::pdf("hsec", x, {});
auto logprob = pluss::distr::logpdf("hsec", x, {});
auto dist = pluss::distr::cdf("hsec", x, {});
```

## `hypergeometric`

### Signature

```cpp
pluss::distr::sample("hypergeometric", {N, M, n}, sizes, cfg, seed);
pluss::distr::sample_like("hypergeometric", {N, M, n}, like, seed);
pluss::distr::pdf("hypergeometric", x, {N, M, n});
pluss::distr::logpdf("hypergeometric", x, {N, M, n});
pluss::distr::cdf("hypergeometric", x, {N, M, n});
```

### Notes

Draws counts without replacement from a finite population.

Aliases: `hypergeom`.

Parameters: `{N, M, n}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.0, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("hypergeom", {20.0, 8.0, 5.0}, {6}, cfg, 36ULL);
auto like_draws = pluss::distr::sample_like("hypergeom", {20.0, 8.0, 5.0}, like, 36ULL);
auto pmf = pluss::distr::pdf("hypergeom", x, {20.0, 8.0, 5.0});
auto logprob = pluss::distr::logpdf("hypergeom", x, {20.0, 8.0, 5.0});
auto dist = pluss::distr::cdf("hypergeom", x, {20.0, 8.0, 5.0});
```

## `inversegaussian`

### Signature

```cpp
pluss::distr::sample("inversegaussian", {mu, lambda}, sizes, cfg, seed);
pluss::distr::sample_like("inversegaussian", {mu, lambda}, like, seed);
pluss::distr::pdf("inversegaussian", x, {mu, lambda});
pluss::distr::logpdf("inversegaussian", x, {mu, lambda});
pluss::distr::cdf("inversegaussian", x, {mu, lambda});
```

### Notes

Positive-support inverse Gaussian / Wald-type family with separate mean and shape.

Aliases: `ig, inversegauss, invgauss`.

Parameters: `{mu, lambda}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("ig", {1.5, 2.0}, {6}, cfg, 37ULL);
auto like_draws = pluss::distr::sample_like("ig", {1.5, 2.0}, like, 37ULL);
auto dens = pluss::distr::pdf("ig", x, {1.5, 2.0});
auto logprob = pluss::distr::logpdf("ig", x, {1.5, 2.0});
auto dist = pluss::distr::cdf("ig", x, {1.5, 2.0});
```

## `maxwell`

### Signature

```cpp
pluss::distr::sample("maxwell", {scale}, sizes, cfg, seed);
pluss::distr::sample_like("maxwell", {scale}, like, seed);
pluss::distr::pdf("maxwell", x, {scale});
pluss::distr::logpdf("maxwell", x, {scale});
pluss::distr::cdf("maxwell", x, {scale});
```

### Notes

Maxwell speed distribution.

Aliases: none.

Parameters: `{scale}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("maxwell", {1.0}, {6}, cfg, 38ULL);
auto like_draws = pluss::distr::sample_like("maxwell", {1.0}, like, 38ULL);
auto dens = pluss::distr::pdf("maxwell", x, {1.0});
auto logprob = pluss::distr::logpdf("maxwell", x, {1.0});
auto dist = pluss::distr::cdf("maxwell", x, {1.0});
```

## `nakagami`

### Signature

```cpp
pluss::distr::sample("nakagami", {m, omega}, sizes, cfg, seed);
pluss::distr::sample_like("nakagami", {m, omega}, like, seed);
pluss::distr::pdf("nakagami", x, {m, omega});
pluss::distr::logpdf("nakagami", x, {m, omega});
pluss::distr::cdf("nakagami", x, {m, omega});
```

### Notes

Positive-support Nakagami family.

Aliases: `nakagamim, nakagami-m`.

Parameters: `{m, omega}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("nakagami", {1.5, 2.0}, {6}, cfg, 39ULL);
auto like_draws = pluss::distr::sample_like("nakagami", {1.5, 2.0}, like, 39ULL);
auto dens = pluss::distr::pdf("nakagami", x, {1.5, 2.0});
auto logprob = pluss::distr::logpdf("nakagami", x, {1.5, 2.0});
auto dist = pluss::distr::cdf("nakagami", x, {1.5, 2.0});
```

## `negativebinomial`

### Signature

```cpp
pluss::distr::sample("negativebinomial", {r, p}, sizes, cfg, seed);
pluss::distr::sample_like("negativebinomial", {r, p}, like, seed);
pluss::distr::pdf("negativebinomial", x, {r, p});
pluss::distr::logpdf("negativebinomial", x, {r, p});
pluss::distr::cdf("negativebinomial", x, {r, p});
```

### Notes

Counts failures before a target number of successes under the implemented convention.

Aliases: `negbinomial, negbinom, pascal`.

Parameters: `{r, p}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("pascal", {3.0, 0.45}, {6}, cfg, 40ULL);
auto like_draws = pluss::distr::sample_like("pascal", {3.0, 0.45}, like, 40ULL);
auto pmf = pluss::distr::pdf("pascal", x, {3.0, 0.45});
auto logprob = pluss::distr::logpdf("pascal", x, {3.0, 0.45});
auto dist = pluss::distr::cdf("pascal", x, {3.0, 0.45});
```

## `normaltrunc`

### Signature

```cpp
pluss::distr::sample("normaltrunc", {a, b, mu, sigma}, sizes, cfg, seed);
pluss::distr::sample_like("normaltrunc", {a, b, mu, sigma}, like, seed);
pluss::distr::pdf("normaltrunc", x, {a, b, mu, sigma});
pluss::distr::logpdf("normaltrunc", x, {a, b, mu, sigma});
pluss::distr::cdf("normaltrunc", x, {a, b, mu, sigma});
```

### Notes

Truncated normal clipped to `[a, b]` before normalization.

Aliases: `normaltruncated, gausstrunc`.

Parameters: `{a, b, mu, sigma}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("normaltrunc", {-1.0, 1.0, 0.0, 1.0}, {6}, cfg, 41ULL);
auto like_draws = pluss::distr::sample_like("normaltrunc", {-1.0, 1.0, 0.0, 1.0}, like, 41ULL);
auto dens = pluss::distr::pdf("normaltrunc", x, {-1.0, 1.0, 0.0, 1.0});
auto logprob = pluss::distr::logpdf("normaltrunc", x, {-1.0, 1.0, 0.0, 1.0});
auto dist = pluss::distr::cdf("normaltrunc", x, {-1.0, 1.0, 0.0, 1.0});
```

## `planck`

### Signature

```cpp
pluss::distr::sample("planck", {lambda, scale}, sizes, cfg, seed);
pluss::distr::sample_like("planck", {lambda, scale}, like, seed);
pluss::distr::pdf("planck", x, {lambda, scale});
pluss::distr::logpdf("planck", x, {lambda, scale});
pluss::distr::cdf("planck", x, {lambda, scale});
```

### Notes

Discrete positive-count family with exponentially damped tails. Use it when you want a count distribution that stays strictly positive and decays faster than a pure power law. `lambda` sets the near-origin shape and `scale` controls the tail decay rate.

Aliases: none.

Parameters: `{lambda, scale}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("planck", {2.0, 1.5}, {6}, cfg, 42ULL);
auto like_draws = pluss::distr::sample_like("planck", {2.0, 1.5}, like, 42ULL);
auto pmf = pluss::distr::pdf("planck", x, {2.0, 1.5});
auto logprob = pluss::distr::logpdf("planck", x, {2.0, 1.5});
auto dist = pluss::distr::cdf("planck", x, {2.0, 1.5});
```

## `poisson`

### Signature

```cpp
pluss::distr::sample("poisson", {lambda}, sizes, cfg, seed);
pluss::distr::sample_like("poisson", {lambda}, like, seed);
pluss::distr::pdf("poisson", x, {lambda});
pluss::distr::logpdf("poisson", x, {lambda});
pluss::distr::cdf("poisson", x, {lambda});
```

### Notes

Counts independent events occurring at rate `lambda`.

Aliases: `po`.

Parameters: `{lambda}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("poisson", {2.0}, {6}, cfg, 43ULL);
auto like_draws = pluss::distr::sample_like("poisson", {2.0}, like, 43ULL);
auto pmf = pluss::distr::pdf("poisson", x, {2.0});
auto logprob = pluss::distr::logpdf("poisson", x, {2.0});
auto dist = pluss::distr::cdf("poisson", x, {2.0});
```

## `quadratic`

### Signature

```cpp
pluss::distr::sample("quadratic", {}, sizes, cfg, seed);
pluss::distr::sample_like("quadratic", {}, like, seed);
pluss::distr::pdf("quadratic", x, {});
pluss::distr::logpdf("quadratic", x, {});
pluss::distr::cdf("quadratic", x, {});
```

### Notes

Bounded quadratic family.

Aliases: `quad, quadr`.

Parameters: `{}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-0.5, 0.0, 0.5}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("quadr", {}, {6}, cfg, 44ULL);
auto like_draws = pluss::distr::sample_like("quadr", like, 44ULL);
auto dens = pluss::distr::pdf("quadr", x, {});
auto logprob = pluss::distr::logpdf("quadr", x, {});
auto dist = pluss::distr::cdf("quadr", x, {});
```

## `rademacher`

### Signature

```cpp
pluss::distr::sample("rademacher", {}, sizes, cfg, seed);
pluss::distr::sample_like("rademacher", {}, like, seed);
pluss::distr::pdf("rademacher", x, {});
pluss::distr::logpdf("rademacher", x, {});
pluss::distr::cdf("rademacher", x, {});
```

### Notes

Produces symmetric `-1/+1` sign draws.

Aliases: none.

Parameters: `{}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("rademacher", {}, {6}, cfg, 45ULL);
auto like_draws = pluss::distr::sample_like("rademacher", like, 45ULL);
auto pmf = pluss::distr::pdf("rademacher", x, {});
auto logprob = pluss::distr::logpdf("rademacher", x, {});
auto dist = pluss::distr::cdf("rademacher", x, {});
```

## `studentst`

### Signature

```cpp
pluss::distr::sample("studentst", {nu}, sizes, cfg, seed);
pluss::distr::sample("studentst", {nu, chi, eta}, sizes, cfg, seed);
pluss::distr::sample_like("studentst", {nu}, like, seed);
pluss::distr::sample_like("studentst", {nu, chi, eta}, like, seed);
pluss::distr::pdf("studentst", x, {nu});
pluss::distr::pdf("studentst", x, {nu, chi, eta});
pluss::distr::logpdf("studentst", x, {nu});
pluss::distr::logpdf("studentst", x, {nu, chi, eta});
pluss::distr::cdf("studentst", x, {nu});
pluss::distr::cdf("studentst", x, {nu, chi, eta});
```

### Notes

Student-t family with optional location/scale extension.

Aliases: `t`.

Parameters: `{nu} or {nu, chi, eta}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("t", {5.0}, {6}, cfg, 46ULL);
auto like_draws = pluss::distr::sample_like("t", {5.0}, like, 46ULL);
auto dens = pluss::distr::pdf("t", x, {5.0});
auto logprob = pluss::distr::logpdf("t", x, {5.0});
auto dist = pluss::distr::cdf("t", x, {5.0});
```

## `triangular`

### Signature

```cpp
pluss::distr::sample("triangular", {a, c, b}, sizes, cfg, seed);
pluss::distr::sample_like("triangular", {a, c, b}, like, seed);
pluss::distr::pdf("triangular", x, {a, c, b});
pluss::distr::logpdf("triangular", x, {a, c, b});
pluss::distr::cdf("triangular", x, {a, c, b});
```

### Notes

Triangular family with lower bound `a`, mode `c`, and upper bound `b`.

Aliases: `tri`.

Parameters: `{a, c, b}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.0, 0.5, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("tri", {0.0, 0.5, 1.0}, {6}, cfg, 47ULL);
auto like_draws = pluss::distr::sample_like("tri", {0.0, 0.5, 1.0}, like, 47ULL);
auto dens = pluss::distr::pdf("tri", x, {0.0, 0.5, 1.0});
auto logprob = pluss::distr::logpdf("tri", x, {0.0, 0.5, 1.0});
auto dist = pluss::distr::cdf("tri", x, {0.0, 0.5, 1.0});
```

## `pareto`

### Signature

```cpp
pluss::distr::sample("pareto", {xm, alpha}, sizes, cfg, seed);
pluss::distr::sample_like("pareto", {xm, alpha}, like, seed);
pluss::distr::pdf("pareto", x, {xm, alpha});
pluss::distr::logpdf("pareto", x, {xm, alpha});
pluss::distr::cdf("pareto", x, {xm, alpha});
```

### Notes

Classical Pareto tail family on values at or above the scale threshold.

Aliases: none.

Parameters: `{xm, alpha}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({1.0, 2.0, 4.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("pareto", {1.0, 3.0}, {6}, cfg, 48ULL);
auto like_draws = pluss::distr::sample_like("pareto", {1.0, 3.0}, like, 48ULL);
auto dens = pluss::distr::pdf("pareto", x, {1.0, 3.0});
auto logprob = pluss::distr::logpdf("pareto", x, {1.0, 3.0});
auto dist = pluss::distr::cdf("pareto", x, {1.0, 3.0});
```

## `lomax`

### Signature

```cpp
pluss::distr::sample("lomax", {scale, alpha}, sizes, cfg, seed);
pluss::distr::sample_like("lomax", {scale, alpha}, like, seed);
pluss::distr::pdf("lomax", x, {scale, alpha});
pluss::distr::logpdf("lomax", x, {scale, alpha});
pluss::distr::cdf("lomax", x, {scale, alpha});
```

### Notes

Shifted Pareto / Lomax family.

Aliases: `pareto2`.

Parameters: `{scale, alpha}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("lomax", {2.0, 3.0}, {6}, cfg, 49ULL);
auto like_draws = pluss::distr::sample_like("lomax", {2.0, 3.0}, like, 49ULL);
auto dens = pluss::distr::pdf("lomax", x, {2.0, 3.0});
auto logprob = pluss::distr::logpdf("lomax", x, {2.0, 3.0});
auto dist = pluss::distr::cdf("lomax", x, {2.0, 3.0});
```

## `rayleigh`

### Signature

```cpp
pluss::distr::sample("rayleigh", {scale}, sizes, cfg, seed);
pluss::distr::sample_like("rayleigh", {scale}, like, seed);
pluss::distr::pdf("rayleigh", x, {scale});
pluss::distr::logpdf("rayleigh", x, {scale});
pluss::distr::cdf("rayleigh", x, {scale});
```

### Notes

Radial magnitude family derived from isotropic Gaussian components.

Aliases: `rayl`.

Parameters: `{scale}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("rayl", {1.0}, {6}, cfg, 50ULL);
auto like_draws = pluss::distr::sample_like("rayl", {1.0}, like, 50ULL);
auto dens = pluss::distr::pdf("rayl", x, {1.0});
auto logprob = pluss::distr::logpdf("rayl", x, {1.0});
auto dist = pluss::distr::cdf("rayl", x, {1.0});
```

## `rice`

### Signature

```cpp
pluss::distr::sample("rice", {nu, sigma}, sizes, cfg, seed);
pluss::distr::sample_like("rice", {nu, sigma}, like, seed);
pluss::distr::pdf("rice", x, {nu, sigma});
pluss::distr::logpdf("rice", x, {nu, sigma});
pluss::distr::cdf("rice", x, {nu, sigma});
```

### Notes

Non-central magnitude family often used in signal models.

Aliases: `rician`.

Parameters: `{nu, sigma}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.5, 2.5}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("rice", {1.0, 1.0}, {6}, cfg, 51ULL);
auto like_draws = pluss::distr::sample_like("rice", {1.0, 1.0}, like, 51ULL);
auto dens = pluss::distr::pdf("rice", x, {1.0, 1.0});
auto logprob = pluss::distr::logpdf("rice", x, {1.0, 1.0});
auto dist = pluss::distr::cdf("rice", x, {1.0, 1.0});
```

## `semicircle`

### Signature

```cpp
pluss::distr::sample("semicircle", {mu, radius}, sizes, cfg, seed);
pluss::distr::sample_like("semicircle", {mu, radius}, like, seed);
pluss::distr::pdf("semicircle", x, {mu, radius});
pluss::distr::logpdf("semicircle", x, {mu, radius});
pluss::distr::cdf("semicircle", x, {mu, radius});
```

### Notes

Compact-support semicircle family.

Aliases: `semicirc, wigner`.

Parameters: `{mu, radius}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("semicirc", {0.0, 1.0}, {6}, cfg, 52ULL);
auto like_draws = pluss::distr::sample_like("semicirc", {0.0, 1.0}, like, 52ULL);
auto dens = pluss::distr::pdf("semicirc", x, {0.0, 1.0});
auto logprob = pluss::distr::logpdf("semicirc", x, {0.0, 1.0});
auto dist = pluss::distr::cdf("semicirc", x, {0.0, 1.0});
```

## `skellam`

### Signature

```cpp
pluss::distr::sample("skellam", {mu1, mu2}, sizes, cfg, seed);
pluss::distr::sample_like("skellam", {mu1, mu2}, like, seed);
pluss::distr::pdf("skellam", x, {mu1, mu2});
pluss::distr::logpdf("skellam", x, {mu1, mu2});
pluss::distr::cdf("skellam", x, {mu1, mu2});
```

### Notes

Difference of two independent Poisson counts.

Aliases: none.

Parameters: `{mu1, mu2}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("skellam", {2.0, 1.0}, {6}, cfg, 53ULL);
auto like_draws = pluss::distr::sample_like("skellam", {2.0, 1.0}, like, 53ULL);
auto pmf = pluss::distr::pdf("skellam", x, {2.0, 1.0});
auto logprob = pluss::distr::logpdf("skellam", x, {2.0, 1.0});
auto dist = pluss::distr::cdf("skellam", x, {2.0, 1.0});
```

## `tukeylambda`

### Signature

```cpp
pluss::distr::sample("tukeylambda", {lambda}, sizes, cfg, seed);
pluss::distr::sample_like("tukeylambda", {lambda}, like, seed);
pluss::distr::pdf("tukeylambda", x, {lambda});
pluss::distr::logpdf("tukeylambda", x, {lambda});
pluss::distr::cdf("tukeylambda", x, {lambda});
```

### Notes

Flexible shape family controlled by the lambda tail parameter.

Aliases: none.

Parameters: `{lambda}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("tukeylambda", {0.14}, {6}, cfg, 54ULL);
auto like_draws = pluss::distr::sample_like("tukeylambda", {0.14}, like, 54ULL);
auto dens = pluss::distr::pdf("tukeylambda", x, {0.14});
auto logprob = pluss::distr::logpdf("tukeylambda", x, {0.14});
auto dist = pluss::distr::cdf("tukeylambda", x, {0.14});
```

## `ushape`

### Signature

```cpp
pluss::distr::sample("ushape", {}, sizes, cfg, seed);
pluss::distr::sample_like("ushape", {}, like, seed);
pluss::distr::pdf("ushape", x, {});
pluss::distr::logpdf("ushape", x, {});
pluss::distr::cdf("ushape", x, {});
```

### Notes

Symmetric U-shaped bounded family.

Aliases: `u`.

Parameters: `{}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.1, 0.5, 0.9}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("u", {}, {6}, cfg, 55ULL);
auto like_draws = pluss::distr::sample_like("u", like, 55ULL);
auto dens = pluss::distr::pdf("u", x, {});
auto logprob = pluss::distr::logpdf("u", x, {});
auto dist = pluss::distr::cdf("u", x, {});
```

## `vonmises`

### Signature

```cpp
pluss::distr::sample("vonmises", {mu, kappa}, sizes, cfg, seed);
pluss::distr::sample_like("vonmises", {mu, kappa}, like, seed);
pluss::distr::pdf("vonmises", x, {mu, kappa});
pluss::distr::logpdf("vonmises", x, {mu, kappa});
pluss::distr::cdf("vonmises", x, {mu, kappa});
```

### Notes

Circular distribution on angles.

Aliases: none.

Parameters: `{mu, kappa}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("vonmises", {0.0, 2.0}, {6}, cfg, 56ULL);
auto like_draws = pluss::distr::sample_like("vonmises", {0.0, 2.0}, like, 56ULL);
auto dens = pluss::distr::pdf("vonmises", x, {0.0, 2.0});
auto logprob = pluss::distr::logpdf("vonmises", x, {0.0, 2.0});
auto dist = pluss::distr::cdf("vonmises", x, {0.0, 2.0});
```

## `wald`

### Signature

```cpp
pluss::distr::sample("wald", {mu}, sizes, cfg, seed);
pluss::distr::sample_like("wald", {mu}, like, seed);
pluss::distr::pdf("wald", x, {mu});
pluss::distr::logpdf("wald", x, {mu});
pluss::distr::cdf("wald", x, {mu});
```

### Notes

Single-parameter Wald special case exposed by the runtime.

Aliases: none.

Parameters: `{mu}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("wald", {2.0}, {6}, cfg, 57ULL);
auto like_draws = pluss::distr::sample_like("wald", {2.0}, like, 57ULL);
auto dens = pluss::distr::pdf("wald", x, {2.0});
auto logprob = pluss::distr::logpdf("wald", x, {2.0});
auto dist = pluss::distr::cdf("wald", x, {2.0});
```

## `weibull`

### Signature

```cpp
pluss::distr::sample("weibull", {shape, scale}, sizes, cfg, seed);
pluss::distr::sample("weibull", {theta, alpha, beta}, sizes, cfg, seed);
pluss::distr::sample_like("weibull", {shape, scale}, like, seed);
pluss::distr::sample_like("weibull", {theta, alpha, beta}, like, seed);
pluss::distr::pdf("weibull", x, {shape, scale});
pluss::distr::pdf("weibull", x, {theta, alpha, beta});
pluss::distr::logpdf("weibull", x, {shape, scale});
pluss::distr::logpdf("weibull", x, {theta, alpha, beta});
pluss::distr::cdf("weibull", x, {shape, scale});
pluss::distr::cdf("weibull", x, {theta, alpha, beta});
```

### Notes

Supports both the short two-parameter Weibull form and the legacy three-parameter location-scale-shape form.

Aliases: `wbl, frechet`.

Parameters: `{shape, scale} or {theta, alpha, beta}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("weibull", {0.0, 2.0, 1.5}, {6}, cfg, 58ULL);
auto like_draws = pluss::distr::sample_like("weibull", {0.0, 2.0, 1.5}, like, 58ULL);
auto dens = pluss::distr::pdf("weibull", x, {0.0, 2.0, 1.5});
auto logprob = pluss::distr::logpdf("weibull", x, {0.0, 2.0, 1.5});
auto dist = pluss::distr::cdf("weibull", x, {0.0, 2.0, 1.5});
```

## `yule`

### Signature

```cpp
pluss::distr::sample("yule", {rho}, sizes, cfg, seed);
pluss::distr::sample_like("yule", {rho}, like, seed);
pluss::distr::pdf("yule", x, {rho});
pluss::distr::logpdf("yule", x, {rho});
pluss::distr::cdf("yule", x, {rho});
```

### Notes

Heavy-tailed discrete Yule-Simon family.

Aliases: `yulesimon`.

Parameters: `{rho}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({1.0, 2.0, 3.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("yule", {3.0}, {6}, cfg, 59ULL);
auto like_draws = pluss::distr::sample_like("yule", {3.0}, like, 59ULL);
auto pmf = pluss::distr::pdf("yule", x, {3.0});
auto logprob = pluss::distr::logpdf("yule", x, {3.0});
auto dist = pluss::distr::cdf("yule", x, {3.0});
```

## `zeta`

### Signature

```cpp
pluss::distr::sample("zeta", {s}, sizes, cfg, seed);
pluss::distr::sample_like("zeta", {s}, like, seed);
pluss::distr::pdf("zeta", x, {s});
pluss::distr::logpdf("zeta", x, {s});
pluss::distr::cdf("zeta", x, {s});
```

### Notes

Zipf/Zeta count family. Use values at or above 1 for the mass function.

Aliases: `zipf`.

Parameters: `{s}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({1.0, 2.0, 3.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("zipf", {2.5}, {6}, cfg, 60ULL);
auto like_draws = pluss::distr::sample_like("zipf", {2.5}, like, 60ULL);
auto pmf = pluss::distr::pdf("zipf", x, {2.5});
auto logprob = pluss::distr::logpdf("zipf", x, {2.5});
auto dist = pluss::distr::cdf("zipf", x, {2.5});
```

## `gig`

### Signature

```cpp
pluss::distr::sample("gig", {lambda, chi, psi}, sizes, cfg, seed);
pluss::distr::sample_like("gig", {lambda, chi, psi}, like, seed);
pluss::distr::pdf("gig", x, {lambda, chi, psi});
pluss::distr::logpdf("gig", x, {lambda, chi, psi});
pluss::distr::cdf("gig", x, {lambda, chi, psi});
```

### Notes

Generalized inverse Gaussian family on the positive real line. `lambda` selects the subfamily, while `chi` and `psi` control the inverse-Gaussian versus gamma-like pull on the tails. In practice this is the workhorse mixing law behind several heavier-tailed continuous models, including the generalized hyperbolic family.

Aliases: none.

Parameters: `{lambda, chi, psi}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({0.5, 1.0, 2.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("gig", {0.5, 1.0, 2.0}, {6}, cfg, 61ULL);
auto like_draws = pluss::distr::sample_like("gig", {0.5, 1.0, 2.0}, like, 61ULL);
auto dens = pluss::distr::pdf("gig", x, {0.5, 1.0, 2.0});
auto logprob = pluss::distr::logpdf("gig", x, {0.5, 1.0, 2.0});
auto dist = pluss::distr::cdf("gig", x, {0.5, 1.0, 2.0});
```

## `gh`

### Signature

```cpp
pluss::distr::sample("gh", {lambda, alpha, beta, mu, delta}, sizes, cfg, seed);
pluss::distr::sample_like("gh", {lambda, alpha, beta, mu, delta}, like, seed);
pluss::distr::pdf("gh", x, {lambda, alpha, beta, mu, delta});
pluss::distr::logpdf("gh", x, {lambda, alpha, beta, mu, delta});
pluss::distr::cdf("gh", x, {lambda, alpha, beta, mu, delta});
```

### Notes

Five-parameter generalized hyperbolic family for skewed, heavy-tailed continuous data. `lambda` chooses the subclass, `alpha` and `beta` control tail weight and asymmetry, and `mu` / `delta` provide location and scale. This is the flexible parent family behind special cases like NIG.

Aliases: none.

Parameters: `{lambda, alpha, beta, mu, delta}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("gh", {1.0, 2.0, 0.5, 0.0, 1.0}, {6}, cfg, 62ULL);
auto like_draws = pluss::distr::sample_like("gh", {1.0, 2.0, 0.5, 0.0, 1.0}, like, 62ULL);
auto dens = pluss::distr::pdf("gh", x, {1.0, 2.0, 0.5, 0.0, 1.0});
auto logprob = pluss::distr::logpdf("gh", x, {1.0, 2.0, 0.5, 0.0, 1.0});
auto dist = pluss::distr::cdf("gh", x, {1.0, 2.0, 0.5, 0.0, 1.0});
```

## `nig`

### Signature

```cpp
pluss::distr::sample("nig", {alpha, beta, mu, delta}, sizes, cfg, seed);
pluss::distr::sample_like("nig", {alpha, beta, mu, delta}, like, seed);
pluss::distr::pdf("nig", x, {alpha, beta, mu, delta});
pluss::distr::logpdf("nig", x, {alpha, beta, mu, delta});
pluss::distr::cdf("nig", x, {alpha, beta, mu, delta});
```

### Notes

Normal-inverse-Gaussian special case of the generalized hyperbolic family, with `alpha > |beta|` required by the runtime. Use it when you need a continuous law that can stay much more tail-heavy and skewed than a Gaussian while still supporting direct sampling, density evaluation, and object-based cumulant generation through `pluss::distr::Nig`.

Aliases: none.

Parameters: `{alpha, beta, mu, delta}`.

Use `sample_like(...)` when you want the sampled output to inherit shape, dtype, and platform from an existing `Series`.

### Examples

```cpp
auto cfg = pluss::Config{}
    .with_dtype(pluss::ScalarType::Float64)
    .with_platform(pluss::pCPU);
auto like = pluss::zeros({2, 4}, cfg.with_dtype(pluss::ScalarType::Float32));

auto x = pluss::make_series({-1.0, 0.0, 1.0}).to(pluss::ScalarType::Float64);
auto draws = pluss::distr::sample("nig", {2.0, 0.5, 0.0, 1.0}, {6}, cfg, 63ULL);
auto like_draws = pluss::distr::sample_like("nig", {2.0, 0.5, 0.0, 1.0}, like, 63ULL);
auto dens = pluss::distr::pdf("nig", x, {2.0, 0.5, 0.0, 1.0});
auto logprob = pluss::distr::logpdf("nig", x, {2.0, 0.5, 0.0, 1.0});
auto dist = pluss::distr::cdf("nig", x, {2.0, 0.5, 0.0, 1.0});
```
