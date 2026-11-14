# Object API Reference

The object wrappers bind distribution parameters once, then expose a method surface that stays on `Series` throughout:

- `sample(...)`
- `sample_like(...)`
- `pdf(...)`
- `logpdf(...)`
- `cdf(...)`
- `icdf(...)` through the generic `Distribution` wrapper
- `logcdf(...)`
- `cgf(...)`
- `cumulant(...)`

Use them when the distribution itself is part of the model state and repeated calls should not keep re-spelling the parameter list.

All `Output` blocks below were verified from the compiled demo [`distributions_objects_demo.cpp`](../../examples/distributions_objects_demo.cpp) using the fixed seed `20260303ULL` on CPU. If you change dtype, platform, or seed, sampled values will change.

## `Normal`

### Signature

```cpp
pluss::distr::Normal normal{mean, stddev};

normal.sample(sizes, cfg = {}, seed = 0);
normal.sample_like(like, seed = 0);
normal.pdf(x);
normal.logpdf(x);
normal.cdf(x);
normal.logcdf(x);
normal.cgf(t);
normal.cumulant(order);
```

### Notes

Represents a Gaussian with fixed `mean` and `stddev`. `cumulant(1)` returns the mean, `cumulant(2)` returns the variance, and higher orders are zero, so it is a good baseline for cumulant-driven code paths.

### Examples

```cpp
pluss::distr::Normal normal{0.0, 1.0};
auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = normal.pdf(x);
auto logdens = normal.logpdf(x);
auto dist = normal.cdf(x);
auto logtail = normal.logcdf(x);
auto cumulants = normal.cgf(t);
auto k1 = normal.cumulant(1);
auto k2 = normal.cumulant(2);
auto draws = normal.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = normal.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.241971, 0.398942, 0.352065, 0.241971]
logpdf(x):   [-1.418939, -0.918939, -1.043939, -1.418939]
cdf(x):      [0.158655, 0.500000, 0.691462, 0.841345]
logcdf(x):   [-1.841022, -0.693147, -0.368946, -0.172754]
cgf(t):      [0.031250, 0.000000, 0.031250]
cumulants:   [0, 1, 0, 0]
sample({4}): [0.912964, -0.916896, -1.181625, 0.307058]
sample_like: [0.912964, -0.916896, -1.181625, 0.307058] dtype=11
```

## `Bernoulli`

### Signature

```cpp
pluss::distr::Bernoulli bernoulli{p};

bernoulli.sample(sizes, cfg = {}, seed = 0);
bernoulli.sample_like(like, seed = 0);
bernoulli.pdf(x);
bernoulli.logpdf(x);
bernoulli.cdf(x);
bernoulli.logcdf(x);
bernoulli.cgf(t);
bernoulli.cumulant(order);
```

### Notes

Binds a success probability once and exposes the full 0/1 event model. `cumulant(...)` is implemented from Bernoulli raw moments, so higher orders remain available.

### Examples

```cpp
pluss::distr::Bernoulli bernoulli{0.25};
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto pmf = bernoulli.pdf(x);
auto logprob = bernoulli.logpdf(x);
auto dist = bernoulli.cdf(x);
auto logtail = bernoulli.logcdf(x);
auto cgf = bernoulli.cgf(t);
auto k1 = bernoulli.cumulant(1);
auto k2 = bernoulli.cumulant(2);
auto draws = bernoulli.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = bernoulli.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.750000, 0.250000, 0.000000, 0.000000]
logpdf(x):   [-0.287682, -1.386294, -inf, -inf]
cdf(x):      [0.750000, 1.000000, 1.000000, 1.000000]
logcdf(x):   [-0.287682, 0.000000, 0.000000, 0.000000]
cgf(t):      [-0.056888, 0.000000, 0.068599]
cumulants:   [0.25, 0.1875, 0.09375, -0.0234375]
sample({4}): [0.000000, 0.000000, 0.000000, 0.000000]
sample_like: [0.000000, 0.000000, 0.000000, 0.000000] dtype=11
```

## `Binomial`

### Signature

```cpp
pluss::distr::Binomial binomial{count, p};

binomial.sample(sizes, cfg = {}, seed = 0);
binomial.sample_like(like, seed = 0);
binomial.pdf(x);
binomial.logpdf(x);
binomial.cdf(x);
binomial.logcdf(x);
binomial.cgf(t);
binomial.cumulant(order);
```

### Notes

Models the number of successes across `count` Bernoulli trials with success rate `p`. `cumulant(order)` reuses the Bernoulli cumulants and scales them by `count`.

### Examples

```cpp
pluss::distr::Binomial binomial{4, 0.25};
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto pmf = binomial.pdf(x);
auto logprob = binomial.logpdf(x);
auto dist = binomial.cdf(x);
auto logtail = binomial.logcdf(x);
auto cgf = binomial.cgf(t);
auto k1 = binomial.cumulant(1);
auto k2 = binomial.cumulant(2);
auto draws = binomial.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = binomial.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.316406, 0.421875, 0.210937, 0.046875]
logpdf(x):   [-1.150728, -0.863046, -1.556193, -3.060271]
cdf(x):      [0.316406, 0.738281, 0.949219, 0.996094]
logcdf(x):   [-1.150728, -0.303430, -0.052116, -0.003914]
cgf(t):      [-0.227551, 0.000000, 0.274395]
cumulants:   [1, 0.75, 0.375, -0.09375]
sample({4}): [3.000000, 2.000000, 0.000000, 3.000000]
sample_like: [3.000000, 2.000000, 0.000000, 3.000000] dtype=11
```

