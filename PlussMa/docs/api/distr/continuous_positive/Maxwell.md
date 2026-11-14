<!-- generated from Maxwell.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Maxwell`

The Maxwell distribution: the length of a three-dimensional vector of independent zero-mean normal variables, the speed of a gas molecule.

## Signature

```cpp
pluss::Series distr::pdf(dst::Maxwell, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Maxwell, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Maxwell, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Maxwell, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Maxwell, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Maxwell (Maxwell–Boltzmann) distribution is the distribution of the speed of a particle in a gas, which is the length of its three-dimensional velocity vector when the components are independent normals with mean zero and standard deviation `scale`. It is the chi distribution with three degrees of freedom and scale `scale`.

The kind-based functions take `{scale}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `scale` | > 0 | the standard deviation of each velocity component |

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

- Parameters outside their valid range are refused with an error that names the requirement (`scale` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Maxwell on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.25, 0.75, 1.5, 2.5, 4.0, 6.0}, f64);
  const std::vector<double> params = {1.2};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Maxwell, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Maxwell, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Maxwell, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.25000 0.75000 1.50000 2.50000 4.00000 6.00000
pdf: 0.02824 0.21365 0.47565 0.32946 0.02856 0.00006
cdf: 0.00237 0.05783 0.33208 0.77301 0.98886 0.99998
logpdf: -3.56705 -1.54343 -0.74308 -1.11031 -3.55572 -9.68924
```

The density is largest on this grid at x = 1.5, where it is 0.47565; the distribution function runs from 0.00237 at x = 0.25 to 0.99998 at x = 6, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.47565) = -0.74307 at x = 1.5.

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
  const std::vector<double> params = {1.2};

  // The three ways to the probability that the variable lies in [0.5, 4]:
  const auto grid = pluss::series::linspace(0.5, 4.0, 3001, f64);
  const double area = distr::pdf(dst::Maxwell, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({0.5, 4.0}, f64);
  const auto c = distr::cdf(dst::Maxwell, edges, params).host<double>();

  const auto draws = distr::sample(dst::Maxwell, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 0.5).to(pluss::ScalarType::Float64) * (draws <= 4.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(4) - cdf(0.5): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.97059
cdf(4) - cdf(0.5): 0.97059
fraction of 200000 draws in range: 0.97103
```

The integral of the density (0.97059) and the difference of the distribution function (0.97059) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.97103) differs from it by 4.40e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 3.78e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.25, 0.75, 1.5, 3.0, 5.0, 8.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "scale 0.7:";
  for (double v : distr::pdf(dst::Maxwell, x, {0.7}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "scale 1.2:";
  for (double v : distr::pdf(dst::Maxwell, x, {1.2}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "scale 2.5:";
  for (double v : distr::pdf(dst::Maxwell, x, {2.5}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.2500 0.7500 1.5000 3.0000 5.0000 8.0000
scale 0.7: 0.1364 0.7370 0.5269 0.0022 0.0000 0.0000
scale 1.2: 0.0282 0.2136 0.4756 0.1826 0.0020 0.0000
scale 2.5: 0.0032 0.0275 0.0960 0.2237 0.1728 0.0195
```

Read down the rows: `scale 0.7` peaks on this grid at x = 0.75 (value 0.737); `scale 1.2` peaks on this grid at x = 1.5 (value 0.4756); `scale 2.5` peaks on this grid at x = 3 (value 0.2237). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 confirms that a Maxwell distribution with scale 1.2 is the chi distribution with 3 degrees of freedom and scale 1.2.

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
  const auto x = pluss::make_series({0.25, 0.75, 1.5, 2.5, 4.0}, f64);
  const std::vector<double> here = {1.2};
  const std::vector<double> there = {0.0, 1.2, 3.0};

  const double dpdf = (distr::pdf(dst::Maxwell, x, here) - distr::pdf(dst::Chi, x, there)).abs().max().item<double>();
  const double dcdf = (distr::cdf(dst::Maxwell, x, here) - distr::cdf(dst::Chi, x, there)).abs().max().item<double>();
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

- [`Chi`](../continuous_positive/Chi.md)
- [`Rayleigh`](../continuous_positive/Rayleigh.md)
- [`ChiSquare`](../continuous_positive/ChiSquare.md)
- [`Gamma`](../continuous_positive/Gamma.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Lomax.html">
    <span class="label">Previous</span>
    <span class="title">Lomax</span>
  </a>
  <a class="next-link" href="Nakagami.html">
    <span class="label">Next</span>
    <span class="title">Nakagami</span>
  </a>
</div>
<!-- apiref:nav:end -->
