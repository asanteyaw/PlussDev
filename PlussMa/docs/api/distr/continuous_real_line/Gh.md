<!-- generated from Gh.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Gh`

The generalised hyperbolic distribution: a five-parameter family that contains the normal inverse Gaussian, the hyperbolic, the variance-gamma and, in a limit, the Student t.

## Signature

```cpp
pluss::Series distr::pdf(dst::Gh, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Gh, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Gh, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Gh, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Gh, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The generalised hyperbolic distribution adds an order `lambda` to the normal inverse Gaussian family: it is a normal variance-mean mixture whose mixing variable follows a generalised inverse Gaussian distribution. `lambda = -1/2` is the normal inverse Gaussian, `lambda = 1` the hyperbolic distribution, and other values give further members. The extra parameter fits tails slightly better, at the cost of a likelihood that is flat along a ridge: the parameters are weakly identified, so interpret a fit through its density, not through its individual parameters.

The kind-based functions take `{lambda, alpha, beta, mu, delta}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `lambda` | any real | the order of the mixing distribution; -1/2 is the NIG, 1 the hyperbolic |
| `alpha` | > abs(beta) | the tail weight |
| `beta` | abs(beta) < alpha | the skewness |
| `mu` | any real | the location |
| `delta` | > 0 | the scale |

Support: all real numbers.

The object form of this family, with the same functions as methods, is on its own page: [`distr::Gh`](../object_based/Gh.md).

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

- Parameters outside their valid range are refused with an error that names the requirement (`alpha` must exceed `abs(beta)` and `delta` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Gh on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({-5.0, -2.0, 0.0, 1.0, 3.0, 6.0}, f64);
  const std::vector<double> params = {1.0, 1.5, -0.4, 0.1, 0.8};

  std::cout << "x:";
  for (double v : x.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Gh, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Gh, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Gh, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -5.00000 -2.00000 0.00000 1.00000 3.00000 6.00000
pdf: 0.00431 0.10301 0.40193 0.14832 0.00445 0.00002
cdf: 0.00397 0.09891 0.62711 0.91172 0.99761 0.99999
logpdf: -5.44567 -2.27296 -0.91147 -1.90837 -5.41461 -11.03311
```

The density is largest on this grid at x = 0, where it is 0.40193; the distribution function runs from 0.00397 at x = -5 to 0.99999 at x = 6, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.40193) = -0.91148 at x = 0.

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
  const std::vector<double> params = {1.0, 1.5, -0.4, 0.1, 0.8};

  // The three ways to the probability that the variable lies in [-3, 3]:
  const auto grid = pluss::series::linspace(-3.0, 3.0, 3001, f64);
  const double area = as_f64(distr::pdf(dst::Gh, grid, params).antidiff(grid).trapezoid());

  const auto edges = pluss::make_series({-3.0, 3.0}, f64);
  const auto c = distr::cdf(dst::Gh, edges, params).items<double>();

  const auto draws = distr::sample(dst::Gh, params, {200000}, f64, 7ULL);
  const double fraction = as_f64(((draws >= -3.0).to(pluss::ScalarType::Float64) * (draws <= 3.0).to(pluss::ScalarType::Float64)).mean());

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(3) - cdf(-3): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.96323
cdf(3) - cdf(-3): 0.96323
fraction of 200000 draws in range: 0.96356
```

The integral of the density (0.96323) and the difference of the distribution function (0.96323) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.96356) differs from it by 3.30e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 4.21e-04), so the density, the distribution function and the sampler describe the same distribution.

### Example 3

Example 3 shows how the shape changes with the parameters: the density at the same points for 4 parameter settings.

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
  const auto x = pluss::make_series({-5.0, -2.0, 0.0, 1.0, 3.0, 6.0}, f64);

  std::cout << "x:";
  for (double v : x.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda -1  :";
  for (double v : distr::pdf(dst::Gh, x, {-1.0, 1.5, -0.4, 0.1, 0.8}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda -0.5:";
  for (double v : distr::pdf(dst::Gh, x, {-0.5, 1.5, -0.4, 0.1, 0.8}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 1   :";
  for (double v : distr::pdf(dst::Gh, x, {1.0, 1.5, -0.4, 0.1, 0.8}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 3   :";
  for (double v : distr::pdf(dst::Gh, x, {3.0, 1.5, -0.4, 0.1, 0.8}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -5.0000 -2.0000 0.0000 1.0000 3.0000 6.0000
lambda -1  : 0.0001 0.0182 0.7783 0.1095 0.0004 0.0000
lambda -0.5: 0.0004 0.0324 0.6714 0.1274 0.0009 0.0000
lambda 1   : 0.0043 0.1030 0.4019 0.1483 0.0045 0.0000
lambda 3   : 0.0253 0.1713 0.2210 0.1177 0.0112 0.0001
```

Read down the rows: `lambda -1` peaks on this grid at x = 0 (value 0.7783); `lambda -0.5` peaks on this grid at x = 0 (value 0.6714); `lambda 1` peaks on this grid at x = 0 (value 0.4019); `lambda 3` peaks on this grid at x = 0 (value 0.221). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

## See Also

- [`Nig`](../continuous_real_line/Nig.md)
- [`Normal`](../continuous_real_line/Normal.md)
- [`StudentT`](../continuous_real_line/StudentT.md)
- [`Gig`](../continuous_positive/Gig.md)
- [`distr::Gh`](../object_based/Gh.md) -- the object form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ExtremeValue.html">
    <span class="label">Previous</span>
    <span class="title">ExtremeValue</span>
  </a>
  <a class="next-link" href="HyperbolicSecant.html">
    <span class="label">Next</span>
    <span class="title">HyperbolicSecant</span>
  </a>
</div>
<!-- apiref:nav:end -->