## `Poisson`

### Signature

```cpp
pluss::distr::Poisson poisson{lambda};

poisson.sample(sizes, cfg = {}, seed = 0);
poisson.sample_like(like, seed = 0);
poisson.pdf(x);
poisson.logpdf(x);
poisson.cdf(x);
poisson.logcdf(x);
poisson.cgf(t);
poisson.cumulant(order);
```

### Notes

Wraps the Poisson count model for a fixed event rate `lambda`. Every cumulant equals `lambda`, which makes this object convenient in count models that consume cumulants directly.

### Examples

```cpp
pluss::distr::Poisson poisson{2.5};
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto pmf = poisson.pdf(x);
auto logprob = poisson.logpdf(x);
auto dist = poisson.cdf(x);
auto logtail = poisson.logcdf(x);
auto cgf = poisson.cgf(t);
auto k1 = poisson.cumulant(1);
auto k2 = poisson.cumulant(2);
auto draws = poisson.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = poisson.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.082085, 0.205212, 0.256516, 0.213763]
logpdf(x):   [-2.500000, -1.583709, -1.360566, -1.542887]
cdf(x):      [0.082085, 0.287297, 0.543813, 0.757576]
logcdf(x):   [-2.500000, -1.247237, -0.609150, -0.277631]
cgf(t):      [-0.552998, 0.000000, 0.710064]
cumulants:   [2.5, 2.5, 2.5, 2.5]
sample({4}): [7.000000, 0.000000, 4.000000, 4.000000]
sample_like: [7.000000, 0.000000, 4.000000, 4.000000] dtype=11
```

## `Geometric`

### Signature

```cpp
pluss::distr::Geometric geometric{p};

geometric.sample(sizes, cfg = {}, seed = 0);
geometric.sample_like(like, seed = 0);
geometric.pdf(x);
geometric.logpdf(x);
geometric.cdf(x);
geometric.logcdf(x);
geometric.cgf(t);
geometric.cumulant(order);
```

### Notes

Uses the success-probability parameterization of the geometric law. The demo evaluates it on positive counts because support starts at 1 in this implementation.

### Examples

```cpp
pluss::distr::Geometric geometric{0.25};
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto pmf = geometric.pdf(x);
auto logprob = geometric.logpdf(x);
auto dist = geometric.cdf(x);
auto logtail = geometric.logcdf(x);
auto cgf = geometric.cgf(t);
auto k1 = geometric.cumulant(1);
auto k2 = geometric.cumulant(2);
auto draws = geometric.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = geometric.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.250000, 0.187500, 0.140625, 0.105469]
logpdf(x):   [-1.386294, -1.673976, -1.961659, -2.249341]
cdf(x):      [0.250000, 0.437500, 0.578125, 0.683594]
logcdf(x):   [-1.386294, -0.826679, -0.547965, -0.380391]
cgf(t):      [-0.758983, 0.000000, 2.161058]
cumulants:   [4, 12, 84, 876]
sample({4}): [2.000000, 8.000000, 3.000000, 3.000000]
sample_like: [2.000000, 8.000000, 3.000000, 3.000000] dtype=11
```

## `NegativeBinomial`

### Signature

```cpp
pluss::distr::NegativeBinomial negative_binomial{r, p};

negative_binomial.sample(sizes, cfg = {}, seed = 0);
negative_binomial.sample_like(like, seed = 0);
negative_binomial.pdf(x);
negative_binomial.logpdf(x);
negative_binomial.cdf(x);
negative_binomial.logcdf(x);
negative_binomial.cgf(t);
negative_binomial.cumulant(order);
```

### Notes

Represents the negative-binomial count law with dispersion-like parameter `r` and success probability `p`. Use it when Poisson variance is too restrictive.

### Examples

```cpp
pluss::distr::NegativeBinomial negative_binomial{3.0, 0.4};
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto pmf = negative_binomial.pdf(x);
auto logprob = negative_binomial.logpdf(x);
auto dist = negative_binomial.cdf(x);
auto logtail = negative_binomial.logcdf(x);
auto cgf = negative_binomial.cgf(t);
auto k1 = negative_binomial.cumulant(1);
auto k2 = negative_binomial.cumulant(2);
auto draws = negative_binomial.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = negative_binomial.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.064000, 0.115200, 0.138240, 0.138240]
logpdf(x):   [-2.748872, -2.161086, -1.978764, -1.978764]
cdf(x):      [0.064000, 0.179200, 0.317440, 0.455680]
logcdf(x):   [-2.748872, -1.719253, -1.147466, -0.785964]
cgf(t):      [-0.859592, 0.000000, 1.665577]
cumulants:   [4.5, 11.25, 45, 264.375]
sample({4}): [4.000000, 8.000000, 1.000000, 9.000000]
sample_like: [4.000000, 8.000000, 1.000000, 9.000000] dtype=11
```

