<!-- generated from StudentT.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::StudentT` object

The object form of the StudentT family: it carries the parameters, and offers the density, the distribution function, random draws, the cumulant generating function and the cumulants as methods.

## Signature

```cpp
distr::StudentT(double degrees_of_freedom, double location, double scale);
distr::StudentT::StudentT(<one Series for each parameter>);   // a different distribution at every point
pluss::Series distr::StudentT::pdf(const pluss::Series& x) const;
pluss::Series distr::StudentT::logpdf(const pluss::Series& x) const;
pluss::Series distr::StudentT::cdf(const pluss::Series& x) const;
pluss::Series distr::StudentT::logcdf(const pluss::Series& x) const;
pluss::Series distr::StudentT::sample(std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0) const;
pluss::Series distr::StudentT::sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
pluss::Series distr::StudentT::cgf(const pluss::Series& t) const;
pluss::Series distr::StudentT::cumulant(std::int64_t order) const;
```

## What It Does

`distr::StudentT` holds the parameters of the [StudentT](../continuous_real_line/StudentT.md) distribution, so that they are written once and the same object is used for every evaluation. Its methods give the same numbers as the kind-based functions `distr::pdf(dst::StudentT, ...)` and the others, on a whole `Series` at once.

- `pdf`, `logpdf`, `cdf` and `logcdf` evaluate the density, the distribution function and their logarithms.
- `sample` draws a `Series` of the given shape; `sample_like` draws one with the shape and dtype of an existing `Series`. The seed fixes the stream.
- `cgf(t)` is the cumulant generating function `log E[exp(t X)]`, and `cumulant(order)` the cumulant of that order (the first is the mean, the second the variance).
- A constructor taking a `Series` for each parameter makes an object that stands for a different distribution at every point: the density at the i-th point uses the i-th value of every parameter.

The parameters of the family are described on the [StudentT](../continuous_real_line/StudentT.md) page.

## Parameters

`double degrees_of_freedom, double location, double scale`
: `degrees_of_freedom`, `location` and `scale`, in that order.

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

- Parameters outside their valid range are refused when a method is called (`nu` and `scale` must be positive).

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

Example 1 uses the object form, `distr::StudentT`, which carries the parameters and offers the same functions as methods, plus `cumulant` for the moments.

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

  const distr::StudentT d{5.0, 0.0, 1.0};
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
pdf at 1: 0.21968
cdf at 1: 0.81839
cumulant(1), the mean: 0.00000   mean of 200000 draws: -0.00073
cumulant(2), the variance: 1.66667   variance of the draws: 1.65706
```

`cumulant(1)` and `cumulant(2)` are the exact mean (0) and variance (1.66667) of the distribution, computed from the parameters without drawing anything; the mean (-7.30e-04) and variance (1.65706) of 200000 draws land within sampling error of them (the standard error of the mean is about 0.00289).

### Example 2

Example 2 uses every evaluation method of the object `distr::StudentT`: the density (mass function), its logarithm, the distribution function and its logarithm, the cumulant generating function `cgf`, and the cumulants of orders 1 to 4. For this family `cgf` is defined only at t = 0 (the moment generating function does not exist for any other t), so it is evaluated there.

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

  const distr::StudentT d{5.0, 0.0, 1.0};
  const auto x = pluss::make_series({-6.0, -2.0, 0.0, 1.0}, f64);
  const auto t = pluss::make_series({0.0}, f64);

  std::cout << "x:";
  for (double v : x.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : d.pdf(x).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : d.logpdf(x).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : d.cdf(x).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logcdf:";
  for (double v : d.logcdf(x).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cgf at t = 0: " << as_f64(d.cgf(t)) << '\n';

  std::cout << "cumulants 1 to 4:";
  for (int order = 1; order <= 4; ++order) std::cout << ' ' << as_f64(d.cumulant(order));
  std::cout << '\n';
}
```

Output

```text
x: -6.00000 -2.00000 0.00000 1.00000
pdf: 0.00069 0.06509 0.37961 0.21968
logpdf: -7.28102 -2.73198 -0.96862 -1.51558
cdf: 0.00092 0.05097 0.50000 0.81839
logcdf: -6.98781 -2.97652 -0.69315 -0.20041
cgf at t = 0: 0.00000
cumulants 1 to 4: 0.00000 1.66667 0.00000 16.66667
```

`logpdf` and `logcdf` are the logarithms of the density and of the distribution function, computed directly, which is more accurate in the tails than taking the logarithm of a tiny number: at the first point exp(-7.28102) = 6.88e-04, the density 6.90e-04 printed above. The cumulants are 0, 1.66667, 0, 16.66667: the first is the mean, the second the variance, and for this symmetric distribution the third is zero. `cgf` is zero at t = 0 for every distribution, since log E[1] = 0.

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

  const distr::StudentT d{5.0, 0.0, 1.0};

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
mean of 200000 draws 0.00149 (cumulant(1) = 0.00000)
variance of 200000 draws 1.67191 (cumulant(2) = 1.66667)
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
  const auto p0 = pluss::make_series({5.0, 6.25, 7.5}, f64);
  const auto p1 = pluss::make_series({0.0, 0.25, 0.5}, f64);
  const auto p2 = pluss::make_series({1.0, 1.25, 1.5}, f64);
  const distr::StudentT many{p0, p1, p2};
  const auto x = pluss::make_series({-6.0, -2.0, 0.0}, f64);
  const auto density = many.pdf(x);
  std::cout << "one object, three parameter sets:";
  for (double v : density.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';

  // The same numbers, one scalar object at a time.
  {
    {
      const distr::StudentT one{5.0, 0.0, 1.0};
      const auto at = pluss::make_series({-6.0}, f64);
      std::cout << "point 0: " << as_f64(density.select(0, 0)) << " and one object at a time " << as_f64(one.pdf(at)) << "\n";
    }
    {
      const distr::StudentT one{6.25, 0.25, 1.25};
      const auto at = pluss::make_series({-2.0}, f64);
      std::cout << "point 1: " << as_f64(density.select(0, 1)) << " and one object at a time " << as_f64(one.pdf(at)) << "\n";
    }
    {
      const distr::StudentT one{7.5, 0.5, 1.5};
      const auto at = pluss::make_series({0.0}, f64);
      std::cout << "point 2: " << as_f64(density.select(0, 2)) << " and one object at a time " << as_f64(one.pdf(at)) << "\n";
    }
  }
}
```

Output

```text
one object, three parameter sets: 0.00069 0.06748 0.24168
point 0: 0.00069 and one object at a time 0.00069
point 1: 0.06748 and one object at a time 0.06748
point 2: 0.24168 and one object at a time 0.24168
```

One object with three values of each parameter returned the three densities 6.90e-04, 0.06748, 0.24168. Each equals the density of the scalar object built from the i-th values of the parameters and evaluated at the i-th point (the three lines below the first), so a Series-valued object is the same as three scalar objects without the loop.

## Changelog

- **0.1** -- the object form with its evaluation, sampling and cumulant methods.

## See Also

- [`distr::StudentT`](../continuous_real_line/StudentT.md) -- the kind-based functions and the parameters
- [`distr::Normal`](Normal.md)
- [`distr::Laplace`](Laplace.md)
- [`distr::Logistic`](Logistic.md)

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Skellam.html">
    <span class="label">Previous</span>
    <span class="title">Skellam</span>
  </a>
  <a class="next-link" href="Uniform.html">
    <span class="label">Next</span>
    <span class="title">Uniform</span>
  </a>
</div>
<!-- apiref:nav:end -->
