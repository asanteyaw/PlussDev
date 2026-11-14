<!-- generated from Bernoulli.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Bernoulli`

The Bernoulli distribution: a single trial with two outcomes, success with probability p and failure otherwise.

## Signature

```cpp
pluss::Series distr::pdf(dst::Bernoulli, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Bernoulli, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Bernoulli, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Bernoulli, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Bernoulli, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Bernoulli distribution is the simplest random variable: it takes the value 1 (success) with probability `p` and 0 (failure) with probability `1 - p`. It models a single coin flip, a yes/no answer or the indicator of an event, and a sum of independent Bernoulli variables is binomial. Its mean is `p` and its variance `p (1 - p)`, largest at `p = 1/2`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `p` | 0 <= p <= 1 | the probability of success |

Support: the two values 0 and 1.

The object form of this family, with the same functions as methods, is on its own page: [`distr::Bernoulli`](../object_based/Bernoulli.md).

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

- Parameters outside their valid range are refused with an error that names the requirement (`p` must lie in [0, 1]).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Bernoulli on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({-1.0, 0.0, 1.0, 2.0}, f64);
  const std::vector<double> params = {0.3};

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).items<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pmf:";
  for (double v : distr::pdf(dst::Bernoulli, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Bernoulli, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpmf:";
  for (double v : distr::logpdf(dst::Bernoulli, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -1 0 1 2
pmf: 0.00000 0.70000 0.30000 0.00000
cdf: 0.00000 0.70000 1.00000 1.00000
logpmf: -inf -0.35667 -1.20397 -inf
```

The mass is largest on this grid at x = 0, where it is 0.7; the distribution function runs from 0 at x = -1 to 1 at x = 2, and it is the running total of the mass. `logpdf` is the logarithm of the mass, for example log(0.7) = -0.35667 at x = 0; where the density is zero (outside the support) the logarithm is `-inf`.

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
  const std::vector<double> params = {0.3};

  // The three ways to the probability that the variable lies in [0, 1]:
  const auto grid = pluss::series::linspace(0.0, 1.0, 2, f64).round();
  const double mass_sum = as_f64(distr::pdf(dst::Bernoulli, grid, params).sum());

  const auto edges = pluss::make_series({-1.0, 1.0}, f64);
  const auto c = distr::cdf(dst::Bernoulli, edges, params).items<double>();

  const auto draws = distr::sample(dst::Bernoulli, params, {200000}, f64, 7ULL);
  const double fraction = as_f64(((draws >= 0.0).to(pluss::ScalarType::Float64) * (draws <= 1.0).to(pluss::ScalarType::Float64)).mean());

  std::cout << "sum of the masses at 0..1: " << mass_sum << '\n';
  std::cout << "cdf(1) - cdf(-1): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
sum of the masses at 0..1: 1.00000
cdf(1) - cdf(-1): 1.00000
fraction of 200000 draws in range: 1.00000
```

The sum of the masses (1) and the difference of the distribution function (1) agree to all printed digits: they are two computations of one probability. The fraction of random draws (1) differs from it by 0, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 2.24e-09), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.0, 1.0}, f64);

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).items<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "p 0.1:";
  for (double v : distr::pdf(dst::Bernoulli, x, {0.1}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "p 0.5:";
  for (double v : distr::pdf(dst::Bernoulli, x, {0.5}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "p 0.9:";
  for (double v : distr::pdf(dst::Bernoulli, x, {0.9}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0 1
p 0.1: 0.9000 0.1000
p 0.5: 0.5000 0.5000
p 0.9: 0.1000 0.9000
```

Read down the rows: `p 0.1` peaks on this grid at x = 0 (value 0.9); `p 0.5` peaks on this grid at x = 0 (value 0.5); `p 0.9` peaks on this grid at x = 1 (value 0.9). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 confirms that the Bernoulli distribution is the binomial distribution with one trial.

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
  const auto x = pluss::make_series({0.0, 1.0}, f64);
  const std::vector<double> here = {0.3};
  const std::vector<double> there = {1.0, 0.3};

  const double dpdf = as_f64((distr::pdf(dst::Bernoulli, x, here) - distr::pdf(dst::Binomial, x, there)).abs().max());
  const double dcdf = as_f64((distr::cdf(dst::Bernoulli, x, here) - distr::cdf(dst::Binomial, x, there)).abs().max());
  std::cout << "largest difference in the density: " << dpdf << '\n';
  std::cout << "largest difference in the distribution function: " << dcdf << '\n';
}
```

Output

```text
largest difference in the density: 1.11e-16
largest difference in the distribution function: 0.00e+00
```

The two largest differences are 1.11e-16 for the density and 0 for the distribution function over the grid: zero up to rounding, so the two descriptions are the same distribution.

## See Also

- [`Binomial`](../discrete/Binomial.md)
- [`Geometric`](../discrete/Geometric.md)
- [`Rademacher`](../discrete/Rademacher.md)
- [`distr::Bernoulli`](../object_based/Bernoulli.md) -- the object form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Bessel.html">
    <span class="label">Next</span>
    <span class="title">Bessel</span>
  </a>
</div>
<!-- apiref:nav:end -->
