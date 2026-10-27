<!-- generated from Triangular.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Triangular`

The triangular distribution: a density that rises linearly to a mode and falls linearly, specified by its minimum, mode and maximum.

## Signature

```cpp
pluss::Series distr::pdf(dst::Triangular, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Triangular, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Triangular, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Triangular, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Triangular, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The triangular distribution is the simplest unimodal model on a bounded interval, specified by a minimum, a most likely value (the mode) and a maximum: it is the usual choice when only those three numbers are known, as in project scheduling and risk estimates. The density rises linearly from the minimum to the mode and falls linearly to the maximum, with its peak height `2/(b - a)`. Its mean is `(a + c + b)/3`.

The kind-based functions take `{a, c, b}`: the minimum, the mode and the maximum.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `a` | a <= c <= b | the minimum |
| `c` | a <= c <= b | the mode, where the density peaks |
| `b` | a <= c <= b | the maximum |

Support: the interval `[a, b]`.

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

- Parameters outside their valid range are refused with an error that names the requirement (the minimum, mode and maximum must satisfy `a <= c <= b` and `a < b`).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Triangular on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({0.0, 0.5, 1.0, 2.0, 3.0, 3.9}, f64);
  const std::vector<double> params = {0.0, 1.0, 4.0};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Triangular, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Triangular, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Triangular, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.00000 0.50000 1.00000 2.00000 3.00000 3.90000
pdf: 0.00000 0.25000 0.50000 0.33333 0.16667 0.01667
cdf: 0.00000 0.06250 0.25000 0.66667 0.91667 0.99917
logpdf: -inf -1.38629 -0.69315 -1.09861 -1.79176 -4.09434
```

The density is largest on this grid at x = 1, where it is 0.5; the distribution function runs from 0 at x = 0 to 0.99917 at x = 3.9, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.5) = -0.69315 at x = 1; where the density is zero (outside the support) the logarithm is `-inf`.

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
  const std::vector<double> params = {0.0, 1.0, 4.0};

  // The three ways to the probability that the variable lies in [0.5, 3]:
  const auto grid = pluss::series::linspace(0.5, 3.0, 3001, f64);
  const double area = distr::pdf(dst::Triangular, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({0.5, 3.0}, f64);
  const auto c = distr::cdf(dst::Triangular, edges, params).host<double>();

  const auto draws = distr::sample(dst::Triangular, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= 0.5).to(pluss::ScalarType::Float64) * (draws <= 3.0).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(3) - cdf(0.5): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.85417
cdf(3) - cdf(0.5): 0.85417
fraction of 200000 draws in range: 0.85416
```

The integral of the density (0.85417) and the difference of the distribution function (0.85417) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.85416) differs from it by 1.00e-05, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 7.89e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({0.0, 0.5, 1.0, 2.0, 3.0, 3.9}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "mode 0.5:";
  for (double v : distr::pdf(dst::Triangular, x, {0.0, 0.5, 4.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "mode 2  :";
  for (double v : distr::pdf(dst::Triangular, x, {0.0, 2.0, 4.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "mode 3.5:";
  for (double v : distr::pdf(dst::Triangular, x, {0.0, 3.5, 4.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: 0.0000 0.5000 1.0000 2.0000 3.0000 3.9000
mode 0.5: 0.0000 0.5000 0.4286 0.2857 0.1429 0.0143
mode 2  : 0.0000 0.1250 0.2500 0.5000 0.2500 0.0250
mode 3.5: 0.0000 0.0714 0.1429 0.2857 0.4286 0.1000
```

Read down the rows: `mode 0.5` peaks on this grid at x = 0.5 (value 0.5); `mode 2` peaks on this grid at x = 2 (value 0.5); `mode 3.5` peaks on this grid at x = 3 (value 0.4286). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

## See Also

- [`Uniform`](../continuous_bounded/Uniform.md)
- [`Beta`](../continuous_bounded/Beta.md)
- [`Semicircle`](../continuous_bounded/Semicircle.md)
- [`Quadratic`](../continuous_bounded/Quadratic.md)
