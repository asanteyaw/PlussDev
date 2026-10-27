<!-- generated from Weibull.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Weibull` object

The object form of the Weibull family: it carries the parameters, and offers the density, the distribution function, random draws, the cumulant generating function and the cumulants as methods.

## Signature

```cpp
distr::Weibull(double shape, double scale);
distr::Weibull::Weibull(<one Series for each parameter>);   // a different distribution at every point
pluss::Series distr::Weibull::pdf(const pluss::Series& x) const;
pluss::Series distr::Weibull::logpdf(const pluss::Series& x) const;
pluss::Series distr::Weibull::cdf(const pluss::Series& x) const;
pluss::Series distr::Weibull::logcdf(const pluss::Series& x) const;
pluss::Series distr::Weibull::sample(std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0) const;
pluss::Series distr::Weibull::sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
pluss::Series distr::Weibull::cgf(const pluss::Series& t) const;
pluss::Series distr::Weibull::cumulant(std::int64_t order) const;
```

## What It Does

`distr::Weibull` holds the parameters of the [Weibull](../continuous_positive/Weibull.md) distribution, so that they are written once and the same object is used for every evaluation. Its methods give the same numbers as the kind-based functions `distr::pdf(dst::Weibull, ...)` and the others, on a whole `Series` at once.

- `pdf`, `logpdf`, `cdf` and `logcdf` evaluate the density, the distribution function and their logarithms.
- `sample` draws a `Series` of the given shape; `sample_like` draws one with the shape and dtype of an existing `Series`. The seed fixes the stream.
- `cgf(t)` is the cumulant generating function `log E[exp(t X)]`, and `cumulant(order)` the cumulant of that order (the first is the mean, the second the variance).
- A constructor taking a `Series` for each parameter makes an object that stands for a different distribution at every point: the density at the i-th point uses the i-th value of every parameter.

The parameters of the family are described on the [Weibull](../continuous_positive/Weibull.md) page.

## Parameters

`double shape, double scale`
: `shape` and `scale` are the parameters of the family.

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

`<one Series for each parameter>`
: Parameters given as Series (all of one length, or one element each to broadcast) make an object that is a different distribution at every point of `x`.

## Returns

`pdf`, `logpdf`, `cdf`, `logcdf` and `cgf` return a `Series` of the shape of their argument; `sample` and `sample_like` return the drawn `Series`; `cumulant` returns a one-element `Series`.

## Errors

- Parameters outside their valid range are refused when a method is called (`shape` and `scale` must be positive).

## Methods

- `pdf` -- the density (mass function) at each point.
- `logpdf` -- the logarithm of the density.
- `cdf` -- the distribution function.
- `logcdf` -- the logarithm of the distribution function.
- `sample` -- random draws of a given shape.
- `sample_like` -- random draws with the shape and dtype of an existing Series.
- `cgf` -- the cumulant generating function.
- `cumulant` -- the cumulant of a given order.

## Practical Examples

### Example 1

Example 1 uses the object form, `distr::Weibull`, which carries the parameters and offers the same functions as methods, plus `cumulant` for the moments.

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

  const distr::Weibull d{1.7, 2.1};
  const auto sample = d.sample({200000}, f64, 11ULL);
  const auto at = pluss::make_series({1.0}, f64);

  std::cout << "pdf at 1: " << as_f64(d.pdf(at)) << '\n';
  std::cout << "cdf at 1: " << as_f64(d.cdf(at)) << '\n';
  const double m = as_f64(sample.mean());
  const double v = as_f64(((sample - m) * (sample - m)).mean());
  std::cout << "cumulant(1), the mean: " << as_f64(d.cumulant(1)) << "   mean of 200000 draws: " << m << '\n';
  std::cout << "cumulant(2), the variance: " << as_f64(d.cumulant(2)) << "   variance of the draws: " << v << '\n';
}
```

Output

```text
pdf at 1: 0.36278
cdf at 1: 0.24670
cumulant(1), the mean: 1.87371   mean of 200000 draws: 1.87702
cumulant(2), the variance: 1.28708   variance of the draws: 1.28789
```

`cumulant(1)` and `cumulant(2)` are the exact mean (1.87371) and variance (1.28708) of the distribution, computed from the parameters without drawing anything; the mean (1.87702) and variance (1.28789) of 200000 draws land within sampling error of them (the standard error of the mean is about 0.00254).

### Example 2

