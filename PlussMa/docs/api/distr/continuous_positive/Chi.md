<!-- generated from Chi.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Chi`

The chi distribution: the distribution of the length of a vector of independent standard normal variables.

## Signature

```cpp
pluss::Series distr::pdf(dst::Chi, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Chi, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Chi, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Chi, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Chi, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The chi distribution with `nu` degrees of freedom is the square root of a chi-squared variable: the length of a `nu`-dimensional vector of independent standard normals. With `nu = 2` it is the Rayleigh distribution and with `nu = 3` the Maxwell distribution; the half-normal distribution is `nu = 1`.

The kind-based functions take `{nu}` or `{location, scale, nu}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `location` | any real | the left end of the support (0 in the short form) |
| `scale` | > 0 | the scale (1 in the short form) |
| `nu` | > 0 | the degrees of freedom |

Support: `x` above the location.

## Parameters

`const pluss::Series& x`
: The points at which to evaluate, a `Series` of any shape; the result has the same shape. Points outside the support have density zero and distribution value 0 or 1.

`const std::vector<double>& params`
: The parameters of the family, in the order shown in the table above (a shorter list selects the defaults: `{nu}` gives location 0 and scale 1). A list of the wrong length is refused.

`std::initializer_list<std::int64_t> sizes`
: The shape of the array of random draws, for example `{n}` or `{rows, columns}`.

`pluss::Config cfg`
: The dtype and platform of the draws; the default is Float32 on the CPU, so request Float64 when precision matters.

`std::uint64_t seed`
: The seed of the stream. The same seed always gives the same draws; a seed of 0 is a fixed default, not a random one.

## Returns

`pdf`, `logpdf`, `cdf` and `logcdf` return a `Series` of the shape of `x`; `sample` returns a `Series` of the requested shape. 

## Errors

- Parameters outside their valid range are refused with an error that names the requirement (`nu` and `scale` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Chi on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.25, 0.75, 1.5, 2.0, 3.0, 4.0}, f64);
  const std::vector<double> params = {3.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Chi, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Chi, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Chi, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.25000 0.75000 1.50000 2.00000 3.00000 4.00000
pdf: 0.04833 0.33878 0.58283 0.43193 0.07977 0.00428
cdf: 0.00408 0.09504 0.47783 0.73854 0.97071 0.99887
logpdf: -3.02963 -1.08241 -0.53986 -0.83950 -2.52857 -5.45320
```

The density is largest on this grid at x = 1.5, where it is 0.58283; the distribution function runs from 0.00408 at x = 0.25 to 0.99887 at x = 4, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.58283) = -0.53986 at x = 1.5.

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
  const std::vector<double> params = {3.0};

  // The three ways to the probability that the variable lies in [0.5, 3]:
  const auto grid = pluss::series::linspace(0.5, 3.0, 3001, f64);
  const double area = distr::pdf(dst::Chi, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({0.5, 3.0}, f64);
  const auto c = distr::cdf(dst::Chi, edges, params).host<double>();

  const auto draws = distr::sample(dst::Chi, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 0.5).to(pluss::ScalarType::Float64) * (draws <= 3.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(3) - cdf(0.5): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.93985
cdf(3) - cdf(0.5): 0.93985
fraction of 200000 draws in range: 0.93977
```

The integral of the density (0.93985) and the difference of the distribution function (0.93985) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.93977) differs from it by 8.00e-05, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 5.32e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.25, 0.75, 1.5, 2.5, 3.5, 5.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "nu 1:";
  for (double v : distr::pdf(dst::Chi, x, {1.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "nu 3:";
  for (double v : distr::pdf(dst::Chi, x, {3.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "nu 8:";
  for (double v : distr::pdf(dst::Chi, x, {8.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.2500 0.7500 1.5000 2.5000 3.5000 5.0000
nu 1: 0.7733 0.6023 0.2590 0.0351 0.0017 0.0000
nu 3: 0.0483 0.3388 0.5828 0.2191 0.0214 0.0001
nu 8: 0.0000 0.0021 0.1156 0.5587 0.2932 0.0061
```

Read down the rows: `nu 1` peaks on this grid at x = 0.25 (value 0.7733); `nu 3` peaks on this grid at x = 1.5 (value 0.5828); `nu 8` peaks on this grid at x = 2.5 (value 0.5587). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 confirms that the chi distribution with 2 degrees of freedom is the Rayleigh distribution.

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
  const auto x = pluss::make_series({0.25, 0.75, 1.5, 2.5, 3.5}, f64);
  const std::vector<double> here = {2.0};
  const std::vector<double> there = {1.0};

  const double dpdf = (distr::pdf(dst::Chi, x, here) - distr::pdf(dst::Rayleigh, x, there)).abs().max().item<double>();
  const double dcdf = (distr::cdf(dst::Chi, x, here) - distr::cdf(dst::Rayleigh, x, there)).abs().max().item<double>();
  std::cout << "largest difference in the density: " << dpdf << '\n';
  std::cout << "largest difference in the distribution function: " << dcdf << '\n';
}
```

Output

```text
largest difference in the density: 1.11e-16
largest difference in the distribution function: 5.55e-16
```

The two largest differences are 1.11e-16 for the density and 5.55e-16 for the distribution function over the grid: zero up to rounding, so the two descriptions are the same distribution.

## See Also

- [`ChiSquare`](../continuous_positive/ChiSquare.md)
- [`Rayleigh`](../continuous_positive/Rayleigh.md)
- [`Maxwell`](../continuous_positive/Maxwell.md)
- [`Nakagami`](../continuous_positive/Nakagami.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Burr.html">
    <span class="label">Previous</span>
    <span class="title">Burr</span>
  </a>
  <a class="next-link" href="ChiSquare.html">
    <span class="label">Next</span>
    <span class="title">ChiSquare</span>
  </a>
</div>
<!-- apiref:nav:end -->
