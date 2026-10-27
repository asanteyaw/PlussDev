<!-- generated from F.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::F`

The F distribution: the distribution of a ratio of two independent chi-squared variables, each divided by its degrees of freedom.

## Signature

```cpp
pluss::Series distr::pdf(dst::F, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::F, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::F, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::F, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::F, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The F distribution with `df1` and `df2` degrees of freedom is the ratio of two independent chi-squared variables divided by their degrees of freedom, so it is the reference distribution for ratios of variance estimates: the analysis of variance, the test of a regression, and the comparison of two variances. Its mean is `df2/(df2 - 2)` for `df2 > 2`, and its right tail is heavy when `df2` is small.

The kind-based functions take `{df1, df2}`; the object is `distr::FDistribution{numerator, denominator}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `df1` | > 0 | the numerator degrees of freedom |
| `df2` | > 0 | the denominator degrees of freedom; the variance exists only for `df2 > 4` |

Support: `x >= 0`.

The object form of this family, with the same functions as methods, is on its own page: [`distr::FDistribution`](../object_based/FDistribution.md).

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

- Parameters outside their valid range are refused with an error that names the requirement (`df1` and `df2` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the F on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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
  const std::vector<double> params = {5.0, 10.0};

  std::cout << "x:";
  for (double v : x.pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::F, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::F, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::F, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.25000 0.50000 1.00000 2.00000 4.00000 8.00000
pdf: 0.53577 0.68761 0.49548 0.16201 0.02190 0.00134
cdf: 0.06964 0.22998 0.53488 0.83581 0.97032 0.99714
logpdf: -0.62405 -0.37454 -0.70223 -1.82012 -3.82139 -6.61286
```

The density is largest on this grid at x = 0.5, where it is 0.68761; the distribution function runs from 0.06964 at x = 0.25 to 0.99714 at x = 8, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.68761) = -0.37453 at x = 0.5.

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
  const std::vector<double> params = {5.0, 10.0};

  // The three ways to the probability that the variable lies in [0.5, 4]:
  const auto grid = pluss::series::linspace(0.5, 4.0, 3001, f64);
  const double area = as_f64(distr::pdf(dst::F, grid, params).antidiff(grid).trapezoid());

  const auto edges = pluss::make_series({0.5, 4.0}, f64);
  const auto c = distr::cdf(dst::F, edges, params).pull<double>();

  const auto draws = distr::sample(dst::F, params, {200000}, f64, 7ULL);
  const double fraction = as_f64(((draws >= 0.5).to(pluss::ScalarType::Float64) * (draws <= 4.0).to(pluss::ScalarType::Float64)).mean());

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(4) - cdf(0.5): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.74035
cdf(4) - cdf(0.5): 0.74035
fraction of 200000 draws in range: 0.73938
```

The integral of the density (0.74035) and the difference of the distribution function (0.74035) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.73938) differs from it by 9.70e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 9.80e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.25, 0.5, 1.0, 2.0, 4.0}, f64);

  std::cout << "x:";
  for (double v : x.pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "df2 5  :";
  for (double v : distr::pdf(dst::F, x, {5.0, 5.0}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "df2 10 :";
  for (double v : distr::pdf(dst::F, x, {5.0, 10.0}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "df2 100:";
  for (double v : distr::pdf(dst::F, x, {5.0, 100.0}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.2500 0.5000 1.0000 2.0000 4.0000
df2 5  : 0.5563 0.6323 0.4244 0.1581 0.0348
df2 10 : 0.5358 0.6876 0.4955 0.1620 0.0219
df2 100: 0.5023 0.7460 0.5955 0.1465 0.0043
```

Read down the rows: `df2 5` peaks on this grid at x = 0.5 (value 0.6323); `df2 10` peaks on this grid at x = 0.5 (value 0.6876); `df2 100` peaks on this grid at x = 0.5 (value 0.746). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

## See Also

- [`ChiSquare`](../continuous_positive/ChiSquare.md)
- [`StudentT`](../continuous_real_line/StudentT.md)
- [`FNoncentral`](../continuous_positive/FNoncentral.md)
- [`Beta`](../continuous_bounded/Beta.md)
- [`distr::FDistribution`](../object_based/FDistribution.md) -- the object form