## `Gamma`

### Signature

```cpp
pluss::distr::Gamma gamma{shape, scale, location = 0.0};

gamma.sample(sizes, cfg = {}, seed = 0);
gamma.sample_like(like, seed = 0);
gamma.pdf(x);
gamma.logpdf(x);
gamma.cdf(x);
gamma.logcdf(x);
gamma.cgf(t);
gamma.cumulant(order);
```

### Notes

Encodes the shifted gamma family directly as object state. `cgf(...)` enforces the domain restriction `t < 1 / scale`, and `cumulant(order)` is available for every positive order.

### Examples

```cpp
pluss::distr::Gamma gamma{2.0, 1.0, 0.0};
auto x = pluss::make_series({0.5, 1.5, 2.0, 2.5});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = gamma.pdf(x);
auto logdens = gamma.logpdf(x);
auto dist = gamma.cdf(x);
auto logtail = gamma.logcdf(x);
auto cgf = gamma.cgf(t);
auto k1 = gamma.cumulant(1);
auto k2 = gamma.cumulant(2);
auto draws = gamma.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = gamma.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.303265, 0.334695, 0.270671, 0.205212]
logpdf(x):   [-1.193147, -1.094535, -1.306853, -1.583709]
cdf(x):      [0.090204, 0.442175, 0.593994, 0.712703]
logcdf(x):   [-2.405681, -0.816050, -0.520886, -0.338691]
cgf(t):      [-0.446287, 0.000000, 0.575364]
cumulants:   [2, 2, 4, 12]
sample({4}): [1.833701, 1.832495, 2.016090, 1.644138]
sample_like: [1.833701, 1.832494, 2.016090, 1.644138] dtype=11
```

## `ChiSquare`

### Signature

```cpp
pluss::distr::ChiSquare chi_square{degrees_of_freedom};

chi_square.sample(sizes, cfg = {}, seed = 0);
chi_square.sample_like(like, seed = 0);
chi_square.pdf(x);
chi_square.logpdf(x);
chi_square.cdf(x);
chi_square.logcdf(x);
chi_square.cgf(t);
chi_square.cumulant(order);
```

### Notes

Wraps the chi-square family in terms of degrees of freedom. Its cumulants grow quickly with order, so it is useful when you need variance and tail-heavy shape diagnostics from the same object.

### Examples

```cpp
pluss::distr::ChiSquare chi_square{5};
auto x = pluss::make_series({1.0, 2.0, 2.5, 3.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = chi_square.pdf(x);
auto logdens = chi_square.logpdf(x);
auto dist = chi_square.cdf(x);
auto logtail = chi_square.logcdf(x);
auto cgf = chi_square.cgf(t);
auto k1 = chi_square.cumulant(1);
auto k2 = chi_square.cumulant(2);
auto draws = chi_square.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = chi_square.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.080657, 0.138369, 0.150602, 0.154180]
logpdf(x):   [-2.517551, -1.977830, -1.893115, -1.869632]
cdf(x):      [0.037434, 0.150855, 0.223505, 0.300014]
logcdf(x):   [-3.285170, -1.891436, -1.498322, -1.203926]
cgf(t):      [-1.013663, 0.000000, 1.732868]
cumulants:   [5, 10, 40, 240]
sample({4}): [4.890656, 4.887920, 5.304276, 4.460768]
sample_like: [4.890656, 4.887920, 5.304276, 4.460768] dtype=11
```

## `InverseGaussian`

### Signature

```cpp
pluss::distr::InverseGaussian inverse_gaussian{theta, chi};

inverse_gaussian.sample(sizes, cfg = {}, seed = 0);
inverse_gaussian.sample_like(like, seed = 0);
inverse_gaussian.pdf(x);
inverse_gaussian.logpdf(x);
inverse_gaussian.cdf(x);
inverse_gaussian.logcdf(x);
inverse_gaussian.cgf(t);
inverse_gaussian.cumulant(order);
```

### Notes

Represents the inverse-Gaussian family with mean-like parameter `theta` and shape parameter `chi`. The wrapper exposes exact lower-order cumulants, which is useful in normal-variance-mixture workflows.

### Examples

```cpp
pluss::distr::InverseGaussian inverse_gaussian{1.5, 3.0};
auto x = pluss::make_series({0.5, 1.5, 2.0, 2.5});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = inverse_gaussian.pdf(x);
auto logdens = inverse_gaussian.logpdf(x);
auto dist = inverse_gaussian.cdf(x);
auto logtail = inverse_gaussian.logcdf(x);
auto cgf = inverse_gaussian.cgf(t);
auto k1 = inverse_gaussian.cumulant(1);
auto k2 = inverse_gaussian.cumulant(2);
auto draws = inverse_gaussian.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = inverse_gaussian.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.515177, 0.376126, 0.224768, 0.133890]
logpdf(x):   [-0.663245, -0.977830, -1.492686, -2.010735]
cdf(x):      [0.081014, 0.627698, 0.774932, 0.862588]
logcdf(x):   [-2.513133, -0.465696, -0.254980, -0.147819]
cgf(t):      [-0.345208, 0.000000, 0.418861]
cumulants:   [1.5, 1.125, 2.53125, 9.49219]
sample({4}): [1.646021, 1.509892, 0.347783, 1.907487]
sample_like: [1.646021, 1.509892, 0.347783, 1.907487] dtype=11
```

