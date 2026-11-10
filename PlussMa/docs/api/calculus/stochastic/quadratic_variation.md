<!-- generated from quadratic_variation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::stochastic::quadratic_variation`

The quadratic variation `[X]` of a sampled path: the sum of squared increments.

## Signature

```cpp
operators::Operator operators::stochastic::quadratic_variation(operators::stochastic::PathOptions options = {});
```

## What It Does

`quadratic_variation()` applied to a function of sampled variables returns the path functional `[X] = Σ (ΔX)²`, the sum over the path of the squared increments of the value of the function. It measures the accumulated roughness of a path. A smooth path has quadratic variation going to zero as the grid is refined (the squared increments are second-order small); a Wiener path has `[W]_T = T`, the fingerprint of Brownian motion and the source of the extra term in Itô's formula.

The result sums the path axis out (or keeps a running value with `cumulative = true`); other axes are independent paths. It is quadratic: `[aX] = a²[X]`. It is not linear in the input and has no adjoint.

## Parameters

`operators::stochastic::PathOptions options`
: Whether to keep the running value (`cumulative`) and which axis is the path (`path_axis`). See `PathOptions`.

## Returns

An `Operator` whose application returns `[X]` as a path functional.

## Errors

- The sampled arrays must have a path axis of at least two samples.

## Practical Examples

### Example 1

A Wiener path over one unit of time has quadratic variation close to 1; a smooth path has almost none. For `W` on a 2000-step grid `[W] ≈ 1.03`; for the straight line `X = t` it is `T²/N = 0.0005`; and `[2W] = 4[W]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const std::int64_t steps = 2000;
  const sde::WienerPath path = sde::simulate_wiener_path(pluss::make_series(0.0), 0.0, 1.0, steps, 7);
  const pluss::Series times = pluss::make_series(path.times, {steps + 1});
  const math::Function integrand = math::function({"t", "W"}, [](math::Function, math::Function w) { return w; });
  const double w_final = path.w.select(0, steps).item<double>();
  const double qv = stochastic::quadratic_variation()(integrand)({times, path.w}).item<double>();

  const math::Function line = math::function({"t"}, [](math::Function t) { return t; });
  const math::Function twice = math::function({"t", "W"}, [](math::Function, math::Function w) { return 2.0 * w; });
  out.writeln("[W]    =", qv);
  out.writeln("[t]    =", stochastic::quadratic_variation()(line)({times}).item<double>(), "  (T^2 / N = 0.0005)");
  out.writeln("[2 W]  =", stochastic::quadratic_variation()(twice)({times, path.w}).item<double>(), "  (4 [W] =", 4.0 * qv, ")");
}
```

Output

```text
[W]    = 1.030136
[t]    = 0.000500   (T^2 / N = 0.0005)
[2 W]  = 4.120543   (4 [W] = 4.120543 )
```

### Example 2

A running quadratic variation. With `cumulative = true` the value at each sample is `[X]` up to that time; it grows roughly linearly, `[W]_t ≈ t`, and its last entry is the total.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::int64_t steps = 2000;
  const sde::WienerPath path = sde::simulate_wiener_path(pluss::make_series(0.0), 0.0, 1.0, steps, 7);
  const pluss::Series times = pluss::make_series(path.times, {steps + 1});
  const math::Function integrand = math::function({"t", "W"}, [](math::Function, math::Function w) { return w; });
  const double w_final = path.w.select(0, steps).item<double>();
  const double qv = stochastic::quadratic_variation()(integrand)({times, path.w}).item<double>();

  const pluss::Series running = stochastic::quadratic_variation({.cumulative = true})(integrand)({times, path.w});
  out.writeln("shape:", running.sizes());
  out.writeln("[W]_t at t = 0.25, 0.5, 0.75, 1:", pluss::make_series({running.select(0, 500).item<double>(), running.select(0, 1000).item<double>(), running.select(0, 1500).item<double>(), running.select(0, 2000).item<double>()}).items<double>());
}
```

Output

```text
shape: [2001]
[W]_t at t = 0.25, 0.5, 0.75, 1: [0.2491, 0.5117, 0.7669, 1.0301]
```

### Example 3

The Itô correction. The difference between the Stratonovich and Itô integrals of `W dW` is exactly half the quadratic variation, on every path; this is why Itô's formula has the extra term.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const std::int64_t steps = 2000;
  const sde::WienerPath path = sde::simulate_wiener_path(pluss::make_series(0.0), 0.0, 1.0, steps, 7);
  const pluss::Series times = pluss::make_series(path.times, {steps + 1});
  const math::Function integrand = math::function({"t", "W"}, [](math::Function, math::Function w) { return w; });
  const double w_final = path.w.select(0, steps).item<double>();
  const double qv = stochastic::quadratic_variation()(integrand)({times, path.w}).item<double>();

  const double ito = stochastic::stochastic_integral("W")(integrand)({times, path.w}).item<double>();
  const double strat = stochastic::stochastic_integral("W", {.convention = stochastic::Convention::Stratonovich})(integrand)({times, path.w}).item<double>();
  out.writeln("Strat - Ito =", strat - ito, "   [W] / 2 =", qv / 2.0);
}
```

Output

```text
Strat - Ito = 0.51506785    [W] / 2 = 0.51506785
```

### Example 4

Batches: arrays of independent paths give one value per path. Three copies of the path give three equal values; `path_axis` selects the axis that is the path.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const std::int64_t steps = 2000;
  const sde::WienerPath path = sde::simulate_wiener_path(pluss::make_series(0.0), 0.0, 1.0, steps, 7);
  const pluss::Series times = pluss::make_series(path.times, {steps + 1});
  const math::Function integrand = math::function({"t", "W"}, [](math::Function, math::Function w) { return w; });
  const double w_final = path.w.select(0, steps).item<double>();
  const double qv = stochastic::quadratic_variation()(integrand)({times, path.w}).item<double>();

  const pluss::Series t3 = times.unsqueeze(0).repeat({3, 1}), w3 = path.w.unsqueeze(0).repeat({3, 1});
  out.writeln("last axis:", stochastic::quadratic_variation()(integrand)({t3, w3}).items<double>());
  const pluss::Series t3t = times.unsqueeze(1).repeat({1, 3}), w3t = path.w.unsqueeze(1).repeat({1, 3});
  out.writeln("axis 0:   ", stochastic::quadratic_variation({.path_axis = 0})(integrand)({t3t, w3t}).items<double>());
}
```

Output

```text
last axis: [1.030136, 1.030136, 1.030136]
axis 0:    [1.030136, 1.030136, 1.030136]
```

## See Also

- [`covariation`](covariation.md): the cross version
- [`PathOptions`](PathOptions.md): the options
- [`stochastic_integral`](stochastic_integral.md): where the correction appears

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="malliavin_derivative.html">
    <span class="label">Previous</span>
    <span class="title">malliavin_derivative</span>
  </a>
  <a class="next-link" href="skorokhod_integral.html">
    <span class="label">Next</span>
    <span class="title">skorokhod_integral</span>
  </a>
</div>
<!-- apiref:nav:end -->
