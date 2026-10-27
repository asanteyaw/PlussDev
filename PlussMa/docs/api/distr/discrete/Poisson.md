<!-- generated from Poisson.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Poisson`

The Poisson distribution: the number of events in a fixed interval when events occur independently at a constant average rate.

## Signature

```cpp
pluss::Series distr::pdf(dst::Poisson, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Poisson, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Poisson, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Poisson, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Poisson, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Poisson distribution counts rare, independent events: calls to a switchboard in a minute, defects in a length of wire, arrivals in an hour. It is described by one number, `lambda`, which is both its mean and its variance, so a count whose variance exceeds its mean is *over-dispersed* and is better described by the negative binomial distribution. It is the limit of the binomial distribution when the number of trials grows and the probability of success shrinks with their product held fixed.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `lambda` | > 0 | the average number of events; equal to the mean and the variance |

Support: the non-negative integers 0, 1, 2, ....

The object form of this family, with the same functions as methods, is on its own page: [`distr::Poisson`](../object_based/Poisson.md).

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

- Parameters outside their valid range are refused with an error that names the requirement (`lambda` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Poisson on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 6.0, 10.0}, f64);
  const std::vector<double> params = {3.2};

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).pull<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pmf:";
  for (double v : distr::pdf(dst::Poisson, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Poisson, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpmf:";
  for (double v : distr::logpdf(dst::Poisson, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0 1 2 3 4 6 10
pmf: 0.04076 0.13044 0.20870 0.22262 0.17809 0.06079 0.00126
cdf: 0.04076 0.17120 0.37990 0.60252 0.78061 0.95538 0.99950
logpmf: -3.20000 -2.03685 -1.56685 -1.50231 -1.72545 -2.80035 -6.67290
```

The mass is largest on this grid at x = 3, where it is 0.22262; the distribution function runs from 0.04076 at x = 0 to 0.9995 at x = 10, and it is the running total of the mass. `logpdf` is the logarithm of the mass, for example log(0.22262) = -1.50229 at x = 3.

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
  const std::vector<double> params = {3.2};

  // The three ways to the probability that the variable lies in [0, 6]:
  const auto grid = pluss::series::linspace(0.0, 6.0, 7, f64).round();
  const double mass_sum = as_f64(distr::pdf(dst::Poisson, grid, params).sum());

  const auto edges = pluss::make_series({-1.0, 6.0}, f64);
  const auto c = distr::cdf(dst::Poisson, edges, params).pull<double>();

  const auto draws = distr::sample(dst::Poisson, params, {200000}, f64, 7ULL);
  const double fraction = as_f64(((draws >= 0.0).to(pluss::ScalarType::Float64) * (draws <= 6.0).to(pluss::ScalarType::Float64)).mean());

  std::cout << "sum of the masses at 0..6: " << mass_sum << '\n';
  std::cout << "cdf(6) - cdf(-1): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
sum of the masses at 0..6: 0.95538
cdf(6) - cdf(-1): 0.95538
fraction of 200000 draws in range: 0.95616
```

The sum of the masses (0.95538) and the difference of the distribution function (0.95538) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.95616) differs from it by 7.80e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 4.62e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 6.0, 8.0, 12.0}, f64);

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).pull<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 0.5:";
  for (double v : distr::pdf(dst::Poisson, x, {0.5}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 3  :";
  for (double v : distr::pdf(dst::Poisson, x, {3.0}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "lambda 8  :";
  for (double v : distr::pdf(dst::Poisson, x, {8.0}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0 1 2 3 4 6 8 12
lambda 0.5: 0.6065 0.3033 0.0758 0.0126 0.0016 0.0000 0.0000 0.0000
lambda 3  : 0.0498 0.1494 0.2240 0.2240 0.1680 0.0504 0.0081 0.0001
lambda 8  : 0.0003 0.0027 0.0107 0.0286 0.0573 0.1221 0.1396 0.0481
```

Read down the rows: `lambda 0.5` peaks on this grid at x = 0 (value 0.6065); `lambda 3` peaks on this grid at x = 2 (value 0.224); `lambda 8` peaks on this grid at x = 8 (value 0.1396). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 shows the Poisson limit of the binomial: with 1000 trials and success probability `lambda/1000` the binomial mass function is within a few ten-thousandths of the Poisson one.

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
  const auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 6.0, 10.0}, f64);
  const std::vector<double> here = {3.2};
  const std::vector<double> there = {1000.0, 0.0032};

  const double dpdf = as_f64((distr::pdf(dst::Poisson, x, here) - distr::pdf(dst::Binomial, x, there)).abs().max());
  const double dcdf = as_f64((distr::cdf(dst::Poisson, x, here) - distr::cdf(dst::Binomial, x, there)).abs().max());
  std::cout << "largest difference in the density: " << dpdf << '\n';
  std::cout << "largest difference in the distribution function: " << dcdf << '\n';
}
```

Output

```text
largest difference in the density: 3.30e-04
largest difference in the distribution function: 4.60e-04
```

The two largest differences are 3.30e-04 for the density and 4.60e-04 for the distribution function over the grid: small and shrinking as the approximation improves, so the two descriptions agree to the accuracy shown.

## See Also

- [`Binomial`](../discrete/Binomial.md)
- [`NegativeBinomial`](../discrete/NegativeBinomial.md)
- [`Geometric`](../discrete/Geometric.md)
- [`Skellam`](../discrete/Skellam.md)
- [`distr::Poisson`](../object_based/Poisson.md) -- the object form