## `Lognormal`

### Signature

```cpp
pluss::distr::Lognormal lognormal{mean_log, stddev_log};

lognormal.sample(sizes, cfg = {}, seed = 0);
lognormal.sample_like(like, seed = 0);
lognormal.pdf(x);
lognormal.logpdf(x);
lognormal.cdf(x);
lognormal.logcdf(x);
lognormal.cgf(t);
lognormal.cumulant(order);
```

### Notes

Stores the normal parameters in log-space and evaluates on the positive real line. The CGF is only documented on the nonpositive grid used in the demo because the ordinary moment generating function diverges for positive arguments.

### Examples

```cpp
pluss::distr::Lognormal lognormal{0.1, 0.4};
auto x = pluss::make_series({0.5, 1.5, 2.0, 2.5});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.5, -0.25, 0.0});

auto dens = lognormal.pdf(x);
auto logdens = lognormal.logpdf(x);
auto dist = lognormal.cdf(x);
auto logtail = lognormal.logcdf(x);
auto cgf = lognormal.cgf(t);
auto k1 = lognormal.cumulant(1);
auto k2 = lognormal.cumulant(2);
auto draws = lognormal.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = lognormal.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.279324, 0.496733, 0.166087, 0.049726]
logpdf(x):   [-1.275383, -0.699703, -1.795244, -3.001222]
cdf(x):      [0.023691, 0.777466, 0.930945, 0.979361]
logcdf(x):   [-3.742656, -0.251716, -0.071555, -0.020855]
cgf(t):      [-0.570493, -0.291929, 0.000000]
cumulants:   [1.19722, 0.248698, 0.16395, 0.201634]
sample({4}): [1.592310, 0.765858, 0.688906, 1.249600]
sample_like: [1.592310, 0.765858, 0.688906, 1.249600] dtype=11
```

## `Uniform`

### Signature

```cpp
pluss::distr::Uniform uniform{low, high};

uniform.sample(sizes, cfg = {}, seed = 0);
uniform.sample_like(like, seed = 0);
uniform.pdf(x);
uniform.logpdf(x);
uniform.cdf(x);
uniform.logcdf(x);
uniform.cgf(t);
uniform.cumulant(order);
```

### Notes

Represents a bounded continuous law on `[low, high]`. Higher-order cumulants come from the interval raw moments, so the object remains useful beyond simple sampling.

### Examples

```cpp
pluss::distr::Uniform uniform{0.0, 1.0};
auto x = pluss::make_series({0.0, 0.5, 0.75, 1.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = uniform.pdf(x);
auto logdens = uniform.logpdf(x);
auto dist = uniform.cdf(x);
auto logtail = uniform.logcdf(x);
auto cgf = uniform.cgf(t);
auto k1 = uniform.cumulant(1);
auto k2 = uniform.cumulant(2);
auto draws = uniform.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = uniform.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [1.000000, 1.000000, 1.000000, 1.000000]
logpdf(x):   [-0.000000, -0.000000, -0.000000, -0.000000]
cdf(x):      [0.000000, 0.500000, 0.750000, 1.000000]
logcdf(x):   [-inf, -0.693147, -0.287682, 0.000000]
cgf(t):      [-0.122397, 0.000000, 0.127603]
cumulants:   [0.5, 0.0833333, 2.77556e-17, -0.00833333]
sample({4}): [0.432964, 0.874658, 0.474611, 0.459537]
sample_like: [0.432964, 0.874658, 0.474611, 0.459537] dtype=11
```

## `Exponential`

### Signature

```cpp
pluss::distr::Exponential exponential{rate};

exponential.sample(sizes, cfg = {}, seed = 0);
exponential.sample_like(like, seed = 0);
exponential.pdf(x);
exponential.logpdf(x);
exponential.cdf(x);
exponential.logcdf(x);
exponential.cgf(t);
exponential.cumulant(order);
```

### Notes

Uses the rate parameterization. `cgf(...)` enforces `t < rate`, and `cumulant(order)` follows the closed-form exponential cumulants.

### Examples

```cpp
pluss::distr::Exponential exponential{2.0};
auto x = pluss::make_series({0.5, 1.5, 2.0, 2.5});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = exponential.pdf(x);
auto logdens = exponential.logpdf(x);
auto dist = exponential.cdf(x);
auto logtail = exponential.logcdf(x);
auto cgf = exponential.cgf(t);
auto k1 = exponential.cumulant(1);
auto k2 = exponential.cumulant(2);
auto draws = exponential.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = exponential.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.735759, 0.099574, 0.036631, 0.013476]
logpdf(x):   [-0.306853, -2.306853, -3.306853, -4.306853]
cdf(x):      [0.632121, 0.950213, 0.981684, 0.993262]
logcdf(x):   [-0.458675, -0.051069, -0.018485, -0.006761]
cgf(t):      [-0.117783, 0.000000, 0.133531]
cumulants:   [0.5, 0.25, 0.25, 0.375]
sample({4}): [0.418551, 0.066961, 0.372630, 0.388768]
sample_like: [0.418551, 0.066961, 0.372630, 0.388768] dtype=11
```

