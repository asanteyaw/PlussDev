<!-- generated from Rayleigh.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Rayleigh`

The Rayleigh distribution: the length of a two-dimensional vector whose components are independent zero-mean normals.

## Signature

```cpp
pluss::Series distr::pdf(dst::Rayleigh, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Rayleigh, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Rayleigh, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Rayleigh, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Rayleigh, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Rayleigh distribution is the distribution of the length of a two-dimensional vector whose components are independent normal variables with mean zero and the same standard deviation `scale`: the speed of a particle in a plane, the amplitude of a fading radio signal, wind speed in simple models. It equals the Weibull distribution with shape 2 and scale `scale sqrt(2)`, and the chi distribution with two degrees of freedom when `scale = 1`.

The kind-based functions take `{scale}`; the object is `distr::Rayleigh{sigma}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `scale` | > 0 | the standard deviation of each component; the mode of the distribution |

Support: `x >= 0`.

The object form of this family, with the same functions as methods, is on its own page: [`distr::Rayleigh`](../object_based/Rayleigh.md).

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

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Rayleigh on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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
  const std::vector<double> params = {1.5};

  std::cout << "x:";
  for (double v : x.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Rayleigh, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Rayleigh, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Rayleigh, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.25000 0.75000 1.50000 2.50000 4.00000 6.00000
pdf: 0.10958 0.29417 0.40435 0.27706 0.05078 0.00089
cdf: 0.01379 0.11750 0.39347 0.75065 0.97143 0.99966
logpdf: -2.21111 -1.22361 -0.90547 -1.28353 -2.98019 -7.01917
```

The density is largest on this grid at x = 1.5, where it is 0.40435; the distribution function runs from 0.01379 at x = 0.25 to 0.99966 at x = 6, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.40435) = -0.90547 at x = 1.5.

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
  const std::vector<double> params = {1.5};

  // The three ways to the probability that the variable lies in [0.5, 4]:
  const auto grid = pluss::series::linspace(0.5, 4.0, 3001, f64);
  const double area = as_f64(distr::pdf(dst::Rayleigh, grid, params).antidiff(grid).trapezoid());

  const auto edges = pluss::make_series({0.5, 4.0}, f64);
  const auto c = distr::cdf(dst::Rayleigh, edges, params).items<double>();

  const auto draws = distr::sample(dst::Rayleigh, params, {200000}, f64, 7ULL);
  const double fraction = as_f64(((draws >= 0.5).to(pluss::ScalarType::Float64) * (draws <= 4.0).to(pluss::ScalarType::Float64)).mean());

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(4) - cdf(0.5): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.91739
cdf(4) - cdf(0.5): 0.91739
fraction of 200000 draws in range: 0.91646
```

The integral of the density (0.91739) and the difference of the distribution function (0.91739) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.91646) differs from it by 9.30e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 6.16e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  for (double v : x.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "scale 0.7:";
  for (double v : distr::pdf(dst::Rayleigh, x, {0.7}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "scale 1.5:";
  for (double v : distr::pdf(dst::Rayleigh, x, {1.5}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "scale 3  :";
  for (double v : distr::pdf(dst::Rayleigh, x, {3.0}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.2500 0.7500 1.5000 3.0000 5.0000 8.0000
scale 0.7: 0.4787 0.8622 0.3082 0.0006 0.0000 0.0000
scale 1.5: 0.1096 0.2942 0.4044 0.1804 0.0086 0.0000
scale 3  : 0.0277 0.0808 0.1471 0.2022 0.1385 0.0254
```

Read down the rows: `scale 0.7` peaks on this grid at x = 0.75 (value 0.8622); `scale 1.5` peaks on this grid at x = 1.5 (value 0.4044); `scale 3` peaks on this grid at x = 3 (value 0.2022). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 checks the distribution function against its closed form, `1 - exp(-x^2/(2 scale^2))`.

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
  const auto x = pluss::make_series({0.25, 0.75, 1.5, 2.5, 4.0, 6.0}, f64);
  const std::vector<double> params = {1.5};

  const auto formula = 1.0 - (x.pow(2.0) * (-1.0 / (2.0 * 1.5 * 1.5))).exp();
  const auto library = distr::cdf(dst::Rayleigh, x, params);

  std::cout << "x:";
  for (double v : x.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "formula:";
  for (double v : formula.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "distr::cdf:";
  for (double v : library.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << std::scientific << std::setprecision(2) << "largest difference: " << as_f64((formula - library).abs().max()) << '\n';
}
```

Output

```text
x: 0.250000 0.750000 1.500000 2.500000 4.000000 6.000000
formula: 0.013793 0.117503 0.393469 0.750648 0.971434 0.999665
distr::cdf: 0.013793 0.117503 0.393469 0.750648 0.971434 0.999665
largest difference: 0.00e+00
```

The formula and `distr::cdf` agree to 0 over the grid, so the library implements exactly the closed form quoted above; the same expression is the quickest way to check a parameterisation before relying on it.

### Example 5

Example 5 confirms that a Rayleigh distribution with scale 1.5 is a Weibull distribution with shape 2 and scale `1.5 sqrt(2)`.

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
  const std::vector<double> here = {1.5};
  const std::vector<double> there = {2.0, 2.1213203435596424};

  const double dpdf = as_f64((distr::pdf(dst::Rayleigh, x, here) - distr::pdf(dst::Weibull, x, there)).abs().max());
  const double dcdf = as_f64((distr::cdf(dst::Rayleigh, x, here) - distr::cdf(dst::Weibull, x, there)).abs().max());
  std::cout << "largest difference in the density: " << dpdf << '\n';
  std::cout << "largest difference in the distribution function: " << dcdf << '\n';
}
```

Output

```text
largest difference in the density: 5.55e-17
largest difference in the distribution function: 1.11e-16
```

The two largest differences are 5.55e-17 for the density and 1.11e-16 for the distribution function over the grid: zero up to rounding, so the two descriptions are the same distribution.

## See Also

- [`Weibull`](../continuous_positive/Weibull.md)
- [`Chi`](../continuous_positive/Chi.md)
- [`Maxwell`](../continuous_positive/Maxwell.md)
- [`Rice`](../continuous_positive/Rice.md)
- [`distr::Rayleigh`](../object_based/Rayleigh.md) -- the object form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Planck.html">
    <span class="label">Previous</span>
    <span class="title">Planck</span>
  </a>
  <a class="next-link" href="Rice.html">
    <span class="label">Next</span>
    <span class="title">Rice</span>
  </a>
</div>
<!-- apiref:nav:end -->
