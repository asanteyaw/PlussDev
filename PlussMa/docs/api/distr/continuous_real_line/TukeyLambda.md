<!-- generated from TukeyLambda.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::TukeyLambda`

The Tukey lambda distribution: a one-parameter family defined through its quantile function that contains the logistic, uniform and approximately the normal and heavy-tailed distributions.

## Signature

```cpp
pluss::Series distr::pdf(dst::TukeyLambda, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::TukeyLambda, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::TukeyLambda, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::TukeyLambda, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::TukeyLambda, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Tukey lambda distribution is defined by its quantile function `Q(p) = (p^lambda - (1 - p)^lambda)/lambda` (the logistic quantile function when `lambda = 0`). One parameter sweeps a whole range of symmetric shapes: `lambda = 0` is the logistic distribution, `lambda = 1` the uniform distribution on (-1, 1), `lambda = 2` the uniform on (-1/2, 1/2), values near 0.14 give a close approximation to the normal, and negative values give heavy tails. It is used to identify the shape of a symmetric distribution from data (the probability-plot correlation coefficient).

The kind-based functions take `{lambda}`; the support is bounded for `lambda > 0` (to `±1/lambda`) and the whole line otherwise.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `lambda` | any real | the shape: negative for heavy tails, 0 logistic, near 0.14 close to normal, positive for short tails and a bounded support |

Support: all real numbers when `lambda <= 0`; the interval `(-1/lambda, 1/lambda)` when `lambda > 0`.

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

- Parameters outside their valid range are refused with an error that names the requirement (no restriction on `lambda`).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the TukeyLambda on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({-4.0, -2.0, 0.0, 1.0, 2.5, 4.0}, f64);
  const std::vector<double> params = {0.14};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::TukeyLambda, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::TukeyLambda, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::TukeyLambda, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -4.00000 -2.00000 0.00000 1.00000 2.50000 4.00000
pdf: 0.00638 0.10591 0.27548 0.21631 0.06255 0.00638
cdf: 0.00282 0.08462 0.50000 0.75476 0.95693 0.99718
logpdf: -5.05510 -2.24517 -1.28925 -1.53106 -2.77176 -5.05510
```

The density is largest on this grid at x = 0, where it is 0.27548; the distribution function runs from 0.00282 at x = -4 to 0.99718 at x = 4, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.27548) = -1.28924 at x = 0.

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
  const std::vector<double> params = {0.14};

  // The three ways to the probability that the variable lies in [-3, 3]:
  const auto grid = pluss::series::linspace(-3.0, 3.0, 3001, f64);
  const double area = distr::pdf(dst::TukeyLambda, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({-3.0, 3.0}, f64);
  const auto c = distr::cdf(dst::TukeyLambda, edges, params).host<double>();

  const auto draws = distr::sample(dst::TukeyLambda, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= -3.0).to(pluss::ScalarType::Float64) * (draws <= 3.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(3) - cdf(-3): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.96053
cdf(3) - cdf(-3): 0.96053
fraction of 200000 draws in range: 0.96038
```

The integral of the density (0.96053) and the difference of the distribution function (0.96053) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.96038) differs from it by 1.50e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 4.35e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda -0.5:";
  for (double v : distr::pdf(dst::TukeyLambda, x, {-0.5}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 0    :";
  for (double v : distr::pdf(dst::TukeyLambda, x, {0.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 0.14 :";
  for (double v : distr::pdf(dst::TukeyLambda, x, {0.14}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 0.8  :";
  for (double v : distr::pdf(dst::TukeyLambda, x, {0.8}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -2.0000 -1.0000 0.0000 1.0000 2.0000
lambda -0.5: 0.0897 0.1430 0.1768 0.1430 0.0897
lambda 0    : 0.1050 0.1966 0.2500 0.1966 0.1050
lambda 0.14 : 0.1059 0.2163 0.2755 0.2163 0.1059
lambda 0.8  : 0.0040 0.3747 0.4353 0.3747 0.0040
```

Read down the rows: `lambda -0.5` peaks on this grid at x = 0 (value 0.1768); `lambda 0` peaks on this grid at x = 0 (value 0.25); `lambda 0.14` peaks on this grid at x = 0 (value 0.2755); `lambda 0.8` peaks on this grid at x = 0 (value 0.4353). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 confirms that the Tukey lambda distribution with `lambda = 0` is the standard logistic distribution.

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
  const auto x = pluss::make_series({-4.0, -1.0, 0.0, 1.0, 4.0}, f64);
  const std::vector<double> here = {0.0};
  const std::vector<double> there = {0.0, 1.0};

  const double dpdf = (distr::pdf(dst::TukeyLambda, x, here) - distr::pdf(dst::Logistic, x, there)).abs().max().item<double>();
  const double dcdf = (distr::cdf(dst::TukeyLambda, x, here) - distr::cdf(dst::Logistic, x, there)).abs().max().item<double>();
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

- [`Logistic`](../continuous_real_line/Logistic.md)
- [`Uniform`](../continuous_bounded/Uniform.md)
- [`Normal`](../continuous_real_line/Normal.md)
- [`Cauchy`](../continuous_real_line/Cauchy.md)
