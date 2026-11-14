<!-- generated from Nig.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `distr::Nig`

The normal inverse Gaussian distribution: a four-parameter family for skewed, heavy-tailed data.

## Signature

```cpp
pluss::Series distr::pdf(dst::Nig, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logpdf(dst::Nig, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::cdf(dst::Nig, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::logcdf(dst::Nig, const pluss::Series& x, const std::vector<double>& params);
pluss::Series distr::sample(dst::Nig, const std::vector<double>& params, std::initializer_list<std::int64_t> sizes, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The normal inverse Gaussian (NIG) distribution is a normal variance-mean mixture: a normal variable whose variance (and mean) are scaled by an inverse Gaussian variable. It has four parameters that govern the tail weight (`alpha`), the skewness (`beta`), the centre and the scale, which makes it a standard model for the returns of financial assets. Its density is `alpha delta K1(alpha q) / (pi q) exp(delta gamma + beta (x - mu))` with `q = sqrt(delta² + (x - mu)²)`, `gamma = sqrt(alpha² - beta²)` and `K1` the modified Bessel function of the second kind. The tails decay like `exp(-(alpha - |beta|)|x|)`: a smaller `alpha` is a heavier tail, and a negative `beta` a longer left tail.

It is the generalised hyperbolic distribution with `lambda = -1/2`. The kind-based functions take `{alpha, beta, mu, delta}`.

The parameters, in the order of the `params` list:

| Parameter | Constraint | Meaning |
|---|---|---|
| `alpha` | > abs(beta) | the tail weight: smaller values give heavier tails, large values approach the normal |
| `beta` | abs(beta) < alpha | the skewness: negative gives a longer left tail |
| `mu` | any real | the location |
| `delta` | > 0 | the scale |

Support: all real numbers.

The object form of this family, with the same functions as methods, is on its own page: [`distr::Nig`](../object_based/Nig.md).

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

- Parameters outside their valid range are refused with an error that names the requirement (`alpha` must exceed `abs(beta)` and `delta` must be positive).
- A `params` list of the wrong length is refused.

## Practical Examples

### Example 1

Example 1 evaluates the density (mass) function, the distribution function and the log-density of the Nig on a small grid with `distr::pdf`, `distr::cdf` and `distr::logpdf`.

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

  const auto x = pluss::make_series({-5.0, -2.0, 0.0, 1.0, 3.0, 6.0}, f64);
  const std::vector<double> params = {1.5, -0.4, 0.1, 0.8};

  std::cout << "x:";
  for (double v : x.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "pdf:";
  for (double v : distr::pdf(dst::Nig, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "cdf:";
  for (double v : distr::cdf(dst::Nig, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "logpdf:";
  for (double v : distr::logpdf(dst::Nig, x, params).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -5.00000 -2.00000 0.00000 1.00000 3.00000 6.00000
pdf: 0.00037 0.03238 0.67138 0.12740 0.00088 0.00000
cdf: 0.00027 0.02055 0.54051 0.95011 0.99962 1.00000
logpdf: -7.90283 -3.43025 -0.39842 -2.06042 -7.03205 -13.71004
```

The density is largest on this grid at x = 0, where it is 0.67138; the distribution function runs from 2.70e-04 at x = -5 to 1 at x = 6, and it is the running total of the density. `logpdf` is the logarithm of the density, for example log(0.67138) = -0.39842 at x = 0.

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
  const std::vector<double> params = {1.5, -0.4, 0.1, 0.8};

  // The three ways to the probability that the variable lies in [-3, 3]:
  const auto grid = pluss::series::linspace(-3.0, 3.0, 3001, f64);
  const double area = as_f64(distr::pdf(dst::Nig, grid, params).antidiff(grid).trapezoid());

  const auto edges = pluss::make_series({-3.0, 3.0}, f64);
  const auto c = distr::cdf(dst::Nig, edges, params).items<double>();

  const auto draws = distr::sample(dst::Nig, params, {200000}, f64, 7ULL);
  const double fraction = as_f64(((draws >= -3.0).to(pluss::ScalarType::Float64) * (draws <= 3.0).to(pluss::ScalarType::Float64)).mean());

  std::cout << "integral of the density: " << area << '\n';
  std::cout << "cdf(3) - cdf(-3): " << c[1] - c[0] << '\n';
  std::cout << "fraction of 200000 draws in range: " << fraction << '\n';
}
```

Output

```text
integral of the density: 0.99511
cdf(3) - cdf(-3): 0.99511
fraction of 200000 draws in range: 0.99511
```

The integral of the density (0.99511) and the difference of the distribution function (0.99511) agree to all printed digits: they are two computations of one probability. The fraction of random draws (0.99511) differs from it by 0, which is the size of the sampling error of a fraction of 200000 draws (a standard error of about 1.56e-04), so the density, the distribution function and the sampler describe the same distribution.

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
  const auto x = pluss::make_series({-5.0, -2.0, 0.0, 1.0, 3.0, 6.0}, f64);

  std::cout << "x:";
  for (double v : x.items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "alpha 0.7:";
  for (double v : distr::pdf(dst::Nig, x, {0.7, -0.4, 0.1, 0.8}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "alpha 1.5:";
  for (double v : distr::pdf(dst::Nig, x, {1.5, -0.4, 0.1, 0.8}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
  std::cout << "alpha 6  :";
  for (double v : distr::pdf(dst::Nig, x, {6.0, -0.4, 0.1, 0.8}).items<double>()) std::cout << ' ' << v;
  std::cout << '\n';
}
```

Output

```text
x: -5.0000 -2.0000 0.0000 1.0000 3.0000 6.0000
alpha 0.7: 0.0082 0.0727 0.5154 0.1300 0.0036 0.0000
alpha 1.5: 0.0004 0.0324 0.6714 0.1274 0.0009 0.0000
alpha 6  : 0.0000 0.0001 1.1497 0.0379 0.0000 0.0000
```

Read down the rows: `alpha 0.7` peaks on this grid at x = 0 (value 0.5154); `alpha 1.5` peaks on this grid at x = 0 (value 0.6714); `alpha 6` peaks on this grid at x = 0 (value 1.1497). The parameters move the location, the width and the shape of the curve, which is what choosing a member of the family means.

### Example 4

Example 4 confirms that the generalised hyperbolic distribution with `lambda = -1/2` is the normal inverse Gaussian distribution.

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
  const auto x = pluss::make_series({-4.0, -1.0, 0.0, 1.0, 4.0}, f64);
  const std::vector<double> here = {1.5, -0.4, 0.1, 0.8};
  const std::vector<double> there = {-0.5, 1.5, -0.4, 0.1, 0.8};

  const double dpdf = as_f64((distr::pdf(dst::Nig, x, here) - distr::pdf(dst::Gh, x, there)).abs().max());
  const double dcdf = as_f64((distr::cdf(dst::Nig, x, here) - distr::cdf(dst::Gh, x, there)).abs().max());
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

- [`Gh`](../continuous_real_line/Gh.md)
- [`Normal`](../continuous_real_line/Normal.md)
- [`StudentT`](../continuous_real_line/StudentT.md)
- [`InverseGaussian`](../continuous_positive/InverseGaussian.md)
- [`distr::Nig`](../object_based/Nig.md) -- the object form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Logistic.html">
    <span class="label">Previous</span>
    <span class="title">Logistic</span>
  </a>
  <a class="next-link" href="Normal.html">
    <span class="label">Next</span>
    <span class="title">Normal</span>
  </a>
</div>
<!-- apiref:nav:end -->
