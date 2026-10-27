<!-- generated from Burr.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Burr`

The Burr (type III) distribution: a flexible heavy-tailed distribution on the positive axis with two shape parameters.

## Signature

```cpp
pluss::Series distr::pdf(dst::Burr, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Burr, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Burr, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Burr, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Burr, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Burr type III distribution, also called the Dagum distribution, has the distribution function `(1 + x^(-c))^(-k)`. Its two shape parameters give a family of unimodal and decreasing densities with polynomial tails at both ends, which makes it a standard model of incomes and of losses in insurance. Heavier right tails come from a smaller `c`.

The kind-based functions take `{c, k}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `c` | > 0 | the tail shape: the right tail of the density decays like `x^(-c - 1)` |
| `k` | > 0 | the shape at the left end: the density near zero is proportional to `x^(c k - 1)` |

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

- Parameters outside their valid range are refused with an error that names the requirement (`c` and `k` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Burr on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.2, 0.5, 1.0, 1.5, 3.0, 8.0}, f64);
  const std::vector<double> params = {2.0, 3.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Burr, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Burr, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Burr, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.20000 0.50000 1.00000 1.50000 3.00000 8.00000
pdf: 0.00164 0.07680 0.37500 0.40839 0.14580 0.01101
cdf: 0.00006 0.00800 0.12500 0.33182 0.72900 0.95455
logpdf: -6.41231 -2.56655 -0.98083 -0.89553 -1.92552 -4.50858
```

The density is largest on this grid at x = 1.5, where it is 0.40839; the distribution function runs from 6.00e-05 at x = 0.2 to 0.95455 at x = 8, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.40839) = -0.89553 at x = 1.5.

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
  const std::vector<double> params = {2.0, 3.0};

  // The three ways to the probability that the variable lies in [0.4, 3]:
  const auto grid = pluss::series::linspace(0.4, 3.0, 3001, f64);
  const double area = distr::pdf(dst::Burr, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({0.4, 3.0}, f64);
  const auto c = distr::cdf(dst::Burr, edges, params).host<double>();

  const auto draws = distr::sample(dst::Burr, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 0.4).to(pluss::ScalarType::Float64) * (draws <= 3.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(3) - cdf(0.4): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.72638
cdf(3) - cdf(0.4): 0.72638
fraction of 200000 draws in range: 0.72525
```

The integral of the density (0.72638) and the difference of the distribution function (0.72638) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.72525) differs from it by 0.00113, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 9.97e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.2, 0.5, 1.0, 1.5, 3.0, 8.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "c 1.2:";
  for (double v : distr::pdf(dst::Burr, x, {1.2, 3.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "c 2  :";
  for (double v : distr::pdf(dst::Burr, x, {2.0, 3.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "c 5  :";
  for (double v : distr::pdf(dst::Burr, x, {5.0, 3.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.2000 0.5000 1.0000 1.5000 3.0000 8.0000
c 1.2: 0.0319 0.1399 0.2250 0.2170 0.1244 0.0270
c 2  : 0.0016 0.0768 0.3750 0.4084 0.1458 0.0110
c 5  : 0.0000 0.0008 0.9375 0.8029 0.0202 0.0001
```

Read down the rows: `c 1.2` peaks on this grid at x = 1 (value 0.225); `c 2` peaks on this grid at x = 1.5 (value 0.4084); `c 5` peaks on this grid at x = 1 (value 0.9375). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 checks the distribution function against its closed form, `(1 + x^(-c))^(-k)`.

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
  const auto x = pluss::make_series({0.2, 0.5, 1.0, 1.5, 3.0, 8.0}, f64);
  const std::vector<double> params = {2.0, 3.0};

  const auto formula = (x.pow(-2.0) + 1.0).pow(-3.0);
  const auto library = distr::cdf(dst::Burr, x, params);

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
x: 0.200000 0.500000 1.000000 1.500000 3.000000 8.000000
formula: 0.000057 0.008000 0.125000 0.331816 0.729000 0.954553
distr::cdf: 0.000057 0.008000 0.125000 0.331816 0.729000 0.954553
largest difference: 0.00e+00
```

The formula and `distr::cdf` agree to 0 over the grid, so the library implements exactly the closed form quoted above; the same expression is the quickest way to check a parameterisation before relying on it.

## See Also

- [`distr::Burr` object](../object_based/Burr.md) -- the object form, with exact cumulants below the tail index
- [`Pareto`](../continuous_positive/Pareto.md)
- [`Lomax`](../continuous_positive/Lomax.md)
- [`Weibull`](../continuous_positive/Weibull.md)
- [`Lognormal`](../continuous_positive/Lognormal.md)
