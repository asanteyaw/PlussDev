<!-- generated from Lognormal.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Lognormal`

The lognormal distribution: the distribution of a variable whose logarithm is normal, the standard model of multiplicative growth.

## Signature

```cpp
pluss::Series distr::pdf(dst::Lognormal, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Lognormal, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Lognormal, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Lognormal, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Lognormal, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

If the logarithm of a positive variable is normal, the variable is lognormal. It arises whenever many small independent *percentage* changes multiply together (incomes, prices, particle sizes, survival times), and it is positively skewed, with a long right tail. `mean_log` and `sd_log` are the mean and standard deviation of the *logarithm*, not of the variable itself: the median is `exp(mean_log)` and the mean is `exp(mean_log + sd_log²/2)`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `mean_log` | any real | the mean of the logarithm of the variable; the median is `exp(mean_log)` |
| `sd_log` | > 0 | the standard deviation of the logarithm; larger values give a heavier right tail |

Support: `x > 0`.

The object form of this family, with the same functions as methods, is on its own page: [`distr::Lognormal`](../object_based/Lognormal.md).

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

- Parameters outside their valid range are refused with an error that names the requirement (`sd_log` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Lognormal on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.2, 0.5, 1.0, 1.5, 3.0, 6.0}, f64);
  const std::vector<double> params = {0.0, 0.5};

  std::cout << "x:";
  for (double v : x.pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Lognormal, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Lognormal, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Lognormal, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.20000 0.50000 1.00000 1.50000 3.00000 6.00000
pdf: 0.02244 0.61046 0.79788 0.38287 0.02379 0.00022
cdf: 0.00064 0.08283 0.50000 0.79130 0.98600 0.99983
logpdf: -3.79693 -0.49355 -0.22579 -0.96006 -3.73830 -8.43835
```

The density is largest on this grid at x = 1, where it is 0.79788; the distribution function runs from 6.40e-04 at x = 0.2 to 0.99983 at x = 6, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.79788) = -0.2258 at x = 1.

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
  const std::vector<double> params = {0.0, 0.5};

  // The three ways to the probability that the variable lies in [0.5, 3]:
  const auto grid = pluss::series::linspace(0.5, 3.0, 3001, f64);
  const double area = as_f64(distr::pdf(dst::Lognormal, grid, params).antidiff(grid).trapezoid());

  const auto edges = pluss::make_series({0.5, 3.0}, f64);
  const auto c = distr::cdf(dst::Lognormal, edges, params).pull<double>();

  const auto draws = distr::sample(dst::Lognormal, params, {200000}, f64, 7ULL);
  const double fraction = as_f64(((draws >= 0.5).to(pluss::ScalarType::Float64) * (draws <= 3.0).to(pluss::ScalarType::Float64)).mean());

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(3) - cdf(0.5): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.90317
cdf(3) - cdf(0.5): 0.90317
fraction of 200000 draws in range: 0.90230
```

The integral of the density (0.90317) and the difference of the distribution function (0.90317) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.9023) differs from it by 8.70e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 6.61e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  std::cout << "sd_log 0.25:";
  for (double v : distr::pdf(dst::Lognormal, x, {0.0, 0.25}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "sd_log 0.5 :";
  for (double v : distr::pdf(dst::Lognormal, x, {0.0, 0.5}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "sd_log 1   :";
  for (double v : distr::pdf(dst::Lognormal, x, {0.0, 1.0}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.2500 0.5000 1.0000 2.0000 4.0000
sd_log 0.25: 0.0000 0.0683 1.5958 0.0171 0.0000
sd_log 0.5 : 0.0683 0.6105 0.7979 0.1526 0.0043
sd_log 1   : 0.6105 0.6275 0.3989 0.1569 0.0382
```

Read down the rows: `sd_log 0.25` peaks on this grid at x = 1 (value 1.5958); `sd_log 0.5` peaks on this grid at x = 1 (value 0.7979); `sd_log 1` peaks on this grid at x = 0.5 (value 0.6275). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 checks the distribution function against its definition: the normal distribution function of the logarithm, `Phi((log(x) - mean_log)/sd_log)`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  std::cout << std::fixed << std::setprecision(6);
  const auto x = pluss::make_series({0.2, 0.5, 1.0, 1.5, 3.0, 6.0}, f64);
  const std::vector<double> params = {0.0, 0.5};

  const auto formula = distr::cdf(dst::Normal, x.log(), {0.0, 0.5});
  const auto library = distr::cdf(dst::Lognormal, x, params);

  std::cout << "x:";
  for (double v : x.pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "formula:";
  for (double v : formula.pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "distr::cdf:";
  for (double v : library.pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << std::scientific << std::setprecision(2) << "largest difference: " << as_f64((formula - library).abs().max()) << '\n';
}
```

Output

```text
x: 0.200000 0.500000 1.000000 1.500000 3.000000 6.000000
formula: 0.000643 0.082829 0.500000 0.791297 0.985998 0.999831
distr::cdf: 0.000643 0.082829 0.500000 0.791297 0.985998 0.999831
largest difference: 0.00e+00
```

The formula and `distr::cdf` agree to 0 over the grid, so the library implements exactly the closed form quoted above; the same expression is the quickest way to check a parameterisation before relying on it.

## See Also

- [`Normal`](../continuous_real_line/Normal.md)
- [`Gamma`](../continuous_positive/Gamma.md)
- [`Weibull`](../continuous_positive/Weibull.md)
- [`InverseGaussian`](../continuous_positive/InverseGaussian.md)
- [`distr::Lognormal`](../object_based/Lognormal.md) -- the object form