Example 2 uses every evaluation method of the object `distr::Weibull`: the density (mass function), its logarithm, the distribution function and its logarithm, the cumulant generating function `cgf`, and the cumulants of orders 1 to 4.

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

  const distr::Weibull d{1.7, 2.1};
  const auto x = pluss::make_series({0.25, 0.5, 1.0, 2.0}, f64);
  const auto t = pluss::make_series({0.1, 0.2, 0.3}, f64);

  std::cout << "x:";
  for (double v : x.pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : d.pdf(x).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : d.logpdf(x).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : d.cdf(x).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logcdf:";
  for (double v : d.logcdf(x).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cgf at t = 0.1 0.2 0.3:";
  for (double v : d.cgf(t).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';

  std::cout << "cumulants 1 to 4:";
  for (int order = 1; order <= 4; ++order) std::cout << ' ' << as_f64(d.cumulant(order));
  std::cout << '\n';
}
```

Output

```text
x: 0.25000 0.50000 1.00000 2.00000
pdf: 0.17766 0.27170 0.36278 0.31165
logpdf: -1.72791 -1.30306 -1.01395 -1.16587
cdf: 0.02648 0.08350 0.24670 0.60164
logcdf: -3.63138 -2.48292 -1.39960 -0.50809
cgf at t = 0.1 0.2 0.3: 0.19402 0.40226 0.62616
cumulants 1 to 4: 1.87371 1.28708 1.26310 1.27950
```

`logpdf` and `logcdf` are the logarithms of the density and of the distribution function, computed directly, which is more accurate in the tails than taking the logarithm of a tiny number: at the first point exp(-1.72791) = 0.17766, the density 0.17766 printed above. The cumulants are 1.87371, 1.28708, 1.2631, 1.2795: the first is the mean, the second the variance, the third measures skewness (it is zero for a symmetric distribution), and the fourth the excess of the tails over a normal one. The cumulants are the Taylor coefficients of `cgf`: 0.1 κ1 + 0.1² κ2 / 2 + 0.1³ κ3 / 6 + 0.1⁴ κ4 / 24 = 0.19402 against `cgf(0.1)` = 0.19402.

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

  const distr::Weibull d{1.7, 2.1};

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
mean of 200000 draws 1.87162 (cumulant(1) = 1.87371)
variance of 200000 draws 1.29425 (cumulant(2) = 1.28708)
```

### Example 4

Example 4 gives the object a Series for each parameter. One object then stands for a different distribution at every point of `x`, and its density agrees with scalar objects built one point at a time.

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

  // A Series for every parameter makes ONE object that is a different distribution at each point:
  // the density at the i-th point uses the i-th value of every parameter.
  const auto p0 = pluss::make_series({1.7, 2.125, 2.55}, f64);
  const auto p1 = pluss::make_series({2.1, 2.625, 3.1500000000000004}, f64);
  const distr::Weibull many{p0, p1};
  const auto x = pluss::make_series({0.25, 0.5, 1.0}, f64);
  const auto density = many.pdf(x);
  std::cout << "one object, three parameter sets:";
  for (double v : density.pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';

  // The same numbers, one scalar object at a time.
  {
    {
      const distr::Weibull one{1.7, 2.1};
      const auto at = pluss::make_series({0.25}, f64);
      std::cout << "point 0: " << as_f64(density.select(0, 0)) << " and one object at a time " << as_f64(one.pdf(at)) << "\n";
    }
    {
      const distr::Weibull one{2.125, 2.625};
      const auto at = pluss::make_series({0.5}, f64);
      std::cout << "point 1: " << as_f64(density.select(0, 1)) << " and one object at a time " << as_f64(one.pdf(at)) << "\n";
    }
    {
      const distr::Weibull one{2.55, 3.1500000000000004};
      const auto at = pluss::make_series({1.0}, f64);
      std::cout << "point 2: " << as_f64(density.select(0, 2)) << " and one object at a time " << as_f64(one.pdf(at)) << "\n";
    }
  }
}
```

Output

```text
one object, three parameter sets: 0.17766 0.12169 0.12959
point 0: 0.17766 and one object at a time 0.17766
point 1: 0.12169 and one object at a time 0.12169
point 2: 0.12959 and one object at a time 0.12959
```

One object with three values of each parameter returned the three densities 0.17766, 0.12169, 0.12959. Each equals the density of the scalar object built from the i-th values of the parameters and evaluated at the i-th point (the three lines below the first), so a Series-valued object is the same as three scalar objects without the loop.

## Changelog

- **0.1** -- the object form with its evaluation, sampling and cumulant methods.

## See Also

- [`distr::Weibull`](../continuous_positive/Weibull.md) -- the kind-based functions and the parameters
- [`distr::Exponential`](Exponential.md)
- [`distr::Rayleigh`](Rayleigh.md)
- [`distr::Gamma`](Gamma.md)
- [`distr::Lognormal`](Lognormal.md)

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
