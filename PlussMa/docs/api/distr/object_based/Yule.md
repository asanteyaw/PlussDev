<!-- generated from Yule.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Yule` object

The object form of the Yule–Simon distribution: a discrete power law with a heavy tail, with exact cumulants wherever they exist and a maximum-likelihood fit.

## Signature

```cpp
distr::Yule(double p);
pluss::Series distr::Yule::pdf(const pluss::Series& x) const;
pluss::Series distr::Yule::logpdf(const pluss::Series& x) const;
pluss::Series distr::Yule::cdf(const pluss::Series& x) const;
pluss::Series distr::Yule::logcdf(const pluss::Series& x) const;
pluss::Series distr::Yule::sample(std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0) const;
pluss::Series distr::Yule::sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
pluss::Series distr::Yule::cgf(const pluss::Series& t) const;
pluss::Series distr::Yule::cumulant(std::int64_t order) const;
distr::YuleFit distr::Yule::MLE(const pluss::Series& x, std::vector<double> start = {}) const;
```

## What It Does

`distr::Yule` holds the parameters of the [Yule](../discrete/Yule.md) family under their own names, and its methods give the same densities, distribution values and draws as the kind-based functions. What it adds for this heavy-tailed family is exact moments: the `n`-th cumulant exists only when `p > n + 1`, and below that index the call throws an error that names the condition, instead of returning a number that depends on where an integral was cut off. The first cumulant is `(p - 1) / (p - 2)`.

For `t < 0` the series of `exp(t k)` times the masses converges quickly and is summed; for `t > 0` it diverges, and the call is refused. The fit maximizes the log-likelihood numerically from a starting point.

The object has no `Series`-parameter form: the kind-based functions of this family accept a `Series` parameter only as a single value.

## Parameters

`double p`
: the tail exponent: the mass of `k` decays like `k^(-p)`.

`const pluss::Series& x`
: The points at which to evaluate, or the observed values to fit.

`const pluss::Series& t`
: The points at which to evaluate the cumulant generating function; it exists for `t <= 0`.

`std::int64_t order`
: The order of the cumulant, 1 or more; it must be below the tail index.

`std::initializer_list<std::int64_t> sizes`, `pluss::Config cfg`, `std::uint64_t seed`
: The shape and dtype of the draws and the seed of the stream; the same seed always gives the same draws.

## Returns

`pdf`, `logpdf`, `cdf`, `logcdf` and `cgf` return a `Series` of the shape of their argument, `sample` and `sample_like` the drawn `Series`, and `cumulant` a one-element `Series`. `MLE` returns a `YuleFit` with the fields `p`, `converged` and `iterations`, and the methods `model()`, `loglik(x)`, `vcov(x)`, `std_err(x)` and `confint(x, level)`.

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

  const distr::Yule d{5.0};
  const auto x = pluss::make_series({1.0, 2.0, 3.0, 5.0, 8.0, 13.0}, f64);

  std::cout << "pdf:";
  for (double v : d.pdf(x).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : d.cdf(x).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : d.logpdf(x).items<double>()) std::cout << ' ' << v;
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
pdf: 0.80000 0.13333 0.03810 0.00635 0.00101 0.00013
cdf: 0.80000 0.93333 0.97143 0.99206 0.99798 0.99958
logpdf: -0.22314 -2.01490 -3.26767 -5.05943 -6.89770 -8.95351
cumulant(1), the mean: 1.33333   mean of 400000 draws: 1.33230
cumulant(2), the variance: 0.88889   variance of the draws: 0.89176
```

### Example 2

Example 2 shows the exact moments and where they stop: at p = 3 the first cumulant exists but the variance does not, and the object says so. The cumulant generating function is then evaluated at negative `t`, where it exists, and at 0, where it is 0.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <string>

namespace distr = pluss::subsp::stats::distr;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const distr::Yule d{3.0};

  std::cout << "cumulant(1): " << as_f64(d.cumulant(1)) << '\n';
  try {
    (void)d.cumulant(2);
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "cumulant(2): " << message.substr(0, message.find('\n')) << '\n';
  }

  const auto t = pluss::make_series({-1.0, -0.5, 0.0}, f64);
  std::cout << "cgf at -1, -0.5, 0:";
  for (double v : distr::Yule{5.0}.cgf(t).items<double>()) std::cout << ' ' << v;
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
cumulant(2): Yule::cumulant: the moment of order 2 does not exist; it needs p > the order + 1
cgf at -1, -0.5, 0: -1.15658 -0.606107 0
cgf at 0.5: Yule::cgf: the moment generating function does not exist for t > 0
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

  const auto x = distr::Yule{3.0}.sample({20000}, f64, 3ULL);
  const auto fit = distr::Yule{3.0}.MLE(x, {2.0});

  const auto se = fit.std_err(x).items<double>();
  const auto ci = fit.confint(x, 0.95).host<double>().array().data;
  std::cout << "p: " << fit.p << "  stderr " << se[0] << "  95% [" << ci[0] << ", " << ci[1] << "]\n";
  std::cout << "loglik: " << as_f64(fit.loglik(x)) << '\n';
}
```

Output

```text
p: 3.0058  stderr 0.0219  95% [2.9628, 3.0488]
loglik: -24200.9667
```

## Changelog

- **0.1** -- the object form with exact cumulants below the tail index, the cumulant generating function for `t <= 0` and a maximum-likelihood fit.

## See Also

- [`distr::Yule`](../discrete/Yule.md) -- the kind-based functions and the parameters
- [`distr::Distribution`](Distribution.md) -- the object that serves every family, with numerical moments

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Weibull.html">
    <span class="label">Previous</span>
    <span class="title">Weibull</span>
  </a>
  <a class="next-link" href="Zeta.html">
    <span class="label">Next</span>
    <span class="title">Zeta</span>
  </a>
</div>
<!-- apiref:nav:end -->
