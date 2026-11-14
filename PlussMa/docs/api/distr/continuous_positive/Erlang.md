<!-- generated from Erlang.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Erlang`

The Erlang distribution: the waiting time until the k-th event of a Poisson process, a gamma distribution with an integer shape.

## Signature

```cpp
pluss::Series distr::pdf(dst::Erlang, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Erlang, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Erlang, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Erlang, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Erlang, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Erlang distribution is the sum of `k` independent exponential waiting times, each with mean `scale`: the time until the `k`-th arrival in a queue. It is the gamma distribution with integer shape `k`, so its mean is `k × scale` and its variance `k × scale²`. It was introduced for telephone traffic and remains the standard model of stage-structured service times.

The kind-based functions take `{scale, k}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `scale` | > 0 | the mean of each exponential stage |
| `k` | positive integer | the number of stages |

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

- Parameters outside their valid range are refused with an error that names the requirement (`scale` must be positive and `k` a positive integer).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Erlang on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.5, 1.5, 3.0, 4.5, 7.0, 12.0}, f64);
  const std::vector<double> params = {1.5, 3.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Erlang, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Erlang, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Erlang, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.50000 1.50000 3.00000 4.50000 7.00000 12.00000
pdf: 0.02654 0.12263 0.18045 0.14936 0.06826 0.00716
cdf: 0.00482 0.08030 0.32332 0.57681 0.84432 0.98625
logpdf: -3.62917 -2.09861 -1.71232 -1.90139 -2.68439 -4.93973
```

The density is largest on this grid at x = 3, where it is 0.18045; the distribution function runs from 0.00482 at x = 0.5 to 0.98625 at x = 12, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.18045) = -1.7123 at x = 3.

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
  const std::vector<double> params = {1.5, 3.0};

  // The three ways to the probability that the variable lies in [1, 8]:
  const auto grid = pluss::series::linspace(1.0, 8.0, 3001, f64);
  const double area = distr::pdf(dst::Erlang, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({1.0, 8.0}, f64);
  const auto c = distr::cdf(dst::Erlang, edges, params).host<double>();

  const auto draws = distr::sample(dst::Erlang, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 1.0).to(pluss::ScalarType::Float64) * (draws <= 8.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(8) - cdf(1): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.87055
cdf(8) - cdf(1): 0.87055
fraction of 200000 draws in range: 0.86920
```

The integral of the density (0.87055) and the difference of the distribution function (0.87055) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.8692) differs from it by 0.00135, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 7.51e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.5, 1.5, 3.0, 4.5, 7.0, 12.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "k 1:";
  for (double v : distr::pdf(dst::Erlang, x, {1.5, 1.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "k 3:";
  for (double v : distr::pdf(dst::Erlang, x, {1.5, 3.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "k 8:";
  for (double v : distr::pdf(dst::Erlang, x, {1.5, 8.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.5000 1.5000 3.0000 4.5000 7.0000 12.0000
k 1: 0.4777 0.2453 0.0902 0.0332 0.0063 0.0002
k 3: 0.0265 0.1226 0.1804 0.1494 0.0683 0.0072
k 8: 0.0000 0.0000 0.0023 0.0144 0.0600 0.0931
```

Read down the rows: `k 1` peaks on this grid at x = 0.5 (value 0.4777); `k 3` peaks on this grid at x = 3 (value 0.1804); `k 8` peaks on this grid at x = 12 (value 0.0931). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 confirms that an Erlang distribution with scale 1.5 and 3 stages is the gamma distribution with shape 3 and scale 1.5.

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
  const auto x = pluss::make_series({0.5, 1.5, 3.0, 6.0, 10.0}, f64);
  const std::vector<double> here = {1.5, 3.0};
  const std::vector<double> there = {0.0, 1.5, 3.0};

  const double dpdf = (distr::pdf(dst::Erlang, x, here) - distr::pdf(dst::Gamma, x, there)).abs().max().item<double>();
  const double dcdf = (distr::cdf(dst::Erlang, x, here) - distr::cdf(dst::Gamma, x, there)).abs().max().item<double>();
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

- [`Gamma`](../continuous_positive/Gamma.md)
- [`Exponential`](../continuous_positive/Exponential.md)
- [`ChiSquare`](../continuous_positive/ChiSquare.md)
- [`Poisson`](../discrete/Poisson.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ChiSquareNoncentral.html">
    <span class="label">Previous</span>
    <span class="title">ChiSquareNoncentral</span>
  </a>
  <a class="next-link" href="Exponential.html">
    <span class="label">Next</span>
    <span class="title">Exponential</span>
  </a>
</div>
<!-- apiref:nav:end -->
