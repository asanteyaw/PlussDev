<!-- generated from NegativeBinomial.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::NegativeBinomial`

The negative binomial distribution: the number of failures before the r-th success, and the standard model of over-dispersed counts.

## Signature

```cpp
pluss::Series distr::pdf(dst::NegativeBinomial, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::NegativeBinomial, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::NegativeBinomial, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::NegativeBinomial, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::NegativeBinomial, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The negative binomial distribution counts the failures before the `r`-th success in independent trials with success probability `p`. Its variance, `r (1 - p)/p²`, *exceeds* its mean `r (1 - p)/p`, which makes it the standard model of over-dispersed counts (more variable than a Poisson): claim counts, species abundance, word frequencies. It is a gamma mixture of Poisson distributions, and tends to the Poisson distribution as `r` grows with the mean held fixed.

The kind-based functions take `{r, p}`; the object is `distr::NegativeBinomial{r, p}`. `r` need not be an integer.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `r` | > 0 | the number of successes (any positive real); larger values mean less over-dispersion |
| `p` | 0 < p < 1 | the success probability |

Support: the integers 0, 1, 2, ....

The object form of this family, with the same functions as methods, is on its own page: [`distr::NegativeBinomial`](../object_based/NegativeBinomial.md).

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

- Parameters outside their valid range are refused with an error that names the requirement (`r` must be positive and `p` lie in (0, 1)).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the NegativeBinomial on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.0, 2.0, 4.0, 6.0, 10.0, 20.0}, f64);
  const std::vector<double> params = {3.5, 0.4};

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).pull<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pmf:";
  for (double v : distr::pdf(dst::NegativeBinomial, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::NegativeBinomial, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpmf:";
  for (double v : distr::logpdf(dst::NegativeBinomial, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0 2 4 6 10 20
pmf: 0.04048 0.11475 0.12307 0.09415 0.03471 0.00098
cdf: 0.04048 0.24023 0.48953 0.69445 0.91249 0.99804
logpmf: -3.20702 -2.16498 -2.09498 -2.36286 -3.36058 -6.92663
```

The mass is largest on this grid at x = 4, where it is 0.12307; the distribution function runs from 0.04048 at x = 0 to 0.99804 at x = 20, and it is the running total of the mass. `logpdf` is the logarithm of the mass, for example log(0.12307) = -2.095 at x = 4.

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
  const std::vector<double> params = {3.5, 0.4};

  // The three ways to the probability that the variable lies in [1, 10]:
  const auto grid = pluss::series::linspace(1.0, 10.0, 10, f64).round();
  const double mass_sum = as_f64(distr::pdf(dst::NegativeBinomial, grid, params).sum());

  const auto edges = pluss::make_series({0.0, 10.0}, f64);
  const auto c = distr::cdf(dst::NegativeBinomial, edges, params).pull<double>();

  const auto draws = distr::sample(dst::NegativeBinomial, params, {200000}, f64, 7ULL);
  const double fraction = as_f64(((draws >= 1.0).to(pluss::ScalarType::Float64) * (draws <= 10.0).to(pluss::ScalarType::Float64)).mean());

  std::cout << "sum of the masses at 1..10: " << mass_sum << '\n';
  std::cout << "cdf(10) - cdf(0): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
sum of the masses at 1..10: 0.87201
cdf(10) - cdf(0): 0.87201
fraction of 200000 draws in range: 0.87213
```

The sum of the masses (0.87201) and the difference of the distribution function (0.87201) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.87213) differs from it by 1.20e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 7.47e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.0, 2.0, 4.0, 6.0, 10.0, 20.0, 30.0}, f64);

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).pull<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "r 1  :";
  for (double v : distr::pdf(dst::NegativeBinomial, x, {1.0, 0.4}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "r 3.5:";
  for (double v : distr::pdf(dst::NegativeBinomial, x, {3.5, 0.4}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "r 20 :";
  for (double v : distr::pdf(dst::NegativeBinomial, x, {20.0, 0.4}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0 2 4 6 10 20 30
r 1  : 0.4000 0.1440 0.0518 0.0187 0.0024 0.0000 0.0000
r 3.5: 0.0405 0.1148 0.1231 0.0942 0.0347 0.0010 0.0000
r 20 : 0.0000 0.0000 0.0000 0.0001 0.0013 0.0277 0.0458
```

Read down the rows: `r 1` peaks on this grid at x = 0 (value 0.4); `r 3.5` peaks on this grid at x = 4 (value 0.1231); `r 20` peaks on this grid at x = 30 (value 0.0458). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

## See Also

- [`Poisson`](../discrete/Poisson.md)
- [`Geometric`](../discrete/Geometric.md)
- [`Binomial`](../discrete/Binomial.md)
- [`Gamma`](../continuous_positive/Gamma.md)
- [`distr::NegativeBinomial`](../object_based/NegativeBinomial.md) -- the object form
