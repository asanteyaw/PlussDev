<!-- generated from Poisson.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Poisson` object

The object form of the Poisson family: it carries the parameters, and offers the density, the distribution function, random draws, the cumulant generating function and the cumulants as methods, and a maximum-likelihood fit with standard errors.

## Signature

```cpp
distr::Poisson(double lambda);
pluss::Series distr::Poisson::pdf(const pluss::Series& x) const;
pluss::Series distr::Poisson::logpdf(const pluss::Series& x) const;
pluss::Series distr::Poisson::cdf(const pluss::Series& x) const;
pluss::Series distr::Poisson::logcdf(const pluss::Series& x) const;
pluss::Series distr::Poisson::sample(std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0) const;
pluss::Series distr::Poisson::sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
pluss::Series distr::Poisson::cgf(const pluss::Series& t) const;
pluss::Series distr::Poisson::cumulant(std::int64_t order) const;
distr::PoissonFit distr::Poisson::MLE(const pluss::Series& x) const;
```

## What It Does

`distr::Poisson` holds the parameters of the [Poisson](../discrete/Poisson.md) distribution, so that they are written once and the same object is used for every evaluation. Its methods give the same numbers as the kind-based functions `distr::pdf(dst::Poisson, ...)` and the others, on a whole `Series` at once.

- `pdf`, `logpdf`, `cdf` and `logcdf` evaluate the mass function, the distribution function and their logarithms.
- `sample` draws a `Series` of the given shape; `sample_like` draws one with the shape and dtype of an existing `Series`. The seed fixes the stream.
- `cgf(t)` is the cumulant generating function `log E[exp(t X)]`, and `cumulant(order)` the cumulant of that order (the first is the mean, the second the variance).
- `MLE(x)` fits the family to observed values by maximum likelihood and returns a `PoissonFit` with the estimates (fields `lambda`), their standard errors, 95% intervals and the log-likelihood.

The parameters of the family are described on the [Poisson](../discrete/Poisson.md) page.

## Parameters

`double lambda`
: `lambda` is the average number of events.

`const pluss::Series& x`
: The points at which to evaluate, a `Series` of any shape; the result has the same shape.

`std::initializer_list<std::int64_t> sizes`
: The shape of the array of random draws, for example `{n}` or `{rows, columns}`.

`pluss::Config cfg`
: The dtype and platform of the draws; the default is Float32 on the CPU, so request Float64 when precision matters.

`std::uint64_t seed`
: The seed of the stream. The same seed always gives the same draws; a seed of 0 is a fixed default, not a random one.

`const pluss::Series& like`
: A Series whose shape and dtype `sample_like` copies for the draws; its values are not used.

`const pluss::Series& t`
: The points at which to evaluate the cumulant generating function.

`std::int64_t order`
: The order of the cumulant: 1 is the mean, 2 the variance, 3 the third cumulant, and so on.

`const pluss::Series& x` (for `MLE`)
: The observed values to fit, one-dimensional and inside the support of the family.

## Returns

`pdf`, `logpdf`, `cdf`, `logcdf` and `cgf` return a `Series` of the shape of their argument; `sample` and `sample_like` return the drawn `Series`; `cumulant` returns a one-element `Series`. `MLE` returns a `PoissonFit`.

## Errors

- Parameters outside their valid range are refused when a method is called (`lambda` must be positive).

## Methods

- `pdf` -- the density (mass function) at each point.
- `logpdf` -- the logarithm of the density.
- `cdf` -- the distribution function.
- `logcdf` -- the logarithm of the distribution function.
- `sample` -- random draws of a given shape.
- `sample_like` -- random draws with the shape and dtype of an existing Series.
- `cgf` -- the cumulant generating function.
- `cumulant` -- the cumulant of a given order.
- `MLE` -- the maximum-likelihood fit with standard errors and intervals.

## Practical Examples

### Example 1

Example 1 uses the object form, `distr::Poisson`, which carries the parameters and offers the same functions as methods, plus `cumulant` for the moments.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  std::cout << std::fixed << std::setprecision(5);

  const distr::Poisson d{3.2};
  const auto sample = d.sample({200000}, f64, 11ULL);
  const auto at = pluss::make_series({3.0}, f64);

  std::cout << "pdf at 3: " << as_f64(d.pdf(at)) << '\n';
  std::cout << "cdf at 3: " << as_f64(d.cdf(at)) << '\n';
  const double m = as_f64(sample.mean());
  const double v = as_f64(((sample - m) * (sample - m)).mean());
  std::cout << "cumulant(1), the mean: " << as_f64(d.cumulant(1)) << "   mean of 200000 draws: " << m << '\n';
  std::cout << "cumulant(2), the variance: " << as_f64(d.cumulant(2)) << "   variance of the draws: " << v << '\n';
}
```

Output

```text
pdf at 3: 0.22262
cdf at 3: 0.60252
cumulant(1), the mean: 3.20000   mean of 200000 draws: 3.19890
cumulant(2), the variance: 3.20000   variance of the draws: 3.16997
```

`cumulant(1)` and `cumulant(2)` are the exact mean (3.2) and variance (3.2) of the distribution, computed from the parameters without drawing anything; the mean (3.1989) and variance (3.16997) of 200000 draws land within sampling error of them (the standard error of the mean is about 0.004).

### Example 2

