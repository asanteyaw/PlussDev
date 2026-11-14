<!-- generated from Bessel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Bessel`

The Bessel distribution: a discrete distribution whose masses are proportional to the terms of a modified Bessel function series.

## Signature

```cpp
pluss::Series distr::pdf(dst::Bessel, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Bessel, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Bessel, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Bessel, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Bessel, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The Bessel distribution on the non-negative integers has masses proportional to `(a/2)^(2k + nu) / (k! Gamma(k + nu + 1))`, the terms of the series of the modified Bessel function `I_nu(a)`, so the normalising constant is `I_nu(a)`. It arises as the distribution of the difference of two independent Poisson variables conditioned on their sum, and as the first-passage count of some random walks. Its mean grows with `a`, and a larger order `nu` shifts the mass to the right.

The kind-based functions take `{nu, a}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `nu` | > -1 | the order of the Bessel function |
| `a` | > 0 | the argument of the Bessel function; larger values move the mass to larger counts |

Support: the integers 0, 1, 2, ....

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

- Parameters outside their valid range are refused with an error that names the requirement (`a` must be positive and `nu` greater than -1).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Bessel on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 5.0, 8.0}, f64);
  const std::vector<double> params = {1.0, 4.0};

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).host<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pmf:";
  for (double v : distr::pdf(dst::Bessel, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Bessel, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpmf:";
  for (double v : distr::logpdf(dst::Bessel, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0 1 2 3 5 8
pmf: 0.20493 0.40986 0.27324 0.09108 0.00243 0.00000
cdf: 0.20493 0.61479 0.88803 0.97911 0.99975 1.00000
logpmf: -1.58509 -0.89194 -1.29741 -2.39602 -6.02036 -13.90117
```

The mass is largest on this grid at x = 1, where it is 0.40986; the distribution function runs from 0.20493 at x = 0 to 1 at x = 8, and it is the running total of the mass. `logpdf` is the logarithm of the mass, for example log(0.40986) = -0.89194 at x = 1.

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
  const std::vector<double> params = {1.0, 4.0};

  // The three ways to the probability that the variable lies in [0, 8]:
  const auto grid = pluss::series::linspace(0.0, 8.0, 9, f64).round();
  const double mass_sum = distr::pdf(dst::Bessel, grid, params).sum().item<double>();

  const auto edges = pluss::make_series({-1.0, 8.0}, f64);
  const auto c = distr::cdf(dst::Bessel, edges, params).host<double>();

  const auto draws = distr::sample(dst::Bessel, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 0.0).to(pluss::ScalarType::Float64) * (draws <= 8.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "sum of the masses at 0..8: " << mass_sum << '\n';
  std::cout << "cdf(8) - cdf(-1): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
sum of the masses at 0..8: 1.00000
cdf(8) - cdf(-1): 1.00000
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
  const auto x = pluss::make_series({0.0, 1.0, 2.0, 4.0, 6.0, 9.0, 12.0}, f64);

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).host<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "a 1:";
  for (double v : distr::pdf(dst::Bessel, x, {1.0, 1.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "a 4:";
  for (double v : distr::pdf(dst::Bessel, x, {1.0, 4.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "a 10:";
  for (double v : distr::pdf(dst::Bessel, x, {1.0, 10.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0 1 2 4 6 9 12
a 1: 0.8847 0.1106 0.0046 0.0000 0.0000 0.0000 0.0000
a 4: 0.2049 0.4099 0.2732 0.0182 0.0002 0.0000 0.0000
a 10: 0.0019 0.0234 0.0975 0.2539 0.1259 0.0054 0.0000
```

Read down the rows: `a 1` peaks on this grid at x = 0 (value 0.8847); `a 4` peaks on this grid at x = 1 (value 0.4099); `a 10` peaks on this grid at x = 4 (value 0.2539). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

## See Also

- [`Poisson`](../discrete/Poisson.md)
- [`Skellam`](../discrete/Skellam.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Bernoulli.html">
    <span class="label">Previous</span>
    <span class="title">Bernoulli</span>
  </a>
  <a class="next-link" href="Binomial.html">
    <span class="label">Next</span>
    <span class="title">Binomial</span>
  </a>
</div>
<!-- apiref:nav:end -->
