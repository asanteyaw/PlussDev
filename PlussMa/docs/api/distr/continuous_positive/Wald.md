<!-- generated from Wald.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Wald`

The Wald distribution: the inverse Gaussian distribution with mean 1.

## Signature

```cpp
pluss::Series distr::pdf(dst::Wald, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Wald, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Wald, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Wald, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Wald, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Wald distribution is the inverse Gaussian distribution with mean 1 and a single shape parameter `chi`: the standardised form in which the mean has been scaled out. A Wald variable multiplied by `mean` is an inverse Gaussian variable with that mean.

The kind-based functions take `{chi}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `chi` | > 0 | the shape; the variance is `1/chi` |

Support: `x > 0`.

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

- Parameters outside their valid range are refused with an error that names the requirement (`chi` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Wald on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.1, 0.4, 0.8, 1.0, 1.5, 3.0}, f64);
  const std::vector<double> params = {2.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Wald, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Wald, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Wald, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.10000 0.40000 0.80000 1.00000 1.50000 3.00000
pdf: 0.00542 0.90671 0.75002 0.56419 0.25996 0.02862
cdf: 0.00005 0.13750 0.49675 0.62770 0.82441 0.97854
logpdf: -5.21849 -0.09793 -0.28765 -0.57236 -1.34723 -3.55362
```

The density is largest on this grid at x = 0.4, where it is 0.90671; the distribution function runs from 5.00e-05 at x = 0.1 to 0.97854 at x = 3, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.90671) = -0.09793 at x = 0.4.

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
  const std::vector<double> params = {2.0};

  // The three ways to the probability that the variable lies in [0.3, 2.5]:
  const auto grid = pluss::series::linspace(0.3, 2.5, 3001, f64);
  const double area = distr::pdf(dst::Wald, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({0.3, 2.5}, f64);
  const auto c = distr::cdf(dst::Wald, edges, params).host<double>();

  const auto draws = distr::sample(dst::Wald, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 0.3).to(pluss::ScalarType::Float64) * (draws <= 2.5).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(2.5) - cdf(0.3): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.90089
cdf(2.5) - cdf(0.3): 0.90089
fraction of 200000 draws in range: 0.89948
```

The integral of the density (0.90089) and the difference of the distribution function (0.90089) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.89948) differs from it by 0.00141, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 6.68e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.1, 0.4, 0.8, 1.0, 1.5, 3.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "chi 0.5:";
  for (double v : distr::pdf(dst::Wald, x, {0.5}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "chi 2  :";
  for (double v : distr::pdf(dst::Wald, x, {2.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "chi 10 :";
  for (double v : distr::pdf(dst::Wald, x, {10.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.1000 0.4000 0.8000 1.0000 1.5000 3.0000
chi 0.5: 1.1775 0.8904 0.3893 0.2821 0.1473 0.0389
chi 2  : 0.0054 0.9067 0.7500 0.5642 0.2600 0.0286
chi 10 : 0.0000 0.0554 1.3731 1.2616 0.2984 0.0003
```

Read down the rows: `chi 0.5` peaks on this grid at x = 0.1 (value 1.1775); `chi 2` peaks on this grid at x = 0.4 (value 0.9067); `chi 10` peaks on this grid at x = 0.8 (value 1.3731). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 confirms that the Wald distribution with parameter 3 is the inverse Gaussian distribution with mean 1 and shape 3.

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
  const auto x = pluss::make_series({0.1, 0.4, 0.8, 1.5, 3.0}, f64);
  const std::vector<double> here = {3.0};
  const std::vector<double> there = {1.0, 3.0};

  const double dpdf = (distr::pdf(dst::Wald, x, here) - distr::pdf(dst::InverseGaussian, x, there)).abs().max().item<double>();
  const double dcdf = (distr::cdf(dst::Wald, x, here) - distr::cdf(dst::InverseGaussian, x, there)).abs().max().item<double>();
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

- [`InverseGaussian`](../continuous_positive/InverseGaussian.md)
- [`Gig`](../continuous_positive/Gig.md)
- [`Lognormal`](../continuous_positive/Lognormal.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Rice.html">
    <span class="label">Previous</span>
    <span class="title">Rice</span>
  </a>
  <a class="next-link" href="Weibull.html">
    <span class="label">Next</span>
    <span class="title">Weibull</span>
  </a>
</div>
<!-- apiref:nav:end -->
