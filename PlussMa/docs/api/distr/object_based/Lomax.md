<!-- generated from Lomax.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Lomax` object

The object form of the Lomax (Pareto type II) distribution: a power-law distribution that starts at zero, with exact cumulants wherever they exist and a maximum-likelihood fit.

## Signature

```cpp
distr::Lomax(double scale, double alpha);
pluss::Series distr::Lomax::pdf(const pluss::Series& x) const;
pluss::Series distr::Lomax::logpdf(const pluss::Series& x) const;
pluss::Series distr::Lomax::cdf(const pluss::Series& x) const;
pluss::Series distr::Lomax::logcdf(const pluss::Series& x) const;
pluss::Series distr::Lomax::sample(std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0) const;
pluss::Series distr::Lomax::sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
pluss::Series distr::Lomax::cgf(const pluss::Series& t) const;
pluss::Series distr::Lomax::cumulant(std::int64_t order) const;
distr::LomaxFit distr::Lomax::MLE(const pluss::Series& x, std::vector<double> start = {}) const;
```

## What It Does

`distr::Lomax` holds the parameters of the [Lomax](../continuous_positive/Lomax.md) family under their own names, and its methods give the same densities, distribution values and draws as the kind-based functions. What it adds for this heavy-tailed family is exact moments: the `n`-th cumulant exists only when `alpha > n`, and below that index the call throws an error that names the condition, instead of returning a number that depends on where an integral was cut off. The first cumulant is `scale / (alpha - 1)`.

For `t < 0` the integral of `exp(t x)` against the density converges and is computed numerically; for `t > 0` it diverges, and the call is refused. The fit maximizes the log-likelihood numerically from a starting point.

The object has no `Series`-parameter form: the kind-based functions of this family accept a `Series` parameter only as a single value.

## Parameters

`double scale`
: the scale of the axis.

`double alpha`
: the tail index: the probability of exceeding `x` decays like `x^(-alpha)`.

`const pluss::Series& x`
: The points at which to evaluate, or the observed values to fit.

`const pluss::Series& t`
: The points at which to evaluate the cumulant generating function; it exists for `t <= 0`.

`std::int64_t order`
: The order of the cumulant, 1 or more; it must be below the tail index.

`std::initializer_list<std::int64_t> sizes`, `pluss::Config cfg`, `std::uint64_t seed`
: The shape and dtype of the draws and the seed of the stream; the same seed always gives the same draws.

## Returns

`pdf`, `logpdf`, `cdf`, `logcdf` and `cgf` return a `Series` of the shape of their argument, `sample` and `sample_like` the drawn `Series`, and `cumulant` a one-element `Series`. `MLE` returns a `LomaxFit` with the fields `scale`, `alpha`, `converged` and `iterations`, and the methods `model()`, `loglik(x)`, `vcov(x)`, `stderr(x)` and `confint(x, level)`.

## Errors

- A parameter the family rejects is refused when a method is called.
- `cumulant` throws when the moment of that order does not exist.
- `cgf` throws for `t > 0`.
- `MLE` throws when the observations are outside the support.

## Methods

- `pdf` -- the density (mass function) at each point.
- `logpdf` -- the logarithm of the density.
- `cdf` -- the distribution function.
- `logcdf` -- the logarithm of the distribution function.
- `sample` -- random draws of a given shape.
- `sample_like` -- random draws with the shape and dtype of an existing Series.
- `cgf` -- the cumulant generating function for `t <= 0`.
- `cumulant` -- the exact cumulant of a given order, below the tail index.
- `MLE` -- the maximum-likelihood fit.

## Practical Examples

### Example 1

Example 1 evaluates the density, the distribution function and the log-density of the object, and compares the exact first two cumulants with the mean and variance of a large sample.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace distr = pluss::subsp::stats::distr;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  std::cout << std::fixed << std::setprecision(5);

  const distr::Lomax d{2.0, 3.5};
  const auto x = pluss::make_series({1.6, 2.0, 3.0, 5.0, 8.0, 13.0}, f64);

  std::cout << "pdf:";
  for (double v : d.pdf(x).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : d.cdf(x).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : d.logpdf(x).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';

  const auto draws = d.sample({400000}, f64, 5ULL);
  const double m = as_f64(draws.mean());
  const double v = as_f64(((draws - m) * (draws - m)).mean());
  std::cout << "cumulant(1), the mean: " << as_f64(d.cumulant(1)) << "   mean of 400000 draws: " << m << '\n';
  std::cout << "cumulant(2), the variance: " << as_f64(d.cumulant(2)) << "   variance of the draws: " << v << '\n';
}
```

