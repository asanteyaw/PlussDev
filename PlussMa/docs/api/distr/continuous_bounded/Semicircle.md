<!-- generated from Semicircle.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Semicircle`

The semicircle (Wigner) distribution: a density in the shape of a half-circle, the limit law of the eigenvalues of large random symmetric matrices.

## Signature

```cpp
pluss::Series distr::pdf(dst::Semicircle, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Semicircle, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Semicircle, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Semicircle, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Semicircle, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The semicircle distribution has the density `2/(pi R²) sqrt(R² - (x - c)²)` on `[c - R, c + R]`: half of an ellipse. It is the Wigner law, the limiting distribution of the eigenvalues of a large random symmetric matrix with independent entries, and the distribution of the free central limit theorem. Its mean is `c` and its variance `R²/4`.

The kind-based functions take `{center, radius}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `center` | any real | the centre of the support |
| `radius` | > 0 | the radius: the density is positive on `[center - radius, center + radius]` |

Support: the interval `[center - radius, center + radius]`.

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

- Parameters outside their valid range are refused with an error that names the requirement (`radius` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Semicircle on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({-1.0, -0.7, -0.3, 0.0, 0.5, 0.95}, f64);
  const std::vector<double> params = {0.0, 1.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Semicircle, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Semicircle, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Semicircle, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -1.00000 -0.70000 -0.30000 0.00000 0.50000 0.95000
pdf: 0.00000 0.45464 0.60730 0.63662 0.55133 0.19878
cdf: 0.00000 0.09406 0.31192 0.50000 0.80450 0.99334
logpdf: -inf -0.78825 -0.49874 -0.45158 -0.59542 -1.61553
```

The density is largest on this grid at x = 0, where it is 0.63662; the distribution function runs from 0 at x = -1 to 0.99334 at x = 0.95, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.63662) = -0.45158 at x = 0; where the density is zero (outside the support) the logarithm is `-inf`.

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
  const std::vector<double> params = {0.0, 1.0};

  // The three ways to the probability that the variable lies in [-0.8, 0.8]:
  const auto grid = pluss::series::linspace(-0.8, 0.8, 3001, f64);
  const double area = distr::pdf(dst::Semicircle, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({-0.8, 0.8}, f64);
  const auto c = distr::cdf(dst::Semicircle, edges, params).host<double>();

  const auto draws = distr::sample(dst::Semicircle, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= -0.8).to(pluss::ScalarType::Float64) * (draws <= 0.8).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(0.8) - cdf(-0.8): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.89591
cdf(0.8) - cdf(-0.8): 0.89591
fraction of 200000 draws in range: 0.89543
```

The integral of the density (0.89591) and the difference of the distribution function (0.89591) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.89543) differs from it by 4.80e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 6.83e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({-2.0, -1.0, -0.5, 0.0, 0.5, 1.0, 2.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "radius 0.5:";
  for (double v : distr::pdf(dst::Semicircle, x, {0.0, 0.5}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "radius 1  :";
  for (double v : distr::pdf(dst::Semicircle, x, {0.0, 1.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "radius 2  :";
  for (double v : distr::pdf(dst::Semicircle, x, {0.0, 2.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -2.0000 -1.0000 -0.5000 0.0000 0.5000 1.0000 2.0000
radius 0.5: 0.0000 0.0000 0.0000 1.2732 0.0000 0.0000 0.0000
radius 1  : 0.0000 0.0000 0.5513 0.6366 0.5513 0.0000 0.0000
radius 2  : 0.0000 0.2757 0.3082 0.3183 0.3082 0.2757 0.0000
```

Read down the rows: `radius 0.5` peaks on this grid at x = 0 (value 1.2732); `radius 1` peaks on this grid at x = 0 (value 0.6366); `radius 2` peaks on this grid at x = 0 (value 0.3183). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

## See Also

- [`Quadratic`](../continuous_bounded/Quadratic.md)
- [`Triangular`](../continuous_bounded/Triangular.md)
- [`Normal`](../continuous_real_line/Normal.md)
- [`Cosine`](../continuous_bounded/Cosine.md)
