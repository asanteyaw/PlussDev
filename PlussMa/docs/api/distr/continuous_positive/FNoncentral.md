<!-- generated from FNoncentral.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::FNoncentral`

The noncentral F distribution: the distribution of the F statistic when the numerator has a noncentral chi-squared distribution.

## Signature

```cpp
pluss::Series distr::pdf(dst::FNoncentral, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::FNoncentral, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::FNoncentral, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::FNoncentral, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::FNoncentral, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The noncentral F distribution is the ratio of a noncentral chi-squared variable (divided by `df1`) to an independent central chi-squared variable (divided by `df2`). It is the distribution of an F statistic when the null hypothesis is false, and so the basis of the power of the F test and of the analysis of variance.

The kind-based functions take `{lambda, df1, df2}`: the noncentrality first.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `lambda` | >= 0 | the noncentrality of the numerator |
| `df1` | > 0 | the numerator degrees of freedom |
| `df2` | > 0 | the denominator degrees of freedom |

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

- Parameters outside their valid range are refused with an error that names the requirement (`df1` and `df2` must be positive and `lambda` non-negative).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the FNoncentral on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.25, 0.5, 1.0, 2.0, 4.0, 8.0}, f64);
  const std::vector<double> params = {2.0, 5.0, 10.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::FNoncentral, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::FNoncentral, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::FNoncentral, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.25000 0.50000 1.00000 2.00000 4.00000 8.00000
pdf: 0.27239 0.44731 0.45875 0.22786 0.04583 0.00382
cdf: 0.03217 0.12553 0.36395 0.70412 0.92817 0.99120
logpdf: -1.30051 -0.80450 -0.77925 -1.47902 -3.08274 -5.56838
```

The density is largest on this grid at x = 1, where it is 0.45875; the distribution function runs from 0.03217 at x = 0.25 to 0.9912 at x = 8, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.45875) = -0.77925 at x = 1.

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
  const std::vector<double> params = {2.0, 5.0, 10.0};

  // The three ways to the probability that the variable lies in [0.5, 5]:
  const auto grid = pluss::series::linspace(0.5, 5.0, 3001, f64);
  const double area = distr::pdf(dst::FNoncentral, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({0.5, 5.0}, f64);
  const auto c = distr::cdf(dst::FNoncentral, edges, params).host<double>();

  const auto draws = distr::sample(dst::FNoncentral, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 0.5).to(pluss::ScalarType::Float64) * (draws <= 5.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(5) - cdf(0.5): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.83526
cdf(5) - cdf(0.5): 0.83526
fraction of 200000 draws in range: 0.83684
```

The integral of the density (0.83526) and the difference of the distribution function (0.83526) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.83684) differs from it by 0.00158, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 8.29e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.25, 0.5, 1.0, 2.0, 4.0, 8.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 0.5:";
  for (double v : distr::pdf(dst::FNoncentral, x, {0.5, 5.0, 10.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 3  :";
  for (double v : distr::pdf(dst::FNoncentral, x, {3.0, 5.0, 10.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 10 :";
  for (double v : distr::pdf(dst::FNoncentral, x, {10.0, 5.0, 10.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.2500 0.5000 1.0000 2.0000 4.0000 8.0000
lambda 0.5: 0.4532 0.6209 0.4927 0.1813 0.0275 0.0018
lambda 3  : 0.1930 0.3541 0.4227 0.2493 0.0587 0.0055
lambda 10 : 0.0158 0.0557 0.1568 0.2405 0.1361 0.0248
```

Read down the rows: `lambda 0.5` peaks on this grid at x = 0.5 (value 0.6209); `lambda 3` peaks on this grid at x = 1 (value 0.4227); `lambda 10` peaks on this grid at x = 2 (value 0.2405). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

## See Also

- [`F`](../continuous_positive/F.md)
- [`ChiSquareNoncentral`](../continuous_positive/ChiSquareNoncentral.md)