Example 2 uses every evaluation method of the object `distr::Poisson`: the density (mass function), its logarithm, the distribution function and its logarithm, the cumulant generating function `cgf`, and the cumulants of orders 1 to 4.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  std::cout << std::fixed << std::setprecision(5);

  const distr::Poisson d{3.2};
  const auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0}, f64);
  const auto t = pluss::make_series({0.1, 0.2, 0.3}, f64);

  std::cout << "x:";
  for (double v : x.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pmf:";
  for (double v : d.pdf(x).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpmf:";
  for (double v : d.logpdf(x).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : d.cdf(x).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logcdf:";
  for (double v : d.logcdf(x).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cgf at t = 0.1 0.2 0.3:";
  for (double v : d.cgf(t).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';

  std::cout << "cumulants 1 to 4:";
  for (int order = 1; order <= 4; ++order) std::cout << ' ' << as_f64(d.cumulant(order));
  std::cout << '\n';
}
```

Output

```text
x: 0.00000 1.00000 2.00000 3.00000
pmf: 0.04076 0.13044 0.20870 0.22262
logpmf: -3.20000 -2.03685 -1.56685 -1.50231
cdf: 0.04076 0.17120 0.37990 0.60252
logcdf: -3.20000 -1.76492 -0.96784 -0.50663
cgf at t = 0.1 0.2 0.3: 0.33655 0.70849 1.11955
cumulants 1 to 4: 3.20000 3.20000 3.20000 3.20000
```

`logpdf` and `logcdf` are the logarithms of the density and of the distribution function, computed directly, which is more accurate in the tails than taking the logarithm of a tiny number: at the first point exp(-3.2) = 0.04076, the density 0.04076 printed above. The cumulants are 3.2, 3.2, 3.2, 3.2: the first is the mean, the second the variance, the third measures skewness (it is zero for a symmetric distribution), and the fourth the excess of the tails over a normal one. The cumulants are the Taylor coefficients of `cgf`: 0.1 κ1 + 0.1² κ2 / 2 + 0.1³ κ3 / 6 + 0.1⁴ κ4 / 24 = 0.33655 against `cgf(0.1)` = 0.33655.

### Example 3

Example 3 draws random values from the object: `sample_like` takes the shape and dtype of an existing Series, `sample` takes a shape, and the seed fixes the stream; a large sample reproduces the first two cumulants.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  std::cout << std::fixed << std::setprecision(5);

  const distr::Poisson d{3.2};

  // sample_like draws a Series with the shape and dtype of an existing one; the seed fixes the stream.
  const auto like = pluss::series::zeros({2, 3}, f64);
  const auto draws = d.sample_like(like, 11ULL);
  std::cout << "shape of the draws: " << draws.sizes()[0] << " x " << draws.sizes()[1] << "\n";

  const auto again = d.sample_like(like, 11ULL);
  std::cout << "same seed, same draws: " << (as_f64((draws - again).abs().max()) == 0.0 ? "yes" : "no") << "\n";
  const auto other = d.sample_like(like, 12ULL);
  std::cout << "other seed, other draws: " << (as_f64((draws - other).abs().max()) > 0.0 ? "yes" : "no") << "\n";

  // sample takes the shape directly; a large sample estimates the mean and the variance.
  const auto big = d.sample({200000}, f64, 5ULL);
  const double m = as_f64(big.mean());
  const double v = as_f64(((big - m) * (big - m)).mean());
  std::cout << "mean of 200000 draws " << m << " (cumulant(1) = " << as_f64(d.cumulant(1)) << ")\n";
  std::cout << "variance of 200000 draws " << v << " (cumulant(2) = " << as_f64(d.cumulant(2)) << ")\n";
}
```

Output

```text
shape of the draws: 2 x 3
same seed, same draws: yes
other seed, other draws: yes
mean of 200000 draws 3.20066 (cumulant(1) = 3.20000)
variance of 200000 draws 3.20877 (cumulant(2) = 3.20000)
```

### Example 4

Example 4 fits the family back to 1000 draws by maximum likelihood with `MLE`, and reads the estimates, their standard errors, the 95% intervals and the log-likelihood.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  std::cout << std::fixed << std::setprecision(4);

  const auto x = distr::Poisson{3.2}.sample({1000}, f64, 20261022ULL);
  const auto fit = distr::Poisson{}.MLE(x);

  std::cout << "lambda: " << fit.lambda << '\n';
  std::cout << "stderr:";
  for (double v : fit.std_err(x).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "95% bounds:";
  for (double v : fit.confint(x).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "loglik: " << as_f64(fit.loglik(x)) << '\n';
}
```

Output

```text
lambda: 3.1970
stderr: 0.0565
95% bounds: 3.0862 3.3078
loglik: -1972.5077
```

The fit recovers the generating values: `lambda` 3.197 against a true 3.2 (0.05 standard errors away). The 95% bounds are the estimate ± 1.96 standard errors, one pair per parameter in the order of the fit's fields, and the log-likelihood is the quantity to compare against other families fitted to the same data.

## Changelog

- **0.1** -- the object form with its evaluation, sampling and cumulant methods.

## See Also

- [`distr::Poisson`](../discrete/Poisson.md) -- the kind-based functions and the parameters
- [`distr::Binomial`](Binomial.md)
- [`distr::NegativeBinomial`](NegativeBinomial.md)
- [`distr::Geometric`](Geometric.md)
- [`distr::Skellam`](Skellam.md)

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Pareto.html">
    <span class="label">Previous</span>
    <span class="title">Pareto</span>
  </a>
  <a class="next-link" href="Rayleigh.html">
    <span class="label">Next</span>
    <span class="title">Rayleigh</span>
  </a>
</div>
<!-- apiref:nav:end -->
