<!-- generated from Alpha.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Alpha`

The alpha distribution: a positively skewed distribution on the positive axis whose density is a normal density in the reciprocal of the variable.

## Signature

```cpp
pluss::Series distr::pdf(dst::Alpha, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Alpha, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Alpha, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Alpha, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Alpha, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The alpha distribution has the density `b phi(a - b/x) / (x² Phi(a))` for `x > 0`, where `phi` and `Phi` are the standard normal density and distribution function: the reciprocal of an alpha variable is a truncated-normal variable. It has a very heavy right tail (no mean) and appears as a model of right-skewed positive data, and as the limit of some first-passage times.

The kind-based functions take `{a, b}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `a` | > 0 | the shape: the reciprocal of the variable is a normal truncated to positive values with this location |
| `b` | > 0 | the scale of the axis |

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

- Parameters outside their valid range are refused with an error that names the requirement (`a` and `b` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Alpha on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.1, 0.3, 0.6, 1.0, 2.0, 5.0}, f64);
  const std::vector<double> params = {2.0, 1.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Alpha, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Alpha, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Alpha, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.10000 0.30000 0.60000 1.00000 2.00000 5.00000
pdf: 0.00000 1.86476 1.07269 0.24760 0.03313 0.00323
cdf: 0.00000 0.09333 0.64524 0.86093 0.95492 0.98651
logpdf: -28.29076 0.62313 0.07017 -1.39593 -3.40722 -5.73480
```

The density is largest on this grid at x = 0.3, where it is 1.86476; the distribution function runs from 0 at x = 0.1 to 0.98651 at x = 5, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(1.86476) = 0.62313 at x = 0.3.

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
  const std::vector<double> params = {2.0, 1.0};

  // The three ways to the probability that the variable lies in [0.2, 6]:
  const auto grid = pluss::series::linspace(0.2, 6.0, 3001, f64);
  const double area = distr::pdf(dst::Alpha, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({0.2, 6.0}, f64);
  const auto c = distr::cdf(dst::Alpha, edges, params).host<double>();

  const auto draws = distr::sample(dst::Alpha, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 0.2).to(pluss::ScalarType::Float64) * (draws <= 6.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(6) - cdf(0.2): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.98774
cdf(6) - cdf(0.2): 0.98774
fraction of 200000 draws in range: 0.98808
```

The integral of the density (0.98774) and the difference of the distribution function (0.98774) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.98808) differs from it by 3.40e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 2.46e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.1, 0.3, 0.6, 1.0, 2.0, 5.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "a 0.5:";
  for (double v : distr::pdf(dst::Alpha, x, {0.5, 1.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "a 2  :";
  for (double v : distr::pdf(dst::Alpha, x, {2.0, 1.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "a 5  :";
  for (double v : distr::pdf(dst::Alpha, x, {5.0, 1.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.1000 0.3000 0.6000 1.0000 2.0000 5.0000
a 0.5: 0.0000 0.1158 0.8115 0.5092 0.1442 0.0221
a 2  : 0.0000 1.8648 1.0727 0.2476 0.0331 0.0032
a 5  : 0.0001 1.1053 0.0043 0.0001 0.0000 0.0000
```

Read down the rows: `a 0.5` peaks on this grid at x = 0.6 (value 0.8115); `a 2` peaks on this grid at x = 0.3 (value 1.8648); `a 5` peaks on this grid at x = 0.3 (value 1.1053). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 checks the distribution function against its closed form, `Phi(a - b/x)/Phi(a)`.

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
  const auto x = pluss::make_series({0.1, 0.3, 0.6, 1.0, 2.0, 5.0}, f64);
  const std::vector<double> params = {2.0, 1.0};

  const auto formula = distr::cdf(dst::Normal, x.reciprocal() * -1.0 + 2.0, {}) * (1.0 / distr::cdf(dst::Normal, pluss::make_series({2.0}, f64), {}).squeeze().item<double>());
  const auto library = distr::cdf(dst::Alpha, x, params);

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
x: 0.100000 0.300000 0.600000 1.000000 2.000000 5.000000
formula: 0.000000 0.093335 0.645238 0.860931 0.954917 0.986513
distr::cdf: 0.000000 0.093335 0.645238 0.860931 0.954917 0.986513
largest difference: 1.11e-16
```

The formula and `distr::cdf` agree to 1.11e-16 over the grid, so the library implements exactly the closed form quoted above; the same expression is the quickest way to check a parameterisation before relying on it.

## See Also

- [`Lognormal`](../continuous_positive/Lognormal.md)
- [`InverseGaussian`](../continuous_positive/InverseGaussian.md)
- [`Pareto`](../continuous_positive/Pareto.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Burr.html">
    <span class="label">Next</span>
    <span class="title">Burr</span>
  </a>
</div>
<!-- apiref:nav:end -->
