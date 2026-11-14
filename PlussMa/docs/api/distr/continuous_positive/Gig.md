<!-- generated from Gig.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Gig`

The generalised inverse Gaussian distribution: a three-parameter family on the positive axis that contains the inverse Gaussian, the gamma and the inverse gamma distributions.

## Signature

```cpp
pluss::Series distr::pdf(dst::Gig, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Gig, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Gig, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Gig, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Gig, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The generalised inverse Gaussian distribution has a density proportional to `x^(lambda - 1) exp(-(chi/x + psi x)/2)`. It contains the inverse Gaussian distribution (`lambda = -1/2`), the gamma distribution (`chi = 0`) and the inverse gamma distribution (`psi = 0`) as special and limiting cases, and it is the mixing distribution of the generalised hyperbolic family.

The kind-based functions take `{lambda, chi, psi}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `lambda` | any real | the power of `x` in the density; -1/2 is the inverse Gaussian |
| `chi` | >= 0 | the weight of the `1/x` term in the exponent; it controls the left tail |
| `psi` | >= 0 | the weight of the `x` term in the exponent; it controls the right tail |

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

- Parameters outside their valid range are refused with an error that names the requirement (`chi` and `psi` must be non-negative, and not both zero).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Gig on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.1, 0.4, 0.8, 1.2, 2.0, 4.0}, f64);
  const std::vector<double> params = {1.0, 2.0, 3.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Gig, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Gig, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Gig, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.10000 0.40000 0.80000 1.20000 2.00000 4.00000
pdf: 0.00030 0.35056 0.67152 0.55903 0.23499 0.01502
cdf: 0.00000 0.03774 0.26496 0.51740 0.82516 0.98965
logpdf: -8.09821 -1.04821 -0.39821 -0.58155 -1.44821 -4.19821
```

The density is largest on this grid at x = 0.8, where it is 0.67152; the distribution function runs from 0 at x = 0.1 to 0.98965 at x = 4, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.67152) = -0.39821 at x = 0.8.

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
  const std::vector<double> params = {1.0, 2.0, 3.0};

  // The three ways to the probability that the variable lies in [0.2, 3]:
  const auto grid = pluss::series::linspace(0.2, 3.0, 3001, f64);
  const double area = distr::pdf(dst::Gig, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({0.2, 3.0}, f64);
  const auto c = distr::cdf(dst::Gig, edges, params).host<double>();

  const auto draws = distr::sample(dst::Gig, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 0.2).to(pluss::ScalarType::Float64) * (draws <= 3.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(3) - cdf(0.2): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.95521
cdf(3) - cdf(0.2): 0.95521
fraction of 200000 draws in range: 0.95552
```

The integral of the density (0.95521) and the difference of the distribution function (0.95521) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.95552) differs from it by 3.10e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 4.63e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.1, 0.4, 0.8, 1.2, 2.0, 4.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda -0.5:";
  for (double v : distr::pdf(dst::Gig, x, {-0.5, 2.0, 3.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 1   :";
  for (double v : distr::pdf(dst::Gig, x, {1.0, 2.0, 3.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 3   :";
  for (double v : distr::pdf(dst::Gig, x, {3.0, 2.0, 3.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.1000 0.4000 0.8000 1.2000 2.0000 4.0000
lambda -0.5: 0.0081 1.1636 0.7881 0.3571 0.0698 0.0016
lambda 1   : 0.0003 0.3506 0.6715 0.5590 0.2350 0.0150
lambda 3   : 0.0000 0.0227 0.1739 0.3257 0.3803 0.0973
```

Read down the rows: `lambda -0.5` peaks on this grid at x = 0.4 (value 1.1636); `lambda 1` peaks on this grid at x = 0.8 (value 0.6715); `lambda 3` peaks on this grid at x = 2 (value 0.3803). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

## See Also

- [`InverseGaussian`](../continuous_positive/InverseGaussian.md)
- [`Gamma`](../continuous_positive/Gamma.md)
- [`Gh`](../continuous_real_line/Gh.md)
- [`Wald`](../continuous_positive/Wald.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Gamma.html">
    <span class="label">Previous</span>
    <span class="title">Gamma</span>
  </a>
  <a class="next-link" href="Gompertz.html">
    <span class="label">Next</span>
    <span class="title">Gompertz</span>
  </a>
</div>
<!-- apiref:nav:end -->
