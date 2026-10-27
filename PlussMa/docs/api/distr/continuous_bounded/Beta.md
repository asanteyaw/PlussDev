<!-- generated from Beta.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Beta`

The beta distribution: a flexible family on a bounded interval, the standard model of proportions and probabilities.

## Signature

```cpp
pluss::Series distr::pdf(dst::Beta, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Beta, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Beta, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Beta, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Beta, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The beta distribution on `[0, 1]` has the density proportional to `x^(a - 1)(1 - x)^(b - 1)` and models proportions, rates and probabilities: the share of a population, the success probability of a coin. Its mean is `a/(a + b)`. With `a = b = 1` it is uniform; with `a, b > 1` it is unimodal; with `a, b < 1` it is U-shaped, with the density unbounded at both ends; and `a ≠ b` gives skewness.

The kind-based functions take `{a, b}` on `[0, 1]` or `{low, high, a, b}` on `[low, high]`; the object `distr::Beta{low, high, alpha, beta}` fixes the support and its `MLE` estimates `alpha` and `beta`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `low` | low < high | the left end of the support (0 in the short form) |
| `high` | low < high | the right end of the support (1 in the short form) |
| `a` | > 0 | the first shape parameter: larger values move mass to the right |
| `b` | > 0 | the second shape parameter: larger values move mass to the left |

Support: the interval `[low, high]`, `[0, 1]` in the short form.

The object form of this family, with the same functions as methods, is on its own page: [`distr::Beta`](../object_based/Beta.md).

## Parameters

`const pluss::Series& x`
: The points at which to evaluate, a `Series` of any shape; the result has the same shape. Points outside the support have density zero and distribution value 0 or 1.

`const std::vector<double>& params`
: The parameters of the family, in the order shown in the table above (a shorter list selects the defaults: `{a, b}` gives the support `[0, 1]`). A list of the wrong length is refused.

`std::initializer_list<std::int64_t> sizes`
: The shape of the array of random draws, for example `{n}` or `{rows, columns}`.

`pluss::Config cfg`
: The dtype and platform of the draws; the default is Float32 on the CPU, so request Float64 when precision matters.

`std::uint64_t seed`
: The seed of the stream. The same seed always gives the same draws; a seed of 0 is a fixed default, not a random one.

## Returns

`pdf`, `logpdf`, `cdf` and `logcdf` return a `Series` of the shape of `x`; `sample` returns a `Series` of the requested shape.

## Errors

- Parameters outside their valid range are refused with an error that names the requirement (`a` and `b` must be positive and `low` less than `high`).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Beta on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.05, 0.15, 0.3, 0.5, 0.7, 0.95}, f64);
  const std::vector<double> params = {2.5, 4.0};

  std::cout << "x:";
  for (double v : x.pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Beta, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Beta, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Beta, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.05000 0.15000 0.30000 0.50000 0.70000 0.95000
pdf: 0.34599 1.28773 2.03427 1.59513 0.57075 0.00418
cdf: 0.00724 0.08990 0.35220 0.73611 0.95150 0.99995
logpdf: -1.06136 0.25288 0.71014 0.46696 -0.56081 -5.47802
```

The density is largest on this grid at x = 0.3, where it is 2.03427; the distribution function runs from 0.00724 at x = 0.05 to 0.99995 at x = 0.95, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(2.03427) = 0.71014 at x = 0.3.

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
  const std::vector<double> params = {2.5, 4.0};

  // The three ways to the probability that the variable lies in [0.1, 0.7]:
  const auto grid = pluss::series::linspace(0.1, 0.7, 3001, f64);
  const double area = as_f64(distr::pdf(dst::Beta, grid, params).antidiff(grid).trapezoid());

  const auto edges = pluss::make_series({0.1, 0.7}, f64);
  const auto c = distr::cdf(dst::Beta, edges, params).pull<double>();

  const auto draws = distr::sample(dst::Beta, params, {200000}, f64, 7ULL);
  const double fraction = as_f64(((draws >= 0.1).to(pluss::ScalarType::Float64) * (draws <= 0.7).to(pluss::ScalarType::Float64)).mean());

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(0.7) - cdf(0.1): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.91489
cdf(0.7) - cdf(0.1): 0.91489
fraction of 200000 draws in range: 0.91435
```

The integral of the density (0.91489) and the difference of the distribution function (0.91489) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.91435) differs from it by 5.40e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 6.24e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.05, 0.2, 0.4, 0.6, 0.8, 0.95}, f64);

  std::cout << "x:";
  for (double v : x.pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "a 2, b 2  :";
  for (double v : distr::pdf(dst::Beta, x, {2.0, 2.0}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "a 2, b 5  :";
  for (double v : distr::pdf(dst::Beta, x, {2.0, 5.0}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "a 5, b 2  :";
  for (double v : distr::pdf(dst::Beta, x, {5.0, 2.0}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "a 0.7, b 0.7:";
  for (double v : distr::pdf(dst::Beta, x, {0.7, 0.7}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.0500 0.2000 0.4000 0.6000 0.8000 0.9500
a 2, b 2  : 0.2850 0.9600 1.4400 1.4400 0.9600 0.2850
a 2, b 5  : 1.2218 2.4576 1.5552 0.4608 0.0384 0.0002
a 5, b 2  : 0.0002 0.0384 0.4608 1.5552 2.4576 1.2218
a 0.7, b 0.7: 1.3136 0.9125 0.8080 0.8080 0.9125 1.3136
```

Read down the rows: `a 2, b 2` peaks on this grid at x = 0.4 (value 1.44); `a 2, b 5` peaks on this grid at x = 0.2 (value 2.4576); `a 5, b 2` peaks on this grid at x = 0.8 (value 2.4576); `a 0.7, b 0.7` peaks on this grid at x = 0.05 (value 1.3136). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 confirms that the arcsine distribution is the beta distribution with both parameters equal to 1/2.

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
  const auto x = pluss::make_series({0.1, 0.3, 0.5, 0.8, 0.95}, f64);
  const std::vector<double> here = {0.5, 0.5};
  const std::vector<double> there = {};

  const double dpdf = as_f64((distr::pdf(dst::Beta, x, here) - distr::pdf(dst::Arcsin, x, there)).abs().max());
  const double dcdf = as_f64((distr::cdf(dst::Beta, x, here) - distr::cdf(dst::Arcsin, x, there)).abs().max());
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

- [`Uniform`](../continuous_bounded/Uniform.md)
- [`Arcsin`](../continuous_bounded/Arcsin.md)
- [`Bradford`](../continuous_bounded/Bradford.md)
- [`Triangular`](../continuous_bounded/Triangular.md)
- [`Binomial`](../discrete/Binomial.md)
- [`distr::Beta`](../object_based/Beta.md) -- the object form