## `Laplace`

### Signature

```cpp
pluss::distr::Laplace laplace{location, scale};

laplace.sample(sizes, cfg = {}, seed = 0);
laplace.sample_like(like, seed = 0);
laplace.pdf(x);
laplace.logpdf(x);
laplace.cdf(x);
laplace.logcdf(x);
laplace.cgf(t);
laplace.cumulant(order);
```

### Notes

Represents the double-exponential law centered at `location`. Odd cumulants above the mean vanish, which makes the wrapper a clean symmetric heavy-tail reference.

### Examples

```cpp
pluss::distr::Laplace laplace{0.0, 0.5};
auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = laplace.pdf(x);
auto logdens = laplace.logpdf(x);
auto dist = laplace.cdf(x);
auto logtail = laplace.logcdf(x);
auto cgf = laplace.cgf(t);
auto k1 = laplace.cumulant(1);
auto k2 = laplace.cumulant(2);
auto draws = laplace.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = laplace.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.135335, 1.000000, 0.367879, 0.135335]
logpdf(x):   [-2.000000, -0.000000, -1.000000, -2.000000]
cdf(x):      [0.067668, 0.500000, 0.816060, 0.932332]
logcdf(x):   [-2.693147, -0.693147, -0.203267, -0.070066]
cgf(t):      [0.015748, 0.000000, 0.015748]
cumulants:   [0, 0.5, 0, 0.75]
sample({4}): [-0.071977, 0.691781, -0.026057, -0.042195]
sample_like: [-0.071977, 0.691781, -0.026057, -0.042195] dtype=11
```

## `Logistic`

### Signature

```cpp
pluss::distr::Logistic logistic{location, scale};

logistic.sample(sizes, cfg = {}, seed = 0);
logistic.sample_like(like, seed = 0);
logistic.pdf(x);
logistic.logpdf(x);
logistic.cdf(x);
logistic.logcdf(x);
logistic.cgf(t);
logistic.cumulant(order);
```

### Notes

Wraps the logistic family with explicit location and scale. It is a good alternative to `Normal` when you want similar symmetry but heavier shoulders and tails.

### Examples

```cpp
pluss::distr::Logistic logistic{0.0, 0.75};
auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = logistic.pdf(x);
auto logdens = logistic.logpdf(x);
auto dist = logistic.cdf(x);
auto logtail = logistic.logcdf(x);
auto cgf = logistic.cgf(t);
auto k1 = logistic.cumulant(1);
auto k2 = logistic.cumulant(2);
auto draws = logistic.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = logistic.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.220121, 0.333333, 0.298877, 0.220121]
logpdf(x):   [-1.513576, -1.098612, -1.207725, -1.513576]
cdf(x):      [0.208609, 0.500000, 0.660756, 0.791391]
logcdf(x):   [-1.567296, -0.693147, -0.414370, -0.233963]
cgf(t):      [0.058514, 0.000000, 0.058514]
cumulants:   [0, 1.85055, 0, 4.10945]
sample({4}): [-0.202327, 1.457090, -0.076234, -0.121656]
sample_like: [-0.202327, 1.457090, -0.076234, -0.121656] dtype=11
```

## `Rayleigh`

### Signature

```cpp
pluss::distr::Rayleigh rayleigh{sigma};

rayleigh.sample(sizes, cfg = {}, seed = 0);
rayleigh.sample_like(like, seed = 0);
rayleigh.pdf(x);
rayleigh.logpdf(x);
rayleigh.cdf(x);
rayleigh.logcdf(x);
rayleigh.cgf(t);
rayleigh.cumulant(order);
```

### Notes

Represents the magnitude distribution induced by two independent centered Gaussians. Use it for strictly positive radial data with a single scale parameter.

### Examples

```cpp
pluss::distr::Rayleigh rayleigh{1.0};
auto x = pluss::make_series({0.5, 1.5, 2.0, 2.5});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = rayleigh.pdf(x);
auto logdens = rayleigh.logpdf(x);
auto dist = rayleigh.cdf(x);
auto logtail = rayleigh.logcdf(x);
auto cgf = rayleigh.cgf(t);
auto k1 = rayleigh.cumulant(1);
auto k2 = rayleigh.cumulant(2);
auto draws = rayleigh.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = rayleigh.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.441248, 0.486979, 0.270671, 0.109842]
logpdf(x):   [-0.818147, -0.719535, -1.306853, -2.208709]
cdf(x):      [0.117503, 0.675348, 0.864665, 0.956063]
logcdf(x):   [-2.141291, -0.392528, -0.145413, -0.044931]
cgf(t):      [-0.300370, 0.000000, 0.327210]
cumulants:   [1.25331, 0.429204, 0.17746, 0.0451493]
sample({4}): [1.293910, 0.517537, 1.220869, 1.247025]
sample_like: [1.293910, 0.517537, 1.220869, 1.247025] dtype=11
```

