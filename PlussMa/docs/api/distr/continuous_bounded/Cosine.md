<!-- generated from Cosine.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Cosine`

The cosine (raised cosine) distribution: a smooth, bounded, bell-shaped density on a symmetric interval.

## Signature

```cpp
pluss::Series distr::pdf(dst::Cosine, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Cosine, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Cosine, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Cosine, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Cosine, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The cosine distribution has the density `(1 + cos(x - t))/(2 pi)` on `[t - pi, t + pi]` in its standard form (`s = pi`): a raised cosine, which is a smooth bounded alternative to the normal. It is the standard kernel of smoothing and the distribution of a sum of two independent uniform angles in some constructions. It is symmetric with its peak at `t`.

The kind-based functions take `{}` (centre 0, half-width pi) or `{t, s}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `t` | any real | the centre |
| `s` | > 0 | the half-width of the support |

Support: the interval `[t - s, t + s]`.

## Parameters

`const pluss::Series& x`
: The points at which to evaluate, a `Series` of any shape; the result has the same shape. Points outside the support have density zero and distribution value 0 or 1.

`const std::vector<double>& params`
: The parameters of the family, in the order shown in the table above (a shorter list selects the defaults: `{}` gives centre 0 and half-width pi). A list of the wrong length is refused.

`std::initializer_list<std::int64_t> sizes`
: The shape of the array of random draws, for example `{n}` or `{rows, columns}`.

`pluss::Config cfg`
: The dtype and platform of the draws; the default is Float32 on the CPU, so request Float64 when precision matters.

`std::uint64_t seed`
: The seed of the stream. The same seed always gives the same draws; a seed of 0 is a fixed default, not a random one.

## Returns

`pdf`, `logpdf`, `cdf` and `logcdf` return a `Series` of the shape of `x`; `sample` returns a `Series` of the requested shape. 

## Errors

- Parameters outside their valid range are refused with an error that names the requirement (`s` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Cosine on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({-3.0, -2.0, -1.0, 0.0, 1.5, 2.5}, f64);
  const std::vector<double> params = {};

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Cosine, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Cosine, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Cosine, x, params).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -3.00000 -2.00000 -1.00000 0.00000 1.50000 2.50000
pdf: 0.00159 0.09292 0.24515 0.31831 0.17041 0.03165
cdf: 0.00008 0.03697 0.20692 0.50000 0.89749 0.99314
logpdf: -6.44230 -2.37598 -1.40590 -1.14473 -1.76953 -3.45305
```

The density is largest on this grid at x = 0, where it is 0.31831; the distribution function runs from 8.00e-05 at x = -3 to 0.99314 at x = 2.5, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.31831) = -1.14473 at x = 0.

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

  // The three ways to the probability that the variable lies in [-2.5, 2.5]:
  const auto grid = pluss::series::linspace(-2.5, 2.5, 3001, f64);
  const double area = distr::pdf(dst::Cosine, grid, params).antidiff(grid).trapezoid().item<double>();

  const auto edges = pluss::make_series({-2.5, 2.5}, f64);
  const auto c = distr::cdf(dst::Cosine, edges, params).host<double>();

  const auto draws = distr::sample(dst::Cosine, params, {200000}, f64, 7ULL);
  const double fraction = ((draws >= -2.5).to(pluss::ScalarType::Float64) * (draws <= 2.5).to(pluss::ScalarType::Float64)).mean().item<double>();

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(2.5) - cdf(-2.5): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.98627
cdf(2.5) - cdf(-2.5): 0.98627
fraction of 200000 draws in range: 0.98577
```

The integral of the density (0.98627) and the difference of the distribution function (0.98627) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.98577) differs from it by 5.00e-04, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 2.60e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({-3.0, -2.0, -1.0, 0.0, 1.0, 2.0, 3.0}, f64);

  std::cout << "x:";
  for (double v : x.host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "s pi:";
  for (double v : distr::pdf(dst::Cosine, x, {0.0, 3.141592653589793}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "s 2 :";
  for (double v : distr::pdf(dst::Cosine, x, {0.0, 2.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "s 5 :";
  for (double v : distr::pdf(dst::Cosine, x, {0.0, 5.0}).host<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -3.0000 -2.0000 -1.0000 0.0000 1.0000 2.0000 3.0000
s pi: 0.0016 0.0929 0.2451 0.3183 0.2451 0.0929 0.0016
s 2 : 0.0000 0.0000 0.2500 0.5000 0.2500 0.0000 0.0000
s 5 : 0.0691 0.1309 0.1809 0.2000 0.1809 0.1309 0.0691
```

Read down the rows: `s pi` peaks on this grid at x = 0 (value 0.3183); `s 2` peaks on this grid at x = 0 (value 0.5); `s 5` peaks on this grid at x = 0 (value 0.2). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

## See Also

- [`HalfCosine`](../continuous_bounded/HalfCosine.md)
- [`Anglit`](../continuous_bounded/Anglit.md)
- [`Quadratic`](../continuous_bounded/Quadratic.md)
- [`VonMises`](../continuous_bounded/VonMises.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Bradford.html">
    <span class="label">Previous</span>
    <span class="title">Bradford</span>
  </a>
  <a class="next-link" href="HalfCosine.html">
    <span class="label">Next</span>
    <span class="title">HalfCosine</span>
  </a>
</div>
<!-- apiref:nav:end -->
