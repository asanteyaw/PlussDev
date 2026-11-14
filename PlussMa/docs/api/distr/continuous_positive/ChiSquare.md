<!-- generated from ChiSquare.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::ChiSquare`

The chi-squared distribution: the distribution of a sum of squared independent standard normal variables.

## Signature

```cpp
pluss::Series distr::pdf(dst::ChiSquare, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::ChiSquare, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::ChiSquare, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::ChiSquare, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::ChiSquare, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The chi-squared distribution with `nu` degrees of freedom is the distribution of the sum of `nu` squared independent standard normal variables. It is the reference distribution of variance estimates and of the likelihood-ratio, Pearson and Ljung–Box test statistics. Its mean is `nu` and its variance `2 nu`. It is a gamma distribution with shape `nu/2` and scale 2.

The kind-based functions take `{nu}`; the object `distr::ChiSquare{degrees_of_freedom}` takes an integer number of degrees of freedom.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `nu` | > 0 | the degrees of freedom: the mean of the distribution |

Support: `x >= 0`.

The object form of this family, with the same functions as methods, is on its own page: [`distr::ChiSquare`](../object_based/ChiSquare.md).

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

- Parameters outside their valid range are refused with an error that names the requirement (`nu` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the ChiSquare on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.5, 1.0, 3.0, 5.0, 9.0, 15.0}, f64);
  const std::vector<double> params = {5.0};

  std::cout << "x:";
  for (double v : x.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::ChiSquare, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::ChiSquare, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::ChiSquare, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.50000 1.00000 3.00000 5.00000 9.00000 15.00000
pdf: 0.03662 0.08066 0.15418 0.12204 0.03989 0.00427
cdf: 0.00788 0.03743 0.30001 0.58412 0.89094 0.98964
logpdf: -3.30727 -2.51755 -1.86963 -2.10339 -3.22171 -5.45548
```

The density is largest on this grid at x = 3, where it is 0.15418; the distribution function runs from 0.00788 at x = 0.5 to 0.98964 at x = 15, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.15418) = -1.86963 at x = 3.

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
  const std::vector<double> params = {5.0};

  // The three ways to the probability that the variable lies in [1, 9]:
  const auto grid = pluss::series::linspace(1.0, 9.0, 3001, f64);
  const double area = as_f64(distr::pdf(dst::ChiSquare, grid, params).antidiff(grid).trapezoid());

  const auto edges = pluss::make_series({1.0, 9.0}, f64);
  const auto c = distr::cdf(dst::ChiSquare, edges, params).items<double>();

  const auto draws = distr::sample(dst::ChiSquare, params, {200000}, f64, 7ULL);
  const double fraction = as_f64(((draws >= 1.0).to(pluss::ScalarType::Float64) * (draws <= 9.0).to(pluss::ScalarType::Float64)).mean());

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(9) - cdf(1): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.85350
cdf(9) - cdf(1): 0.85350
fraction of 200000 draws in range: 0.85215
```

The integral of the density (0.8535) and the difference of the distribution function (0.8535) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.85215) differs from it by 0.00135, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 7.91e-04), so the density, the distribution function and the sampler describe the same distribution.

### Example 3

Example 3 shows how the shape changes with the parameters: the density at the same points for 3 parameter settings.

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
  const auto x = pluss::make_series({0.5, 2.0, 4.0, 8.0, 12.0, 20.0}, f64);

  std::cout << "x:";
  for (double v : x.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "nu 2 :";
  for (double v : distr::pdf(dst::ChiSquare, x, {2.0}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "nu 5 :";
  for (double v : distr::pdf(dst::ChiSquare, x, {5.0}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "nu 12:";
  for (double v : distr::pdf(dst::ChiSquare, x, {12.0}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.5000 2.0000 4.0000 8.0000 12.0000 20.0000
nu 2 : 0.3894 0.1839 0.0677 0.0092 0.0012 0.0000
nu 5 : 0.0366 0.1384 0.1440 0.0551 0.0137 0.0005
nu 12: 0.0000 0.0015 0.0180 0.0781 0.0803 0.0189
```

Read down the rows: `nu 2` peaks on this grid at x = 0.5 (value 0.3894); `nu 5` peaks on this grid at x = 4 (value 0.144); `nu 12` peaks on this grid at x = 12 (value 0.0803). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 confirms that the chi-squared distribution with 5 degrees of freedom is a gamma distribution with shape 2.5 and scale 2.

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
  const auto x = pluss::make_series({0.5, 1.5, 3.0, 6.0, 12.0}, f64);
  const std::vector<double> here = {5.0};
  const std::vector<double> there = {0.0, 2.0, 2.5};

  const double dpdf = as_f64((distr::pdf(dst::ChiSquare, x, here) - distr::pdf(dst::Gamma, x, there)).abs().max());
  const double dcdf = as_f64((distr::cdf(dst::ChiSquare, x, here) - distr::cdf(dst::Gamma, x, there)).abs().max());
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

- [`Gamma`](../continuous_positive/Gamma.md)
- [`ChiSquareNoncentral`](../continuous_positive/ChiSquareNoncentral.md)
- [`Chi`](../continuous_positive/Chi.md)
- `FDistribution`
- [`Exponential`](../continuous_positive/Exponential.md)
- [`distr::ChiSquare`](../object_based/ChiSquare.md) -- the object form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Chi.html">
    <span class="label">Previous</span>
    <span class="title">Chi</span>
  </a>
  <a class="next-link" href="ChiSquareNoncentral.html">
    <span class="label">Next</span>
    <span class="title">ChiSquareNoncentral</span>
  </a>
</div>
<!-- apiref:nav:end -->
