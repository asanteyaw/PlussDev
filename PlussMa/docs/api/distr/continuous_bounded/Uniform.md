<!-- generated from Uniform.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Uniform`

The uniform distribution: every value in an interval is equally likely.

## Signature

```cpp
pluss::Series distr::pdf(dst::Uniform, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Uniform, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Uniform, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Uniform, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Uniform, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The uniform distribution on `[a, b]` has a constant density `1/(b - a)` inside the interval and zero outside. It is the distribution of a random number drawn without preference, the distribution of the p-value of a true null hypothesis, and the building block of every sampler: a uniform draw pushed through the inverse of a distribution function has that distribution. Its mean is `(a + b)/2` and its variance `(b - a)²/12`.

The kind-based functions take `{a, b}`; the object is `distr::Uniform{low, high}`, whose `MLE` is the sample minimum and maximum.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `a` | a < b | the lower end of the interval |
| `b` | a < b | the upper end of the interval |

Support: the interval `[a, b]`.

The object form of this family, with the same functions as methods, is on its own page: [`distr::Uniform`](../object_based/Uniform.md).

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

- Parameters outside their valid range are refused with an error that names the requirement (`a` must be less than `b`).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Uniform on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.0, 1.0, 1.5, 2.0, 2.9, 3.5}, f64);
  const std::vector<double> params = {1.0, 3.0};

  std::cout << "x:";
  for (double v : x.pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Uniform, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Uniform, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Uniform, x, params).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.00000 1.00000 1.50000 2.00000 2.90000 3.50000
pdf: 0.00000 0.50000 0.50000 0.50000 0.50000 0.00000
cdf: 0.00000 0.00000 0.25000 0.50000 0.95000 1.00000
logpdf: -inf -0.69315 -0.69315 -0.69315 -0.69315 -inf
```

The density is largest on this grid at x = 1, where it is 0.5; the distribution function runs from 0 at x = 0 to 1 at x = 3.5, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.5) = -0.69315 at x = 1; where the density is zero (outside the support) the logarithm is `-inf`.

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
  const std::vector<double> params = {1.0, 3.0};

  // The three ways to the probability that the variable lies in [1.5, 2.5]:
  const auto grid = pluss::series::linspace(1.5, 2.5, 3001, f64);
  const double area = as_f64(distr::pdf(dst::Uniform, grid, params).antidiff(grid).trapezoid());

  const auto edges = pluss::make_series({1.5, 2.5}, f64);
  const auto c = distr::cdf(dst::Uniform, edges, params).pull<double>();

  const auto draws = distr::sample(dst::Uniform, params, {200000}, f64, 7ULL);
  const double fraction = as_f64(((draws >= 1.5).to(pluss::ScalarType::Float64) * (draws <= 2.5).to(pluss::ScalarType::Float64)).mean());

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(2.5) - cdf(1.5): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.50000
cdf(2.5) - cdf(1.5): 0.50000
fraction of 200000 draws in range: 0.49936
```

The integral of the density (0.5) and the difference of the distribution function (0.5) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.49936) differs from it by 6.40e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 0.00112), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({-0.5, 0.5, 1.5, 2.5, 4.0}, f64);

  std::cout << "x:";
  for (double v : x.pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "[0, 1]:";
  for (double v : distr::pdf(dst::Uniform, x, {0.0, 1.0}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "[1, 3]:";
  for (double v : distr::pdf(dst::Uniform, x, {1.0, 3.0}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "[-1, 5]:";
  for (double v : distr::pdf(dst::Uniform, x, {-1.0, 5.0}).pull<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -0.5000 0.5000 1.5000 2.5000 4.0000
[0, 1]: 0.0000 1.0000 0.0000 0.0000 0.0000
[1, 3]: 0.0000 0.0000 0.5000 0.5000 0.0000
[-1, 5]: 0.1667 0.1667 0.1667 0.1667 0.1667
```

Read down the rows: `[0, 1]` peaks on this grid at x = 0.5 (value 1); `[1, 3]` peaks on this grid at x = 1.5 (value 0.5); `[-1, 5]` peaks on this grid at x = -0.5 (value 0.1667). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 checks the distribution function against its closed form, `(x - a)/(b - a)` inside the interval.

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
  const auto x = pluss::make_series({1.2, 1.5, 2.0, 2.5, 2.9}, f64);
  const std::vector<double> params = {1.0, 3.0};

  const auto formula = (x - 1.0) * 0.5;
  const auto library = distr::cdf(dst::Uniform, x, params);

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
x: 1.200000 1.500000 2.000000 2.500000 2.900000
formula: 0.100000 0.250000 0.500000 0.750000 0.950000
distr::cdf: 0.100000 0.250000 0.500000 0.750000 0.950000
largest difference: 0.00e+00
```

The formula and `distr::cdf` agree to 0 over the grid, so the library implements exactly the closed form quoted above; the same expression is the quickest way to check a parameterisation before relying on it.

### Example 5

Example 5 confirms that the uniform distribution on [0, 1] is the beta distribution with both parameters equal to 1.

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
  const auto x = pluss::make_series({0.1, 0.3, 0.5, 0.8, 0.95}, f64);
  const std::vector<double> here = {0.0, 1.0};
  const std::vector<double> there = {1.0, 1.0};

  const double dpdf = as_f64((distr::pdf(dst::Uniform, x, here) - distr::pdf(dst::Beta, x, there)).abs().max());
  const double dcdf = as_f64((distr::cdf(dst::Uniform, x, here) - distr::cdf(dst::Beta, x, there)).abs().max());
  std::cout << "largest difference in the density: " << dpdf << '\n';
  std::cout << "largest difference in the distribution function: " << dcdf << '\n';
}
```

Output

```text
largest difference in the density: 0.00e+00
largest difference in the distribution function: 1.39e-17
```

The two largest differences are 0 for the density and 1.39e-17 for the distribution function over the grid: zero up to rounding, so the two descriptions are the same distribution.

## See Also

- [`Beta`](../continuous_bounded/Beta.md)
- [`Triangular`](../continuous_bounded/Triangular.md)
- [`Arcsin`](../continuous_bounded/Arcsin.md)
- [`Quadratic`](../continuous_bounded/Quadratic.md)
- [`distr::Uniform`](../object_based/Uniform.md) -- the object form