## `NormalTrunc`

### Signature

```cpp
pluss::distr::NormalTrunc normal_trunc{lower, upper, mean, stddev};

normal_trunc.sample(sizes, cfg = {}, seed = 0);
normal_trunc.sample_like(like, seed = 0);
normal_trunc.pdf(x);
normal_trunc.logpdf(x);
normal_trunc.cdf(x);
normal_trunc.logcdf(x);
normal_trunc.cgf(t);
normal_trunc.cumulant(order);
```

### Notes

Represents a Gaussian truncated to `[lower, upper]`. The support bounds change both the normalization constant and the low-order cumulants, so it is useful when hard limits are part of the model.

### Examples

```cpp
pluss::distr::NormalTrunc normal_trunc{-1.0, 1.0, 0.0, 1.0};
auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = normal_trunc.pdf(x);
auto logdens = normal_trunc.logpdf(x);
auto dist = normal_trunc.cdf(x);
auto logtail = normal_trunc.logcdf(x);
auto cgf = normal_trunc.cgf(t);
auto k1 = normal_trunc.cumulant(1);
auto k2 = normal_trunc.cumulant(2);
auto draws = normal_trunc.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = normal_trunc.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.354437, 0.584369, 0.515703, 0.354437]
logpdf(x):   [-1.037223, -0.537223, -0.662223, -1.037223]
cdf(x):      [0.000000, 0.500000, 0.780453, 1.000000]
logcdf(x):   [-inf, -0.693147, -0.247880, 0.000000]
cgf(t):      [0.009083, 0.000000, 0.009083]
cumulants:   [0, 0.291125, 0, -0.0897611]
sample({4}): [-0.114969, 0.692866, -0.043461, -0.069298]
sample_like: [-0.114969, 0.692866, -0.043461, -0.069298] dtype=11
```

## `StudentT`

### Signature

```cpp
pluss::distr::StudentT student_t{degrees_of_freedom, location, scale};

student_t.sample(sizes, cfg = {}, seed = 0);
student_t.sample_like(like, seed = 0);
student_t.pdf(x);
student_t.logpdf(x);
student_t.cdf(x);
student_t.logcdf(x);
student_t.cgf(t);
student_t.cumulant(order);
```

### Notes

Represents a Student-t law with location and scale. In this implementation the CGF is only defined at `t = 0`, so use `pdf`, `cdf`, and `cumulant(...)` rather than expecting a full moment-generating surface.

### Examples

```cpp
pluss::distr::StudentT student_t{9.0, 0.0, 1.0};
auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({0.0});

auto dens = student_t.pdf(x);
auto logdens = student_t.logpdf(x);
auto dist = student_t.cdf(x);
auto logtail = student_t.logcdf(x);
auto cgf = student_t.cgf(t);
auto k1 = student_t.cumulant(1);
auto k2 = student_t.cumulant(2);
auto draws = student_t.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = student_t.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.229131, 0.388035, 0.338357, 0.229131]
logpdf(x):   [-1.473463, -0.946660, -1.083655, -1.473463]
cdf(x):      [0.171718, 0.500000, 0.685464, 0.828282]
logcdf(x):   [-1.761901, -0.693147, -0.377659, -0.188402]
cgf(t):      [0.000000]
cumulants:   [0, 1.28571, 0, 1.98367]
sample({4}): [0.127284, 1.882429, -0.485678, -0.595184]
sample_like: [0.127284, 1.882429, -0.485678, -0.595184] dtype=11
```

## `FDistribution`

### Signature

```cpp
pluss::distr::FDistribution f_dist{numerator_degrees_of_freedom, denominator_degrees_of_freedom};

f_dist.sample(sizes, cfg = {}, seed = 0);
f_dist.sample_like(like, seed = 0);
f_dist.pdf(x);
f_dist.logpdf(x);
f_dist.cdf(x);
f_dist.logcdf(x);
f_dist.cgf(t);
f_dist.cumulant(order);
```

### Notes

Models the ratio of two scaled chi-square variates. Like `StudentT`, the CGF is only defined at zero in this implementation, so most user code should rely on the density, CDF, and cumulants.

### Examples

```cpp
pluss::distr::FDistribution f_dist{5.0, 12.0};
auto x = pluss::make_series({0.5, 1.5, 2.0, 2.5});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({0.0});

auto dens = f_dist.pdf(x);
auto logdens = f_dist.logpdf(x);
auto dist = f_dist.cdf(x);
auto logtail = f_dist.logcdf(x);
auto cgf = f_dist.cgf(t);
auto k1 = f_dist.cumulant(1);
auto k2 = f_dist.cumulant(2);
auto draws = f_dist.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = f_dist.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.697776, 0.292235, 0.161377, 0.090342]
logpdf(x):   [-0.359857, -1.230198, -1.824013, -2.404158]
cdf(x):      [0.229236, 0.738883, 0.849074, 0.910176]
logcdf(x):   [-1.473003, -0.302615, -0.163609, -0.094117]
cgf(t):      [0.000000]
cumulants:   [1.2, 1.08, 3.456, 28.3824]
sample({4}): [0.901638, 1.032027, 0.245787, 0.217795]
sample_like: [0.901639, 1.032027, 0.245787, 0.217795] dtype=11
```

