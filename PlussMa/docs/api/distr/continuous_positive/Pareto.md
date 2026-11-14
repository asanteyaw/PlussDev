<!-- generated from Pareto.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Pareto`

The Pareto distribution: the power-law distribution of sizes above a minimum, the standard model of wealth, city sizes and insurance losses.

## Signature

```cpp
pluss::Series distr::pdf(dst::Pareto, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Pareto, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Pareto, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Pareto, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Pareto, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Pareto distribution has a density that starts at its minimum `xm` and decays as a power, `alpha xm^alpha / x^(alpha + 1)`, so the probability of exceeding `x` is `(xm/x)^alpha`. Its tail is polynomial: the mean exists only for `alpha > 1` and the variance only for `alpha > 2`. A smaller `alpha` is a heavier tail.

The kind-based functions take `{xm, alpha}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `xm` | > 0 | the minimum value: the left end of the support |
| `alpha` | > 0 | the tail index: the probability of exceeding `x` decays like `x^(-alpha)` |

Support: `x >= xm`.

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

- Parameters outside their valid range are refused with an error that names the requirement (`xm` and `alpha` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Pareto on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.5, 1.0, 1.5, 2.0, 4.0, 10.0}, f64);
  const std::vector<double> params = {1.0, 3.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Pareto, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Pareto, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Pareto, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.50000 1.00000 1.50000 2.00000 4.00000 10.00000
pdf: 0.00000 3.00000 0.59259 0.18750 0.01172 0.00030
cdf: 0.00000 0.00000 0.70370 0.87500 0.98438 0.99900
logpdf: -inf 1.09861 -0.52325 -1.67398 -4.44657 -8.11173
```

The density is largest on this grid at x = 1, where it is 3; the distribution function runs from 0 at x = 0.5 to 0.999 at x = 10, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(3) = 1.09861 at x = 1; where the density is zero (outside the support) the logarithm is `-inf`.

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
  const std::vector<double> params = {1.0, 3.0};

  // The three ways to the probability that the variable lies in [1, 6]:
  const auto grid = pluss::series::linspace(1.0, 6.0, 3001, f64);
  const double area = distr::pdf(dst::Pareto, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({1.0, 6.0}, f64);
  const auto c = distr::cdf(dst::Pareto, edges, params).host<double>();

  const auto draws = distr::sample(dst::Pareto, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 1.0).to(pluss::ScalarType::Float64) * (draws <= 6.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(6) - cdf(1): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.99537
cdf(6) - cdf(1): 0.99537
fraction of 200000 draws in range: 0.99552
```

The integral of the density (0.99537) and the difference of the distribution function (0.99537) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.99552) differs from it by 1.50e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 1.52e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({1.0, 1.25, 1.5, 2.0, 3.0, 5.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "alpha 1.5:";
  for (double v : distr::pdf(dst::Pareto, x, {1.0, 1.5}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "alpha 3  :";
  for (double v : distr::pdf(dst::Pareto, x, {1.0, 3.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "alpha 8  :";
  for (double v : distr::pdf(dst::Pareto, x, {1.0, 8.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 1.0000 1.2500 1.5000 2.0000 3.0000 5.0000
alpha 1.5: 1.5000 0.8587 0.5443 0.2652 0.0962 0.0268
alpha 3  : 3.0000 1.2288 0.5926 0.1875 0.0370 0.0048
alpha 8  : 8.0000 1.0737 0.2081 0.0156 0.0004 0.0000
```

Read down the rows: `alpha 1.5` peaks on this grid at x = 1 (value 1.5); `alpha 3` peaks on this grid at x = 1 (value 3); `alpha 8` peaks on this grid at x = 1 (value 8). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 checks the distribution function against its closed form, `1 - (xm/x)^alpha` for `x >= xm`.

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
  const auto x = pluss::make_series({1.0, 1.5, 2.0, 4.0, 10.0}, f64);
  const std::vector<double> params = {1.0, 3.0};

  const auto formula = 1.0 - (x.reciprocal() * 1.0).pow(3.0);
  const auto library = distr::cdf(dst::Pareto, x, params);

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
x: 1.000000 1.500000 2.000000 4.000000 10.000000
formula: 0.000000 0.703704 0.875000 0.984375 0.999000
distr::cdf: 0.000000 0.703704 0.875000 0.984375 0.999000
largest difference: 0.00e+00
```

The formula and `distr::cdf` agree to 0 over the grid, so the library implements exactly the closed form quoted above; the same expression is the quickest way to check a parameterisation before relying on it.

## See Also

- [`distr::Pareto` object](../object_based/Pareto.md) -- the object form, with exact cumulants below the tail index
- [`Lomax`](../continuous_positive/Lomax.md)
- [`Burr`](../continuous_positive/Burr.md)
- [`Lognormal`](../continuous_positive/Lognormal.md)
- [`Weibull`](../continuous_positive/Weibull.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Nakagami.html">
    <span class="label">Previous</span>
    <span class="title">Nakagami</span>
  </a>
  <a class="next-link" href="Planck.html">
    <span class="label">Next</span>
    <span class="title">Planck</span>
  </a>
</div>
<!-- apiref:nav:end -->
