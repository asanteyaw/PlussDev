<!-- generated from Arcsin.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Arcsin`

The arcsine distribution: a U-shaped density on the unit interval that piles up at both ends.

## Signature

```cpp
pluss::Series distr::pdf(dst::Arcsin, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Arcsin, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Arcsin, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Arcsin, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Arcsin, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The arcsine distribution has the density `1/(pi sqrt(x (1 - x)))` on `(0, 1)`, which is smallest in the middle and unbounded at both ends. It is the beta distribution with both parameters equal to 1/2. It is the distribution of the fraction of time a symmetric random walk spends on the positive side, and the distribution of a sine wave sampled at a random time (rescaled), which is why the extremes are the most likely values.

The kind-based functions take `{}`: the distribution has no parameters.

Support: the open interval `(0, 1)`.

## Parameters

`const pluss::Series& x`
: The points at which to evaluate, a `Series` of any shape; the result has the same shape. Points outside the support have density zero and distribution value 0 or 1.

`const std::vector<double>& params`
: The parameters of the family, in the order shown in the table above. A list of the wrong length is refused.

`std::initializer_list<std::int64_t> sizes`
: The shape of the array of random draws, for example `{n}` or `{rows, columns}`.

`pluss::Config cfg`
: The dtype and platform of the draws; the default is Float32 on the CPU, so request Float64 when precision matters.

`std::uint64_t seed`
: The seed of the stream. The same seed always gives the same draws; a seed of 0 is a fixed default, not a random one.

## Returns

`pdf`, `logpdf`, `cdf` and `logcdf` return a `Series` of the shape of `x`; `sample` returns a `Series` of the requested shape. 

## Errors

- Parameters outside their valid range are refused with an error that names the requirement (the distribution has no parameters).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Arcsin on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.02, 0.1, 0.3, 0.5, 0.7, 0.98}, f64);
  const std::vector<double> params = {};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Arcsin, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Arcsin, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Arcsin, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.02000 0.10000 0.30000 0.50000 0.70000 0.98000
pdf: 2.27364 1.06103 0.69461 0.63662 0.69461 2.27364
cdf: 0.09033 0.20483 0.36901 0.50000 0.63099 0.90967
logpdf: 0.82138 0.05924 -0.36441 -0.45158 -0.36441 0.82138
```

The density is largest on this grid at x = 0.02, where it is 2.27364; the distribution function runs from 0.09033 at x = 0.02 to 0.90967 at x = 0.98, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(2.27364) = 0.82138 at x = 0.02.

### Example 2

Example 2 checks that the three descriptions of the same probability agree: the integral of the density (sum of the masses) over a range, the difference of the distribution function at its ends, and the fraction of random draws that land in it.

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
  const std::vector<double> params = {};

  // The three ways to the probability that the variable lies in [0.1, 0.9]:
  const auto grid = pluss::series::linspace(0.1, 0.9, 3001, f64);
  const double area = distr::pdf(dst::Arcsin, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({0.1, 0.9}, f64);
  const auto c = distr::cdf(dst::Arcsin, edges, params).host<double>();

  const auto draws = distr::sample(dst::Arcsin, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 0.1).to(pluss::ScalarType::Float64) * (draws <= 0.9).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(0.9) - cdf(0.1): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.59033
cdf(0.9) - cdf(0.1): 0.59033
fraction of 200000 draws in range: 0.59067
```

The integral of the density (0.59033) and the difference of the distribution function (0.59033) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.59067) differs from it by 3.40e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 0.0011), so the density, the distribution function and the sampler describe the same distribution.

### Example 3

Example 3 checks the distribution function against its closed form, `2 arcsin(sqrt(x))/pi`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  std::cout << std::fixed << std::setprecision(6);
  const auto x = pluss::make_series({0.02, 0.1, 0.3, 0.5, 0.7, 0.98}, f64);
  const std::vector<double> params = {};

  const auto formula = x.sqrt().asin() * (2.0 / 3.14159265358979323846);
  const auto library = distr::cdf(dst::Arcsin, x, params);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "formula:";
  for (double v : formula.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "distr::cdf:";
  for (double v : library.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << std::scientific << std::setprecision(2) << "largest difference: " << (formula - library).abs().max().item<double>() << '\n';
}
```

Output

```text
x: 0.020000 0.100000 0.300000 0.500000 0.700000 0.980000
formula: 0.090334 0.204833 0.369010 0.500000 0.630990 0.909666
distr::cdf: 0.090334 0.204833 0.369010 0.500000 0.630990 0.909666
largest difference: 4.44e-16
```

The formula and `distr::cdf` agree to 4.44e-16 over the grid, so the library implements exactly the closed form quoted above; the same expression is the quickest way to check a parameterisation before relying on it.

### Example 4

Example 4 confirms that the arcsine distribution is the beta distribution with both parameters equal to 1/2.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  std::cout << std::scientific << std::setprecision(2);
  const auto x = pluss::make_series({0.1, 0.3, 0.5, 0.8, 0.95}, f64);
  const std::vector<double> here = {};
  const std::vector<double> there = {0.5, 0.5};

  const double dpdf = (distr::pdf(dst::Arcsin, x, here) - distr::pdf(dst::Beta, x, there)).abs().max().item<double>();
  const double dcdf = (distr::cdf(dst::Arcsin, x, here) - distr::cdf(dst::Beta, x, there)).abs().max().item<double>();
  std::cout << "largest difference in the density: " << dpdf << '\n';
  std::cout << "largest difference in the distribution function: " << dcdf << '\n';
}
```

Output

```text
largest difference in the density: 0.00e+00
largest difference in the distribution function: 0.00e+00
```

The two largest differences are 0 for the density and 0 for the distribution function over the grid: zero up to rounding, so the two descriptions are the same distribution.

## See Also

- [`Beta`](../continuous_bounded/Beta.md)
- [`UShape`](../continuous_bounded/UShape.md)
- [`Uniform`](../continuous_bounded/Uniform.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Anglit.html">
    <span class="label">Previous</span>
    <span class="title">Anglit</span>
  </a>
  <a class="next-link" href="Beta.html">
    <span class="label">Next</span>
    <span class="title">Beta</span>
  </a>
</div>
<!-- apiref:nav:end -->
