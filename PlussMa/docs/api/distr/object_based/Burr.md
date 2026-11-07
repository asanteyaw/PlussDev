<!-- generated from Burr.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Burr` object

The object form of the Burr (type III) distribution: a flexible heavy-tailed distribution with two shape parameters, with exact cumulants wherever they exist and a maximum-likelihood fit.

## Signature

```cpp
distr::Burr(double c, double k);
pluss::Series distr::Burr::pdf(const pluss::Series& x) const;
pluss::Series distr::Burr::logpdf(const pluss::Series& x) const;
pluss::Series distr::Burr::cdf(const pluss::Series& x) const;
pluss::Series distr::Burr::logcdf(const pluss::Series& x) const;
pluss::Series distr::Burr::sample(std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0) const;
pluss::Series distr::Burr::sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
pluss::Series distr::Burr::cgf(const pluss::Series& t) const;
pluss::Series distr::Burr::cumulant(std::int64_t order) const;
distr::BurrFit distr::Burr::MLE(const pluss::Series& x, std::vector<double> start = {}) const;
```

## What It Does

`distr::Burr` holds the parameters of the [Burr](../continuous_positive/Burr.md) family under their own names, and its methods give the same densities, distribution values and draws as the kind-based functions. What it adds for this heavy-tailed family is exact moments: the `n`-th cumulant exists only when `c > n`, and below that index the call throws an error that names the condition, instead of returning a number that depends on where an integral was cut off. The first cumulant is `k Gamma(k + 1/c) Gamma(1 - 1/c) / Gamma(k + 1)`.

For `t < 0` the integral of `exp(t x)` against the density converges and is computed numerically; for `t > 0` it diverges, and the call is refused. The fit maximizes the log-likelihood numerically from a starting point.

The object has no `Series`-parameter form: the kind-based functions of this family accept a `Series` parameter only as a single value.

## Parameters

`double c`
: the tail shape: the right tail of the density decays like `x^(-c - 1)`.

`double k`
: the shape at the left end: the density near zero is proportional to `x^(c k - 1)`.

`const pluss::Series& x`
: The points at which to evaluate, or the observed values to fit.

`const pluss::Series& t`
: The points at which to evaluate the cumulant generating function; it exists for `t <= 0`.

`std::int64_t order`
: The order of the cumulant, 1 or more; it must be below the tail index.

`std::initializer_list<std::int64_t> sizes`, `pluss::Config cfg`, `std::uint64_t seed`
: The shape and dtype of the draws and the seed of the stream; the same seed always gives the same draws.

## Returns

`pdf`, `logpdf`, `cdf`, `logcdf` and `cgf` return a `Series` of the shape of their argument, `sample` and `sample_like` the drawn `Series`, and `cumulant` a one-element `Series`. `MLE` returns a `BurrFit` with the fields `c`, `k`, `converged` and `iterations`, and the methods `model()`, `loglik(x)`, `vcov(x)`, `std_err(x)` and `confint(x, level)`.

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

  const distr::Burr d{5.0, 2.0};
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
pdf: 0.45352 0.14247 0.01355 0.00064 0.00004 0.00000
cdf: 0.83345 0.94031 0.99182 0.99936 0.99994 0.99999
logpdf: -0.79071 -1.94861 -4.30141 -7.35500 -10.17416 -13.08712
cumulant(1), the mean: 1.28275   mean of 400000 draws: 1.28290
cumulant(2), the variance: 0.20438   variance of the draws: 0.20430
```

### Example 2

Example 2 shows the exact moments and where they stop: at c = 2 the first cumulant exists but the variance does not, and the object says so. The cumulant generating function is then evaluated at negative `t`, where it exists, and at 0, where it is 0.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <string>

namespace distr = pluss::subsp::stats::distr;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const distr::Burr d{2.0, 3.0};

  std::cout << "cumulant(1): " << as_f64(d.cumulant(1)) << '\n';
  try {
    (void)d.cumulant(2);
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "cumulant(2): " << message.substr(0, message.find('\n')) << '\n';
  }

  const auto t = pluss::make_series({-1.0, -0.5, 0.0}, f64);
  std::cout << "cgf at -1, -0.5, 0:";
  for (double v : distr::Burr{5.0, 2.0}.cgf(t).pull<double>()) std::cout << ' ' << v;
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
cumulant(1): 2.94524
cumulant(2): Burr::cumulant: the moment of order 2 does not exist; it needs c > the order
cgf at -1, -0.5, 0: -1.20574 -0.61976 0
cgf at 0.5: Burr::cgf: the moment generating function does not exist for t > 0
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

  const auto x = distr::Burr{4.0, 2.0}.sample({20000}, f64, 3ULL);
  const auto fit = distr::Burr{4.0, 2.0}.MLE(x, {2.0, 1.0});

  const auto se = fit.std_err(x).pull<double>();
  const auto ci = fit.confint(x, 0.95).host<double>().array().data;
  std::cout << "c: " << fit.c << "  stderr " << se[0] << "  95% [" << ci[0] << ", " << ci[1] << "]\n";
  std::cout << "k: " << fit.k << "  stderr " << se[1] << "  95% [" << ci[2] << ", " << ci[3] << "]\n";
  std::cout << "loglik: " << as_f64(fit.loglik(x)) << '\n';
}
```

Output

```text
c: 4.0365  stderr 0.0223  95% [3.9929, 4.0802]
k: 1.9900  stderr 0.0141  95% [1.9624, 2.0176]
loglik: -13184.1026
```

## Changelog

- **0.1** -- the object form with exact cumulants below the tail index, the cumulant generating function for `t <= 0` and a maximum-likelihood fit.

## See Also

- [`distr::Burr`](../continuous_positive/Burr.md) -- the kind-based functions and the parameters
- [`distr::Distribution`](Distribution.md) -- the object that serves every family, with numerical moments

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
