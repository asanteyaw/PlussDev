<!-- generated from Rice.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Rice`

The Rice distribution: the length of a two-dimensional vector whose mean is offset from the origin and whose components have independent normal noise.

## Signature

```cpp
pluss::Series distr::pdf(dst::Rice, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Rice, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Rice, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Rice, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Rice, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Rice (Rician) distribution is the distribution of the length of a two-dimensional vector `(a + e1, b + e2)` where `e1` and `e2` are independent normal noise with standard deviation `sigma` and `(a, b)` has length `nu`: the amplitude of a radio signal with a steady component in noise, or the magnitude of an image in magnetic resonance. With `nu = 0` it is the Rayleigh distribution; for `nu` large relative to `sigma` it approaches a normal distribution centred at `nu`.

The kind-based functions take `{nu, sigma}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `nu` | >= 0 | the distance of the underlying vector from the origin (the steady signal) |
| `sigma` | > 0 | the standard deviation of the noise in each component |

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

- Parameters outside their valid range are refused with an error that names the requirement (`sigma` must be positive and `nu` non-negative).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Rice on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.25, 1.0, 2.0, 3.0, 4.5, 6.5}, f64);
  const std::vector<double> params = {2.0, 1.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Rice, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Rice, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Rice, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.25000 1.00000 2.00000 3.00000 4.50000 6.50000
pdf: 0.03487 0.18712 0.41400 0.30325 0.02668 0.00003
cdf: 0.00429 0.08189 0.39650 0.78564 0.99023 0.99999
logpdf: -3.35599 -1.67601 -0.88188 -1.19320 -3.62370 -10.44459
```

The density is largest on this grid at x = 2, where it is 0.414; the distribution function runs from 0.00429 at x = 0.25 to 0.99999 at x = 6.5, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.414) = -0.88189 at x = 2.

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

  // The three ways to the probability that the variable lies in [0.5, 4.5]:
  const auto grid = pluss::series::linspace(0.5, 4.5, 3001, f64);
  const double area = distr::pdf(dst::Rice, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({0.5, 4.5}, f64);
  const auto c = distr::cdf(dst::Rice, edges, params).host<double>();

  const auto draws = distr::sample(dst::Rice, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 0.5).to(pluss::ScalarType::Float64) * (draws <= 4.5).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(4.5) - cdf(0.5): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.97230
cdf(4.5) - cdf(0.5): 0.97230
fraction of 200000 draws in range: 0.97265
```

The integral of the density (0.9723) and the difference of the distribution function (0.9723) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.97265) differs from it by 3.50e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 3.67e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.5, 1.5, 2.5, 3.5, 5.0, 7.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "nu 0 (Rayleigh):";
  for (double v : distr::pdf(dst::Rice, x, {0.0, 1.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "nu 2            :";
  for (double v : distr::pdf(dst::Rice, x, {2.0, 1.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "nu 5            :";
  for (double v : distr::pdf(dst::Rice, x, {5.0, 1.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.5000 1.5000 2.5000 3.5000 5.0000 7.0000
nu 0 (Rayleigh): 0.4412 0.4870 0.1098 0.0077 0.0000 0.0000
nu 2            : 0.0756 0.3217 0.4049 0.1747 0.0071 0.0000
nu 5            : 0.0000 0.0005 0.0125 0.1092 0.4010 0.0641
```

Read down the rows: `nu 0 (Rayleigh)` peaks on this grid at x = 1.5 (value 0.487); `nu 2` peaks on this grid at x = 2.5 (value 0.4049); `nu 5` peaks on this grid at x = 5 (value 0.401). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 confirms that the Rice distribution with `nu` close to 0 is the Rayleigh distribution, comparing it with the Rayleigh of the same sigma.

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
  const auto x = pluss::make_series({0.25, 1.0, 2.0, 3.0, 4.5}, f64);
  const std::vector<double> here = {1e-08, 1.0};
  const std::vector<double> there = {1.0};

  const double dpdf = (distr::pdf(dst::Rice, x, here) - distr::pdf(dst::Rayleigh, x, there)).abs().max().item<double>();
  const double dcdf = (distr::cdf(dst::Rice, x, here) - distr::cdf(dst::Rayleigh, x, there)).abs().max().item<double>();
  std::cout << "largest difference in the density: " << dpdf << '\n';
  std::cout << "largest difference in the distribution function: " << dcdf << '\n';
}
```

Output

```text
largest difference in the density: 1.39e-17
largest difference in the distribution function: 1.59e-09
```

The two largest differences are 1.39e-17 for the density and 1.59e-09 for the distribution function over the grid: small and shrinking as the approximation improves, so the two descriptions agree to the accuracy shown.

## See Also

- [`Rayleigh`](../continuous_positive/Rayleigh.md)
- [`Chi`](../continuous_positive/Chi.md)
- [`ChiSquareNoncentral`](../continuous_positive/ChiSquareNoncentral.md)
- [`Nakagami`](../continuous_positive/Nakagami.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Rayleigh.html">
    <span class="label">Previous</span>
    <span class="title">Rayleigh</span>
  </a>
  <a class="next-link" href="Wald.html">
    <span class="label">Next</span>
    <span class="title">Wald</span>
  </a>
</div>
<!-- apiref:nav:end -->
