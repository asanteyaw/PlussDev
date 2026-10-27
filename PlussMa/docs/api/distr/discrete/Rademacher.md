<!-- generated from Rademacher.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Rademacher`

The Rademacher distribution: the values +1 and -1, each with probability one half.

## Signature

```cpp
pluss::Series distr::pdf(dst::Rademacher, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Rademacher, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Rademacher, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Rademacher, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Rademacher, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Rademacher distribution takes the values -1 and +1 with equal probability: the random sign, the step of a symmetric random walk, the coin flip with outcomes recoded to plus and minus one. It has mean 0 and variance 1, and sums of independent Rademacher variables approach the normal distribution by the central limit theorem.

The kind-based functions take `{}`: the distribution has no parameters.

Support: the two values -1 and +1.

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

- Parameters outside their valid range are refused with an error that names the requirement (the distribution has no parameters).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Rademacher on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}, f64);
  const std::vector<double> params = {};

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).host<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pmf:";
  for (double v : distr::pdf(dst::Rademacher, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Rademacher, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpmf:";
  for (double v : distr::logpdf(dst::Rademacher, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -2 -1 0 1 2
pmf: 0.00000 0.50000 0.00000 0.50000 0.00000
cdf: 0.00000 0.50000 0.50000 1.00000 1.00000
logpmf: -inf -0.69315 -inf -0.69315 -inf
```

The mass is largest on this grid at x = -1, where it is 0.5; the distribution function runs from 0 at x = -2 to 1 at x = 2, and it is the running total of the mass. `logpdf` is the logarithm of the mass, for example log(0.5) = -0.69315 at x = -1; where the density is zero (outside the support) the logarithm is `-inf`.

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

  // The three ways to the probability that the variable lies in [-1, 1]:
  const auto grid = pluss::series::linspace(-1.0, 1.0, 3, f64).round();
  const double mass_sum = distr::pdf(dst::Rademacher, grid, params).sum().item<double>();

  const auto edges = pluss::make_series({-2.0, 1.0}, f64);
  const auto c = distr::cdf(dst::Rademacher, edges, params).host<double>();

  const auto draws = distr::sample(dst::Rademacher, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= -1.0).to(pluss::ScalarType::Float64) * (draws <= 1.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "sum of the masses at -1..1: " << mass_sum << '\n';
  std::cout << "cdf(1) - cdf(-2): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
sum of the masses at -1..1: 1.00000
cdf(1) - cdf(-2): 1.00000
fraction of 200000 draws in range: 1.00000
```

The sum of the masses (1) and the difference of the distribution function (1) agree to all printed digits: they are two computations of one probability. The fraction of random draws (1) differs from it by 0, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 2.24e-09), so the density, the distribution function and the sampler describe the same distribution.

## See Also

- [`Bernoulli`](../discrete/Bernoulli.md)
- [`Binomial`](../discrete/Binomial.md)
- [`Normal`](../continuous_real_line/Normal.md)
