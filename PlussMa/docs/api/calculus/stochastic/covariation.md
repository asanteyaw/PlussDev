<!-- generated from covariation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::stochastic::covariation`

The quadratic covariation `[X, Y]` of two sampled paths: the sum of products of their increments.

## Signature

```cpp
operators::Operator operators::stochastic::covariation(math::Function other, operators::stochastic::PathOptions options = {});
```

## What It Does

`covariation(Y)` applied to a function `X` of the sampled variables returns the path functional `[X, Y] = Σ ΔX ΔY`, the sum over the path of the products of the increments of `X` and `Y`. It is the cross version of [`quadratic_variation`](quadratic_variation.md), with `[X, X] = [X]`. For independent Wiener paths it is near 0, for perfectly correlated ones it is `T`, and for a pair with correlation `ρ` it is `ρT`. A smooth path has zero covariation with any continuous path. It is bilinear and symmetric.

`other` is a function of the *same* sampled variables as the input, so two different paths are two variables of one function, such as `W` and `V` in `math::function({"t", "W", "V"}, ...)`.

## Parameters

`math::Function other`
: The second path `Y`, a function of the same sampled variables as the input.

`operators::stochastic::PathOptions options`
: Running value and path axis.

## Returns

An `Operator` whose application returns `[X, Y]` as a path functional.

## Errors

- The sampled arrays must have a path axis of at least two samples.

## Practical Examples

### Example 1

A path with itself: `[W, W]` equals the quadratic variation `[W]`. With the smooth path `t` it is near 0 (here `1.3·10⁻⁴`, of order `T/N · max|ΔW|`).

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

  const math::Function w_again = math::function({"t", "W"}, [](math::Function, math::Function w) { return w; });
  const math::Function time_only = math::function({"t", "W"}, [](math::Function t, math::Function) { return t; });
  out.writeln("[W, W]  =", stochastic::covariation(w_again)(integrand)({times, path.w}).item<double>(), "   [W] =", qv);
  out.writeln("[W, t]  =", stochastic::covariation(time_only)(integrand)({times, path.w}).item<double>());
}
```

Output

```text
[W, W]  = 1.030136    [W] = 1.030136
[W, t]  = 0.000132
```

### Example 2

Two independent Wiener paths have covariation near 0, and `[W, W₂]²` is small compared with `[W][W₂]`. A third path built as `Y = ρW + √(1 − ρ²) V` has `[W, Y] ≈ ρ`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::int64_t steps = 4000;
  const double rho = 0.6;
  const sde::WienerPath first = sde::simulate_wiener_path(pluss::make_series(0.0), 0.0, 1.0, steps, 11);
  const sde::WienerPath second = sde::simulate_wiener_path(pluss::make_series(0.0), 0.0, 1.0, steps, 12);
  const pluss::Series times = pluss::make_series(first.times, {steps + 1});
  const math::Function W = math::function({"t", "W", "V"}, [](math::Function, math::Function w, math::Function) { return w; });
  const math::Function V = math::function({"t", "W", "V"}, [](math::Function, math::Function, math::Function v) { return v; });
  const math::Function Y = math::function({"t", "W", "V"}, [&](math::Function, math::Function w, math::Function v) { return rho * w + std::sqrt(1.0 - rho * rho) * v; });
  out.writeln("[W, V] (independent)  =", stochastic::covariation(V)(W)({times, first.w, second.w}).item<double>());
  out.writeln("[W, Y] (rho = 0.6)    =", stochastic::covariation(Y)(W)({times, first.w, second.w}).item<double>());
}
```

Output

```text
[W, V] (independent)  = -0.0142
[W, Y] (rho = 0.6)    = 0.5858
```

### Example 3

Bilinearity: `[W, 2W + 3t] = 2[W]`, because the covariation with the smooth part vanishes. And it is symmetric.

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

  const math::Function mixed = math::function({"t", "W"}, [](math::Function t, math::Function w) { return 2.0 * w + 3.0 * t; });
  out.writeln("[W, 2W + 3t] =", stochastic::covariation(mixed)(integrand)({times, path.w}).item<double>(), "   2 [W] =", 2.0 * qv);
  out.writeln("[2W + 3t, W] =", stochastic::covariation(integrand)(mixed)({times, path.w}).item<double>());
}
```

Output

```text
[W, 2W + 3t] = 2.060666    2 [W] = 2.060271
[2W + 3t, W] = 2.060666
```

## See Also

- [`quadratic_variation`](quadratic_variation.md): the case `Y = X`
- [`PathOptions`](PathOptions.md): the options

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="convention_change.html">
    <span class="label">Previous</span>
    <span class="title">convention_change</span>
  </a>
  <a class="next-link" href="generator.html">
    <span class="label">Next</span>
    <span class="title">generator</span>
  </a>
</div>
<!-- apiref:nav:end -->
