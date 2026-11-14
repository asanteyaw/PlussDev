<!-- generated from aleatoria.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::aleatoria`

Estimates an integral by Monte Carlo: the average of the integrand at random points, with its standard error.

## Signature

```cpp
integ::AleatoriaResult integ::aleatoria(const integ::IntegrandFn &fn, double a, double b, const integ::AleatoriaOptions &options = {});
integ::AleatoriaResult integ::aleatoria(const integ::IntegrandNdFn &fn, const std::vector<std::pair<double, double>> &bounds, const integ::AleatoriaOptions &options = {});
```

## What It Does

Monte Carlo draws `samples` points uniformly over the interval (or box), evaluates the integrand at all of them in **one call**, and returns the average times the volume of the domain. Its error falls as `1/√samples` whatever the smoothness of the integrand and whatever the dimension, which is why it is the method for integrals in many variables and for integrands too rough or discontinuous for a quadrature rule; for a smooth one-variable integrand [`quadra`](quadra.md) is far more accurate. The result carries the standard error of the estimate, so the accuracy is known, not assumed.

With `stratified` on, each sample is drawn from its own equal-width stratum, which removes the clumping and gaps of independent draws at no cost and lowers the error. Infinite bounds are handled in the one-variable form by the same change of variables `quadra` uses. The run is reproducible from `seed`. Complex integrands work, and the integral is a differentiable function of any parameter the integrand captures.

## Parameters

`const integ::IntegrandFn &fn`
: `Series (Series)`, evaluated once on all the sample points.

`double a, double b`
: The interval; either may be infinite.

`const integ::IntegrandNdFn &fn`
: `Series (std::vector<Series>)`: one `Series` of sample coordinates per axis.

`const std::vector<std::pair<double, double>> &bounds`
: One `(lower, upper)` pair per axis.

`const integ::AleatoriaOptions &options`
: Sample count, seed, stratification.

## Returns

An [`AleatoriaResult`](AleatoriaResult.md).

## Errors

- A missing integrand, `samples ≤ 0`, an empty `bounds`, or `b ≤ a` throw.

## Practical Examples

### Example 1

Integrating `x²` on `[0, 1]`, which is `1/3`. The estimate comes with its standard error, so the distance from the exact value is about one standard error.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  integ::AleatoriaOptions options;
  options.samples = 100000;
  options.seed = 7;
  const integ::AleatoriaResult r = integ::aleatoria([](const Series &t) { return t.square(); }, 0.0, 1.0, options);
  out.writeln("estimate =", r.value.item<double>(), ", standard error =", r.standard_error, ", exact =", 1.0 / 3.0);
}
```

Output

```text
estimate = 0.333221 , standard error = 0.000945 , exact = 0.333333
```

### Example 2

A box in several variables. The integral of `x₁ + x₂ + x₃` over the unit cube is `3/2`; Monte Carlo does not care that this is three-dimensional.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  integ::AleatoriaOptions options;
  options.samples = 200000;
  options.seed = 3;
  const integ::AleatoriaResult r = integ::aleatoria([](const std::vector<Series> &c) { return c[0] + c[1] + c[2]; }, {{0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}}, options);
  out.writeln("estimate =", r.value.item<double>(), ", standard error =", r.standard_error, ", exact = 1.5");
}
```

Output

```text
estimate = 1.49916 , standard error = 0.00112 , exact = 1.5
```

### Example 3

Stratified sampling. The same number of samples, a much smaller actual error: one draw per stratum rather than independent draws. The reported standard error is the formula for independent draws and does not credit the stratification, so with `stratified` on it overstates the true error.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(7));
  for (const bool stratified : {false, true}) {
    integ::AleatoriaOptions options;
    options.samples = 20000;
    options.seed = 11;
    options.stratified = stratified;
    const integ::AleatoriaResult r = integ::aleatoria([](const Series &t) { return t.exp(); }, 0.0, 1.0, options);
    out.writeln("stratified", stratified, ": |error| =", std::abs(r.value.item<double>() - (std::exp(1.0) - 1.0)), ", standard error =", r.standard_error);
  }
}
```

Output

```text
stratified false : |error| = 0.0073020 , standard error = 0.0034590
stratified true : |error| = 0.0000003 , standard error = 0.0034789
```

### Example 4

An infinite domain. The standard normal density integrates to 1 over the whole line.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  integ::AleatoriaOptions options;
  options.samples = 400000;
  options.seed = 5;
  const integ::AleatoriaResult r = integ::aleatoria([](const Series &t) { return (t.square() * -0.5).exp() / std::sqrt(2.0 * 3.14159265358979323846); }, -INFINITY, INFINITY, options);
  out.writeln("estimate =", r.value.item<double>(), ", standard error =", r.standard_error);
}
```

Output

```text
estimate = 0.99926 , standard error = 0.00087
```

### Example 5

A gradient through the estimate. For `∫₀¹ a x² dx = a/3` the derivative with respect to `a` is `1/3`, and the gradient of the Monte Carlo average is exact for the sampled points.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  Series a = pluss::make_series(2.0);
  a.tracks_slope_(true);
  integ::AleatoriaOptions options;
  options.samples = 50000;
  options.seed = 2;
  const integ::AleatoriaResult r = integ::aleatoria([&](const Series &t) { return t.square() * a; }, 0.0, 1.0, options);
  r.value.sum().reverse();
  out.writeln("estimate =", r.value.item<double>(), ", d/da =", a.sensitivity().item<double>());
}
```

Output

```text
estimate = 0.66854 , d/da = 0.33427
```

## See Also

- [`AleatoriaOptions`](AleatoriaOptions.md): the settings
- [`quadra`](quadra.md): deterministic quadrature
- [`integra`](integra.md): choosing automatically

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="QuadraScalarResult.html">
    <span class="label">Previous</span>
    <span class="title">QuadraScalarResult</span>
  </a>
  <a class="next-link" href="analytica_arcsine_kernel.html">
    <span class="label">Next</span>
    <span class="title">arcsine_kernel</span>
  </a>
</div>
<!-- apiref:nav:end -->
