<!-- generated from ExtremeValue.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::ExtremeValue`

The extreme value (Gumbel) distribution: the distribution of the largest of many independent observations of a variable with an exponential-type tail.

## Signature

```cpp
pluss::Series distr::pdf(dst::ExtremeValue, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::ExtremeValue, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::ExtremeValue, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::ExtremeValue, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::ExtremeValue, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The extreme value distribution of maxima, also called the Gumbel distribution, is the limit law of the maximum of a large number of independent draws from a distribution with an exponential-type tail (the normal, the exponential, the gamma). It is the model of annual flood peaks, maximum wind speeds and record temperatures. Its distribution function is `exp(-exp(-(x - location)/scale))`, and its density is skewed to the right, with a longer upper tail.

The kind-based functions take `{location, scale}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `location` | any real | the mode of the distribution |
| `scale` | > 0 | the scale; the standard deviation is `scale pi / sqrt(6)` |

Support: all real numbers.

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

- Parameters outside their valid range are refused with an error that names the requirement (`scale` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the ExtremeValue on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0, 7.0}, f64);
  const std::vector<double> params = {0.0, 1.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::ExtremeValue, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::ExtremeValue, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::ExtremeValue, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -3.00000 -1.00000 0.00000 1.00000 3.00000 7.00000
pdf: 0.00000 0.17937 0.36788 0.25465 0.04737 0.00091
cdf: 0.00000 0.06599 0.36788 0.69220 0.95143 0.99909
logpdf: -17.08554 -1.71828 -1.00000 -1.36788 -3.04979 -7.00091
```

The density is largest on this grid at x = 0, where it is 0.36788; the distribution function runs from 0 at x = -3 to 0.99909 at x = 7, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.36788) = -1 at x = 0.

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
  const std::vector<double> params = {0.0, 1.0};

  // The three ways to the probability that the variable lies in [-1, 5]:
  const auto grid = pluss::series::linspace(-1.0, 5.0, 3001, f64);
  const double area = distr::pdf(dst::ExtremeValue, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({-1.0, 5.0}, f64);
  const auto c = distr::cdf(dst::ExtremeValue, edges, params).host<double>();

  const auto draws = distr::sample(dst::ExtremeValue, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= -1.0).to(pluss::ScalarType::Float64) * (draws <= 5.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(5) - cdf(-1): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.92730
cdf(5) - cdf(-1): 0.92730
fraction of 200000 draws in range: 0.92712
```

The integral of the density (0.9273) and the difference of the distribution function (0.9273) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.92712) differs from it by 1.80e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 5.81e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({-4.0, -1.0, 0.0, 2.0, 5.0, 9.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "scale 0.5:";
  for (double v : distr::pdf(dst::ExtremeValue, x, {0.0, 0.5}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "scale 1  :";
  for (double v : distr::pdf(dst::ExtremeValue, x, {0.0, 1.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "scale 2  :";
  for (double v : distr::pdf(dst::ExtremeValue, x, {0.0, 2.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -4.0000 -1.0000 0.0000 2.0000 5.0000 9.0000
scale 0.5: 0.0000 0.0091 0.7358 0.0360 0.0001 0.0000
scale 1  : 0.0000 0.1794 0.3679 0.1182 0.0067 0.0001
scale 2  : 0.0023 0.1585 0.1839 0.1273 0.0378 0.0055
```

Read down the rows: `scale 0.5` peaks on this grid at x = 0 (value 0.7358); `scale 1` peaks on this grid at x = 0 (value 0.3679); `scale 2` peaks on this grid at x = 0 (value 0.1839). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 checks the distribution function against its closed form, `exp(-exp(-(x - location)/scale))`.

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
  const auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0, 7.0}, f64);
  const std::vector<double> params = {0.0, 1.0};

  const auto formula = (-(-(x - 0.0) / 1.0).exp()).exp();
  const auto library = distr::cdf(dst::ExtremeValue, x, params);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "formula:";
  for (double v : formula.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "distr::cdf:";
  for (double v : library.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << std::scientific << std::setprecision(2) << "largest difference: " << (formula - library).abs().max().item<double>() << '\n';
}
```

Output

```text
x: -3.000000 -1.000000 0.000000 1.000000 3.000000 7.000000
formula: 0.000000 0.065988 0.367879 0.692201 0.951432 0.999089
distr::cdf: 0.000000 0.065988 0.367879 0.692201 0.951432 0.999089
largest difference: 5.55e-17
```

The formula and `distr::cdf` agree to 5.55e-17 over the grid, so the library implements exactly the closed form quoted above; the same expression is the quickest way to check a parameterisation before relying on it.

## See Also

- [`Weibull`](../continuous_positive/Weibull.md)
- [`Gompertz`](../continuous_positive/Gompertz.md)
- [`Normal`](../continuous_real_line/Normal.md)
- [`Logistic`](../continuous_real_line/Logistic.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Cauchy.html">
    <span class="label">Previous</span>
    <span class="title">Cauchy</span>
  </a>
  <a class="next-link" href="Gh.html">
    <span class="label">Next</span>
    <span class="title">Gh</span>
  </a>
</div>
<!-- apiref:nav:end -->
