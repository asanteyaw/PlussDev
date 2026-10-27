<!-- generated from Yule.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Yule`

The Yule–Simon distribution: a discrete power-law distribution with a heavy tail, the model of preferential attachment.

## Signature

```cpp
pluss::Series distr::pdf(dst::Yule, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Yule, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Yule, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Yule, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Yule, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Yule–Simon distribution on the positive integers has the mass function `(p - 1) B(k, p)`, with `B` the beta function, and a tail that decays as a power, `k^(-p)`. It is the stationary distribution of the sizes in a preferential-attachment process (the rich get richer): the number of species per genus, the number of links to a web page, word frequencies. The mean is finite only for `p > 2`.

The kind-based functions take `{p}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `p` | > 1 | the tail exponent: the mass of `k` decays like `k^(-p)` |

Support: the integers 1, 2, 3, ....

## Parameters

`const pluss::Series& x`
: The points at which to evaluate, a `Series` of any shape; the result has the same shape. The mass function is zero at a value that is not a possible count.

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

- Parameters outside their valid range are refused with an error that names the requirement (`p` must be greater than 1).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Yule on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 5.0, 10.0}, f64);
  const std::vector<double> params = {3.0};

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).host<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pmf:";
  for (double v : distr::pdf(dst::Yule, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Yule, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpmf:";
  for (double v : distr::logpdf(dst::Yule, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0 1 2 3 5 10
pmf: 0.00000 0.66667 0.16667 0.06667 0.01905 0.00303
cdf: 0.00000 0.66667 0.83333 0.90000 0.95238 0.98485
logpmf: -inf -0.40547 -1.79176 -2.70805 -3.96081 -5.79909
```

The mass is largest on this grid at x = 1, where it is 0.66667; the distribution function runs from 0 at x = 0 to 0.98485 at x = 10, and it is the running total of the mass. `logpdf` is the logarithm of the mass, for example log(0.66667) = -0.40546 at x = 1; where the density is zero (outside the support) the logarithm is `-inf`.

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

  // The three ways to the probability that the variable lies in [1, 10]:
  const auto grid = pluss::series::linspace(1.0, 10.0, 10, f64).round();
  const double mass_sum = distr::pdf(dst::Yule, grid, params).sum().item<double>();

  const auto edges = pluss::make_series({0.0, 10.0}, f64);
  const auto c = distr::cdf(dst::Yule, edges, params).host<double>();

  const auto draws = distr::sample(dst::Yule, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 1.0).to(pluss::ScalarType::Float64) * (draws <= 10.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "sum of the masses at 1..10: " << mass_sum << '\n';
  std::cout << "cdf(10) - cdf(0): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
sum of the masses at 1..10: 0.98485
cdf(10) - cdf(0): 0.98485
fraction of 200000 draws in range: 0.98504
```

The sum of the masses (0.98485) and the difference of the distribution function (0.98485) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.98504) differs from it by 1.90e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 2.73e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 6.0, 10.0}, f64);

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).host<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "p 1.5:";
  for (double v : distr::pdf(dst::Yule, x, {1.5}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "p 3  :";
  for (double v : distr::pdf(dst::Yule, x, {3.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "p 6  :";
  for (double v : distr::pdf(dst::Yule, x, {6.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 1 2 3 4 6 10
p 1.5: 0.3333 0.1333 0.0762 0.0508 0.0284 0.0135
p 3  : 0.6667 0.1667 0.0667 0.0333 0.0119 0.0030
p 6  : 0.8333 0.1190 0.0298 0.0099 0.0018 0.0002
```

Read down the rows: `p 1.5` peaks on this grid at x = 1 (value 0.3333); `p 3` peaks on this grid at x = 1 (value 0.6667); `p 6` peaks on this grid at x = 1 (value 0.8333). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

## See Also

- [`distr::Yule` object](../object_based/Yule.md) -- the object form, with exact cumulants below the tail index
- [`Zeta`](../discrete/Zeta.md)
- [`Geometric`](../discrete/Geometric.md)
- [`Pareto`](../continuous_positive/Pareto.md)