## `Weibull`

### Signature

```cpp
pluss::distr::Weibull weibull{shape, scale};

weibull.sample(sizes, cfg = {}, seed = 0);
weibull.sample_like(like, seed = 0);
weibull.pdf(x);
weibull.logpdf(x);
weibull.cdf(x);
weibull.logcdf(x);
weibull.cgf(t);
weibull.cumulant(order);
```

### Notes

Encodes the Weibull family directly as an object. It is the natural choice for lifetime and reliability models where shape controls whether hazard is increasing or decreasing.

### Examples

```cpp
pluss::distr::Weibull weibull{2.0, 1.5};
auto x = pluss::make_series({0.5, 1.5, 2.0, 2.5});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = weibull.pdf(x);
auto logdens = weibull.logpdf(x);
auto dist = weibull.cdf(x);
auto logtail = weibull.logcdf(x);
auto cgf = weibull.cgf(t);
auto k1 = weibull.cumulant(1);
auto k2 = weibull.cumulant(2);
auto draws = weibull.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = weibull.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.397706, 0.490506, 0.300468, 0.138170]
logpdf(x):   [-0.922041, -0.712318, -1.202414, -1.979270]
cdf(x):      [0.105161, 0.632121, 0.830987, 0.937823]
logcdf(x):   [-2.252266, -0.458675, -0.185142, -0.064194]
cgf(t):      [-0.317788, 0.000000, 0.347985]
cumulants:   [1.32934, 0.482854, 0.211753, 0.0571421]
sample({4}): [1.372398, 0.548931, 1.294927, 1.322670]
sample_like: [1.372398, 0.548931, 1.294927, 1.322670] dtype=11
```

## `Beta`

### Signature

```cpp
pluss::distr::Beta beta{low, high, alpha, beta};

beta.sample(sizes, cfg = {}, seed = 0);
beta.sample_like(like, seed = 0);
beta.pdf(x);
beta.logpdf(x);
beta.cdf(x);
beta.logcdf(x);
beta.cgf(t);
beta.cumulant(order);
```

### Notes

Represents a beta law rescaled onto `[low, high]`. Use it for bounded proportions or rates when you want both support constraints and adjustable skew.

### Examples

```cpp
pluss::distr::Beta beta{0.0, 1.0, 2.0, 3.0};
auto x = pluss::make_series({0.0, 0.4, 0.6, 0.8});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = beta.pdf(x);
auto logdens = beta.logpdf(x);
auto dist = beta.cdf(x);
auto logtail = beta.logcdf(x);
auto cgf = beta.cgf(t);
auto k1 = beta.cumulant(1);
auto k2 = beta.cumulant(2);
auto draws = beta.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = beta.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.000000, 1.728000, 1.152000, 0.384000]
logpdf(x):   [-inf, 0.546965, 0.141500, -0.957113]
cdf(x):      [0.000000, 0.524800, 0.820800, 0.972800]
logcdf(x):   [-inf, -0.644738, -0.197476, -0.027577]
cgf(t):      [-0.098756, 0.000000, 0.101256]
cumulants:   [0.4, 0.04, 0.00228571, -0.00102857]
sample({4}): [0.375967, 0.417967, 0.096186, 0.169165]
sample_like: [0.375967, 0.417967, 0.096186, 0.169165] dtype=11
```

## `Skellam`

### Signature

```cpp
pluss::distr::Skellam skellam{lambda1, lambda2};

skellam.sample(sizes, cfg = {}, seed = 0);
skellam.sample_like(like, seed = 0);
skellam.pdf(x);
skellam.logpdf(x);
skellam.cdf(x);
skellam.logcdf(x);
skellam.cgf(t);
skellam.cumulant(order);
```

### Notes

Represents the difference of two independent Poisson counts. Odd cumulants track the rate imbalance and even cumulants track total count variability.

### Examples

```cpp
pluss::distr::Skellam skellam{2.0, 1.0};
auto x = pluss::make_series({-1.0, 0.0, 1.0, 2.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto pmf = skellam.pdf(x);
auto logprob = skellam.logpdf(x);
auto dist = skellam.cdf(x);
auto logtail = skellam.logcdf(x);
auto cgf = skellam.cgf(t);
auto k1 = skellam.cumulant(1);
auto k2 = skellam.cumulant(2);
auto draws = skellam.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = skellam.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.119232, 0.211712, 0.238463, 0.184961]
logpdf(x):   [-2.126686, -1.552528, -1.433539, -1.687612]
cdf(x):      [0.182585, 0.394297, 0.632760, 0.817721]
logcdf(x):   [-1.700541, -0.930651, -0.457664, -0.201234]
cgf(t):      [-0.158373, 0.000000, 0.346852]
cumulants:   [1, 3, 1, 3]
sample({4}): [5.000000, 3.000000, 4.000000, 0.000000]
sample_like: [5.000000, 3.000000, 4.000000, 0.000000] dtype=11
```

## `Nig`

### Signature

