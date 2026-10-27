<!-- generated from Anglit.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Anglit`

The anglit distribution: a cosine-shaped density on a symmetric interval, the distribution of the arcsine of a random square root.

## Signature

```cpp
pluss::Series distr::pdf(dst::Anglit, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Anglit, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Anglit, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Anglit, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Anglit, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The anglit distribution has the density `cos(2 (x - t))` on `[t - pi/4, t + pi/4]` in its standard form (`s = pi/4`): a single smooth arch, highest at the centre and falling to zero at the ends. Its distribution function is `(1 + sin(2 (x - t)))/2`. It is a bounded, symmetric model with a smooth peak.

The kind-based functions take `{}` (centre 0, half-width pi/4) or `{t, s}`, where `s` is the half-width of the support.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `t` | any real | the centre |
| `s` | > 0 | the half-width of the support: the density is positive on `[t - s, t + s]` |

Support: the interval `[t - s, t + s]`.

## Parameters

`const pluss::Series& x`
: The points at which to evaluate, a `Series` of any shape; the result has the same shape. Points outside the support have density zero and distribution value 0 or 1.

`const std::vector<double>& params`
: The parameters of the family, in the order shown in the table above (a shorter list selects the defaults: `{}` gives centre 0 and half-width pi/4). A list of the wrong length is refused.

`std::initializer_list<std::int64_t> sizes`
: The shape of the array of random draws, for example `{n}` or `{rows, columns}`.

`pluss::Config cfg`
: The dtype and platform of the draws; the default is Float32 on the CPU, so request Float64 when precision matters.

`std::uint64_t seed`
: The seed of the stream. The same seed always gives the same draws; a seed of 0 is a fixed default, not a random one.

## Returns

`pdf`, `logpdf`, `cdf` and `logcdf` return a `Series` of the shape of `x`; `sample` returns a `Series` of the requested shape. 

## Errors

- Parameters outside their valid range are refused with an error that names the requirement (`s` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Anglit on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({-0.7, -0.4, 0.0, 0.2, 0.5, 0.7}, f64);
  const std::vector<double> params = {};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Anglit, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Anglit, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Anglit, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -0.70000 -0.40000 0.00000 0.20000 0.50000 0.70000
pdf: 0.16997 0.69671 1.00000 0.92106 0.54030 0.16997
cdf: 0.00728 0.14132 0.50000 0.69471 0.92074 0.99272
logpdf: -1.77215 -0.36139 0.00000 -0.08223 -0.61563 -1.77215
```

The density is largest on this grid at x = 0, where it is 1; the distribution function runs from 0.00728 at x = -0.7 to 0.99272 at x = 0.7, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(1) = 0 at x = 0.

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
  const std::vector<double> params = {};

  // The three ways to the probability that the variable lies in [-0.6, 0.6]:
  const auto grid = pluss::series::linspace(-0.6, 0.6, 3001, f64);
  const double area = distr::pdf(dst::Anglit, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({-0.6, 0.6}, f64);
  const auto c = distr::cdf(dst::Anglit, edges, params).host<double>();

  const auto draws = distr::sample(dst::Anglit, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= -0.6).to(pluss::ScalarType::Float64) * (draws <= 0.6).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(0.6) - cdf(-0.6): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.93204
cdf(0.6) - cdf(-0.6): 0.93204
fraction of 200000 draws in range: 0.93145
```

The integral of the density (0.93204) and the difference of the distribution function (0.93204) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.93145) differs from it by 5.90e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 5.63e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({-1.5, -0.7, 0.0, 0.7, 1.5}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "s pi/4:";
  for (double v : distr::pdf(dst::Anglit, x, {0.0, 0.7853981633974483}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "s 1   :";
  for (double v : distr::pdf(dst::Anglit, x, {0.0, 1.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "s 2   :";
  for (double v : distr::pdf(dst::Anglit, x, {0.0, 2.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -1.5000 -0.7000 0.0000 0.7000 1.5000
s pi/4: 0.0000 0.1700 1.0000 0.1700 0.0000
s 1   : 0.0000 0.3566 0.7854 0.3566 0.0000
s 2   : 0.1503 0.3348 0.3927 0.3348 0.1503
```

Read down the rows: `s pi/4` peaks on this grid at x = 0 (value 1); `s 1` peaks on this grid at x = 0 (value 0.7854); `s 2` peaks on this grid at x = 0 (value 0.3927). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 checks the distribution function against its closed form, `(1 + sin(2x))/2` on the standard support.

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
  const auto x = pluss::make_series({-0.7, -0.4, 0.0, 0.2, 0.5, 0.7}, f64);
  const std::vector<double> params = {};

  const auto formula = (x * 2.0).sin() * 0.5 + 0.5;
  const auto library = distr::cdf(dst::Anglit, x, params);

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
x: -0.700000 -0.400000 0.000000 0.200000 0.500000 0.700000
formula: 0.007275 0.141322 0.500000 0.694709 0.920735 0.992725
distr::cdf: 0.007275 0.141322 0.500000 0.694709 0.920735 0.992725
largest difference: 1.11e-16
```

The formula and `distr::cdf` agree to 1.11e-16 over the grid, so the library implements exactly the closed form quoted above; the same expression is the quickest way to check a parameterisation before relying on it.

## See Also

- [`Cosine`](../continuous_bounded/Cosine.md)
- [`HalfCosine`](../continuous_bounded/HalfCosine.md)
- [`Quadratic`](../continuous_bounded/Quadratic.md)
- [`Semicircle`](../continuous_bounded/Semicircle.md)
