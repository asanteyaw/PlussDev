<!-- generated from Gompertz.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Gompertz`

The Gompertz distribution: the distribution of adult lifetimes, with a hazard rate that grows exponentially with age.

## Signature

```cpp
pluss::Series distr::pdf(dst::Gompertz, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Gompertz, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Gompertz, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Gompertz, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Gompertz, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Gompertz distribution has a hazard (failure) rate that increases exponentially with age, `b c^x`, which is the classical law of adult mortality. Its density is `b c^x exp(-b (c^x - 1)/ln c)` and its distribution function `1 - exp(-b (c^x - 1)/ln c)`. It is the limit distribution of the minimum of a large number of independent draws in some settings, and a model for the length of life of systems that wear out.

The kind-based functions take `{b, c}` with `c > 1`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `b` | > 0 | the initial hazard rate (at `x = 0`) |
| `c` | > 1 | the rate of growth of the hazard: a larger `c` makes failure concentrate at earlier ages |

Support: `x >= 0`.

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

- Parameters outside their valid range are refused with an error that names the requirement (`b` must be positive and `c` greater than 1).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Gompertz on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.0, 0.25, 0.5, 1.0, 1.5, 2.5}, f64);
  const std::vector<double> params = {0.8, 2.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Gompertz, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Gompertz, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Gompertz, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.00000 0.25000 0.50000 1.00000 1.50000 2.50000
pdf: 0.80000 0.76473 0.70143 0.50452 0.27425 0.02096
cdf: 0.00000 0.19618 0.38002 0.68468 0.87880 0.99537
logpdf: -0.22314 -0.26823 -0.35464 -0.68415 -1.29371 -3.86501
```

The density is largest on this grid at x = 0, where it is 0.8; the distribution function runs from 0 at x = 0 to 0.99537 at x = 2.5, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.8) = -0.22314 at x = 0.

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
  const std::vector<double> params = {0.8, 2.0};

  // The three ways to the probability that the variable lies in [0.2, 2]:
  const auto grid = pluss::series::linspace(0.2, 2.0, 3001, f64);
  const double area = distr::pdf(dst::Gompertz, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({0.2, 2.0}, f64);
  const auto c = distr::cdf(dst::Gompertz, edges, params).host<double>();

  const auto draws = distr::sample(dst::Gompertz, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 0.2).to(pluss::ScalarType::Float64) * (draws <= 2.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(2) - cdf(0.2): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.81095
cdf(2) - cdf(0.2): 0.81095
fraction of 200000 draws in range: 0.81037
```

The integral of the density (0.81095) and the difference of the distribution function (0.81095) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.81037) differs from it by 5.80e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 8.76e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.0, 0.25, 0.5, 1.0, 1.5, 2.5}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "c 1.5:";
  for (double v : distr::pdf(dst::Gompertz, x, {0.8, 1.5}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "c 2  :";
  for (double v : distr::pdf(dst::Gompertz, x, {0.8, 2.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "c 5  :";
  for (double v : distr::pdf(dst::Gompertz, x, {0.8, 5.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.0000 0.2500 0.5000 1.0000 1.5000 2.5000
c 1.5: 0.8000 0.7173 0.6289 0.4474 0.2818 0.0690
c 2  : 0.8000 0.7647 0.7014 0.5045 0.2743 0.0210
c 5  : 0.8000 0.9352 0.9677 0.5477 0.0567 0.0000
```

Read down the rows: `c 1.5` peaks on this grid at x = 0 (value 0.8); `c 2` peaks on this grid at x = 0 (value 0.8); `c 5` peaks on this grid at x = 0.5 (value 0.9677). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 checks the distribution function against its closed form, `1 - exp(-b (c^x - 1)/ln c)`.

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
  const auto x = pluss::make_series({0.0, 0.25, 0.5, 1.0, 1.5, 2.5}, f64);
  const std::vector<double> params = {0.8, 2.0};

  const auto formula = 1.0 - (((x * 0.6931471805599453).exp() - 1.0) * (-0.8 / 0.6931471805599453)).exp();
  const auto library = distr::cdf(dst::Gompertz, x, params);

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
x: 0.000000 0.250000 0.500000 1.000000 1.500000 2.500000
formula: 0.000000 0.196176 0.380019 0.684676 0.878797 0.995368
distr::cdf: 0.000000 0.196176 0.380019 0.684676 0.878797 0.995368
largest difference: 2.22e-16
```

The formula and `distr::cdf` agree to 2.22e-16 over the grid, so the library implements exactly the closed form quoted above; the same expression is the quickest way to check a parameterisation before relying on it.

## See Also

- [`Weibull`](../continuous_positive/Weibull.md)
- [`ExtremeValue`](../continuous_real_line/ExtremeValue.md)
- [`Exponential`](../continuous_positive/Exponential.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Gig.html">
    <span class="label">Previous</span>
    <span class="title">Gig</span>
  </a>
  <a class="next-link" href="InverseGaussian.html">
    <span class="label">Next</span>
    <span class="title">InverseGaussian</span>
  </a>
</div>
<!-- apiref:nav:end -->
