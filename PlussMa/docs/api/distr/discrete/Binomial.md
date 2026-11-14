<!-- generated from Binomial.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Binomial`

The binomial distribution: the number of successes in a fixed number of independent trials with the same success probability.

## Signature

```cpp
pluss::Series distr::pdf(dst::Binomial, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Binomial, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Binomial, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Binomial, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Binomial, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The binomial distribution with `n` trials and success probability `p` counts the successes in `n` independent yes/no trials: heads in `n` coin flips, defective items in a sample of `n`, responders among `n` patients. Its mean is `n p` and its variance `n p (1 - p)`, which is *smaller* than the mean, so counts that look under-dispersed relative to a Poisson are binomial. As `n` grows with `n p` fixed it approaches the Poisson distribution, and for large `n p (1 - p)` it approaches the normal.

The kind-based functions take `{n, p}`; the object is `distr::Binomial{count, p}`, whose `MLE` estimates `p` with `count` held fixed.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `n` | positive integer | the number of trials |
| `p` | 0 <= p <= 1 | the probability of success on each trial |

Support: the integers 0, 1, ..., n.

The object form of this family, with the same functions as methods, is on its own page: [`distr::Binomial`](../object_based/Binomial.md).

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

- Parameters outside their valid range are refused with an error that names the requirement (`n` must be a positive integer and `p` lie in [0, 1]).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Binomial on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.0, 2.0, 4.0, 6.0, 9.0, 12.0}, f64);
  const std::vector<double> params = {12.0, 0.35};

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).items<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pmf:";
  for (double v : distr::pdf(dst::Binomial, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Binomial, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpmf:";
  for (double v : distr::logpdf(dst::Binomial, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0 2 4 6 9 12
pmf: 0.00569 0.10885 0.23669 0.12810 0.00476 0.00000
cdf: 0.00569 0.15129 0.58335 0.91537 0.99915 1.00000
logpmf: -5.16939 -2.21782 -1.44099 -2.05492 -5.34712 -12.59787
```

The mass is largest on this grid at x = 4, where it is 0.23669; the distribution function runs from 0.00569 at x = 0 to 1 at x = 12, and it is the running total of the mass. `logpdf` is the logarithm of the mass, for example log(0.23669) = -1.441 at x = 4.

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
  const std::vector<double> params = {12.0, 0.35};

  // The three ways to the probability that the variable lies in [2, 6]:
  const auto grid = pluss::series::linspace(2.0, 6.0, 5, f64).round();
  const double mass_sum = as_f64(distr::pdf(dst::Binomial, grid, params).sum());

  const auto edges = pluss::make_series({1.0, 6.0}, f64);
  const auto c = distr::cdf(dst::Binomial, edges, params).items<double>();

  const auto draws = distr::sample(dst::Binomial, params, {200000}, f64, 7ULL);
  const double fraction = as_f64(((draws >= 2.0).to(pluss::ScalarType::Float64) * (draws <= 6.0).to(pluss::ScalarType::Float64)).mean());

  std::cout << "sum of the masses at 2..6: " << mass_sum << '\n';
  std::cout << "cdf(6) - cdf(1): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
sum of the masses at 2..6: 0.87293
cdf(6) - cdf(1): 0.87293
fraction of 200000 draws in range: 0.87276
```

The sum of the masses (0.87293) and the difference of the distribution function (0.87293) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.87276) differs from it by 1.70e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 7.45e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.0, 2.0, 4.0, 6.0, 8.0, 10.0, 12.0}, f64);

  std::cout << "x:";
  for (std::int64_t v : x.to(pluss::ScalarType::Int64).items<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "p 0.1:";
  for (double v : distr::pdf(dst::Binomial, x, {12.0, 0.1}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "p 0.35:";
  for (double v : distr::pdf(dst::Binomial, x, {12.0, 0.35}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "p 0.8:";
  for (double v : distr::pdf(dst::Binomial, x, {12.0, 0.8}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0 2 4 6 8 10 12
p 0.1: 0.2824 0.2301 0.0213 0.0005 0.0000 0.0000 0.0000
p 0.35: 0.0057 0.1088 0.2367 0.1281 0.0199 0.0008 0.0000
p 0.8: 0.0000 0.0000 0.0005 0.0155 0.1329 0.2835 0.0687
```

Read down the rows: `p 0.1` peaks on this grid at x = 0 (value 0.2824); `p 0.35` peaks on this grid at x = 4 (value 0.2367); `p 0.8` peaks on this grid at x = 10 (value 0.2835). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 confirms that a binomial distribution with one trial is the Bernoulli distribution.

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
  const std::vector<double> here = {1.0, 0.3};
  const std::vector<double> there = {0.3};

  const double dpdf = as_f64((distr::pdf(dst::Binomial, x, here) - distr::pdf(dst::Bernoulli, x, there)).abs().max());
  const double dcdf = as_f64((distr::cdf(dst::Binomial, x, here) - distr::cdf(dst::Bernoulli, x, there)).abs().max());
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

- [`Bernoulli`](../discrete/Bernoulli.md)
- [`Poisson`](../discrete/Poisson.md)
- [`NegativeBinomial`](../discrete/NegativeBinomial.md)
- [`Hypergeometric`](../discrete/Hypergeometric.md)
- [`Normal`](../continuous_real_line/Normal.md)
- [`distr::Binomial`](../object_based/Binomial.md) -- the object form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Bessel.html">
    <span class="label">Previous</span>
    <span class="title">Bessel</span>
  </a>
  <a class="next-link" href="Geometric.html">
    <span class="label">Next</span>
    <span class="title">Geometric</span>
  </a>
</div>
<!-- apiref:nav:end -->