Output

```text
pdf: 0.12425 0.07734 0.02833 0.00623 0.00125 0.00020
cdf: 0.87220 0.91161 0.95952 0.98753 0.99642 0.99913
logpdf: -2.08542 -2.55955 -3.56369 -5.07782 -6.68285 -8.50745
cumulant(1), the mean: 0.80000   mean of 400000 draws: 0.79936
cumulant(2), the variance: 1.49333   variance of the draws: 1.49769
```

### Example 2

Example 2 shows the exact moments and where they stop: at alpha = 2 the first cumulant exists but the variance does not, and the object says so. The cumulant generating function is then evaluated at negative `t`, where it exists, and at 0, where it is 0.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <string>

namespace distr = pluss::subsp::stats::distr;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const distr::Lomax d{2.0, 2.0};

  std::cout << "cumulant(1): " << as_f64(d.cumulant(1)) << '\n';
  try {
    (void)d.cumulant(2);
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "cumulant(2): " << message.substr(0, message.find('\n')) << '\n';
  }

  const auto t = pluss::make_series({-1.0, -0.5, 0.0}, f64);
  std::cout << "cgf at -1, -0.5, 0:";
  for (double v : distr::Lomax{2.0, 3.5}.cgf(t).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  try {
    (void)d.cgf(pluss::make_series(0.5, f64));
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "cgf at 0.5: " << message.substr(0, message.find('\n')) << '\n';
  }
}
```

Output

```text
cumulant(1): 2
cumulant(2): Lomax::cumulant: the moment of order 2 does not exist; it needs alpha > the order
cgf at -1, -0.5, 0: -0.518045 -0.304446 0
cgf at 0.5: Lomax::cgf: the moment generating function does not exist for t > 0
```

### Example 3

Example 3 fits the family back to 20000 draws by maximum likelihood, and reads the estimates, their standard errors and the 95% intervals.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace distr = pluss::subsp::stats::distr;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  std::cout << std::fixed << std::setprecision(4);

  const auto x = distr::Lomax{2.0, 3.0}.sample({20000}, f64, 3ULL);
  const auto fit = distr::Lomax{2.0, 3.0}.MLE(x, {1.0, 2.0});

  const auto se = fit.stderr(x).pull<double>();
  const auto ci = fit.confint(x, 0.95).host<double>().array().data;
  std::cout << "scale: " << fit.scale << "  stderr " << se[0] << "  95% [" << ci[0] << ", " << ci[1] << "]\n";
  std::cout << "alpha: " << fit.alpha << "  stderr " << se[1] << "  95% [" << ci[2] << ", " << ci[3] << "]\n";
  std::cout << "loglik: " << as_f64(fit.loglik(x)) << '\n';
}
```

Output

```text
scale: 2.0665  stderr 0.0763  95% [1.9169, 2.2161]
alpha: 3.0446  stderr 0.0873  95% [2.8734, 3.2158]
loglik: -18818.6221
```

## Changelog

- **0.1** -- the object form with exact cumulants below the tail index, the cumulant generating function for `t <= 0` and a maximum-likelihood fit.

## See Also

- [`distr::Lomax`](../continuous_positive/Lomax.md) -- the kind-based functions and the parameters
- [`distr::Distribution`](Distribution.md) -- the object that serves every family, with numerical moments

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