```cpp
pluss::distr::Nig nig{alpha, beta, location, scale};

nig.sample(sizes, cfg = {}, seed = 0);
nig.sample_like(like, seed = 0);
nig.pdf(x);
nig.logpdf(x);
nig.cdf(x);
nig.logcdf(x);
nig.cgf(t);
nig.cumulant(order);
```

### Notes

Normal-inverse-Gaussian wrapper for skewed heavy-tailed location-scale modeling. `cumulant(order)` currently supports exact closed forms up to order 4.

### Examples

```cpp
pluss::distr::Nig nig{2.0, 0.5, 0.0, 1.0};
auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = nig.pdf(x);
auto logdens = nig.logpdf(x);
auto dist = nig.cdf(x);
auto logtail = nig.logcdf(x);
auto cgf = nig.cgf(t);
auto k1 = nig.cumulant(1);
auto k2 = nig.cumulant(2);
auto draws = nig.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = nig.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.093492, 0.617447, 0.522367, 0.254138]
logpdf(x):   [-2.369877, -0.482163, -0.649385, -1.369877]
cdf(x):      [0.033036, 0.367565, 0.671088, 0.862480]
logcdf(x):   [-3.410163, -1.000856, -0.398855, -0.147944]
cgf(t):      [-0.047822, 0.000000, 0.082442]
cumulants:   [0.258199, 0.550824, 0.22033, 0.587546]
sample({4}): [0.276759, 1.896829, 1.379230, -0.724068]
sample_like: [0.276759, 1.896829, 1.379230, -0.724068] dtype=11
```

## `Gh`

### Signature

```cpp
pluss::distr::Gh gh{lambda, alpha, beta, location, scale};

gh.sample(sizes, cfg = {}, seed = 0);
gh.sample_like(like, seed = 0);
gh.pdf(x);
gh.logpdf(x);
gh.cdf(x);
gh.logcdf(x);
gh.cgf(t);
gh.cumulant(order);
```

### Notes

Generalized hyperbolic wrapper for asymmetric heavy-tailed modeling. It generalizes `Nig` by exposing the extra shape parameter `lambda` while retaining the same object-oriented method surface.

### Examples

```cpp
pluss::distr::Gh gh{1.0, 2.0, 0.5, 0.0, 1.0};
auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = gh.pdf(x);
auto logdens = gh.logpdf(x);
auto dist = gh.cdf(x);
auto logtail = gh.logcdf(x);
auto cgf = gh.cgf(t);
auto k1 = gh.cumulant(1);
auto k2 = gh.cumulant(2);
auto draws = gh.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = gh.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.114108, 0.430768, 0.436811, 0.310177]
logpdf(x):   [-2.170613, -0.842186, -0.828254, -1.170613]
cdf(x):      [0.054111, 0.322720, 0.547947, 0.736934]
logcdf(x):   [-2.916726, -1.130971, -0.601576, -0.305257]
cgf(t):      [-0.087597, 0.000000, 0.153511]
cumulants:   [0.475739, 1.04555, 0.610504, 1.71741]
sample({4}): [2.037122, -0.171804, -0.721009, 2.011074]
sample_like: [2.037122, -0.171804, -0.721009, 2.011074] dtype=11
```

## `GaussianMixture`

### Signature

```cpp
pluss::distr::GaussianMixture gm{weights, means, stddevs};

gm.sample(sizes, cfg = {}, seed = 0, use_sobol = false);
gm.sample_like(like, seed = 0, use_sobol = false);
gm.pdf(x);
gm.logpdf(x);
gm.cdf(x);
gm.logcdf(x);
gm.cgf(t);
gm.cumulant(order);
```

### Notes

Wraps a Gaussian mixture with component weights, means, and standard deviations stored as `Series`. The wrapper validates component-shape compatibility and supports `use_sobol=true` for Sobol-driven sampling.

### Examples

```cpp
auto weights = pluss::make_series({0.7, 0.3});
auto means = pluss::make_series({-1.0, 2.0});
auto stddevs = pluss::make_series({0.5, 1.25});
pluss::distr::GaussianMixture gm{weights, means, stddevs};
auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
auto like = pluss::series::zeros({4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));
auto t = pluss::make_series({-0.25, 0.0, 0.25});

auto dens = gm.pdf(x);
auto logdens = gm.logpdf(x);
auto dist = gm.cdf(x);
auto logtail = gm.logcdf(x);
auto cgf = gm.cgf(t);
auto k1 = gm.cumulant(1);
auto k2 = gm.cumulant(2);
auto draws = gm.sample({4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20260303ULL);
auto like_draws = gm.sample_like(like, 20260303ULL);
```

Output

```text
pdf(x):      [0.563894, 0.102208, 0.052809, 0.069713]
logpdf(x):   [-0.572889, -2.280742, -2.941069, -2.663364]
cdf(x):      [0.352459, 0.700515, 0.733576, 0.763534]
logcdf(x):   [-1.042820, -0.355940, -0.309824, -0.269797]
cgf(t):      [0.092517, 0.000000, 0.066540]
cumulants:   [-0.1, 2.53375, 4.74862, 2.61617]
sample({4}): [0.853880, 2.383822, -0.691794, 1.097873]
sample_like: [0.853880, 2.383822, -0.691794, 1.097873] dtype=11
```
