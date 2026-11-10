<!-- generated from stochastic_integral.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::stochastic::stochastic_integral`

The stochastic integral of a function of a sampled path against a noise path, in the Itô, Stratonovich, Klimontovich or general θ convention.

## Signature

```cpp
operators::Operator operators::stochastic::stochastic_integral(std::string_view noise, operators::stochastic::IntegralOptions options = {});
```

## What It Does

A stochastic integral `∫ f(t, W) dW` cannot be an ordinary integral, because the noise `W` has no derivative. It is defined as a limit of sums over the steps of a sampled path, and *where in each step the integrand is read* decides what the limit is. `stochastic_integral("W")` is the operator that, applied to a function `f` of the sampled variables (time, the noise `W`, and so on), returns the integral as a path functional: given arrays that hold the samples of one path along a *path axis*, the result is the sum over the path of `f` at the chosen point of each step times the increment of `W`, with the path axis summed out. Every other axis is an independent path, so a batch of paths is integrated in one call.

The integrand is read at `(1 − θ)` times the left end plus `θ` times the right end of each step. The `Convention` names the usual choices:

| `Convention` | `θ` | Properties |
|---|---|---|
| `Ito` (default) | 0 | the integrand cannot anticipate the increment it multiplies; the integral is a martingale |
| `Stratonovich` | ½ | the ordinary chain rule holds |
| `Klimontovich` | 1 | right end point (backward) |
| `Theta` | `options.theta` | any `θ` in `[0, 1]` |

The integrand is a pointwise `Function` of the sampled variables, so it cannot look ahead along the path and the Itô sum is non-anticipating by construction: an opaque callable, which could look ahead, is rejected. For `∫ W dW` the discrete values are exact on any path: with `[W] = Σ (ΔW)²`, `(W_T² + (2θ − 1)[W]) / 2`. See [`IntegralOptions`](IntegralOptions.md) for the fields.

## Parameters

`std::string_view noise`
: The variable of the integrand that is the noise path `W`. The integral is taken with respect to its increments.

`operators::stochastic::IntegralOptions options`
: The convention, `θ`, whether to keep the running integral, and the path axis. See `IntegralOptions`.

## Returns

An `Operator` whose application returns the integral as a path functional: evaluated on sampled arrays, the path axis is summed out (kept, as a running integral, if `cumulative`).

## Errors

- `an opaque callable can look ahead along the path, so it is not a non-anticipating integrand`.
- `theta must lie in [0, 1]`.
- An unknown noise variable raises `unknown variable '<name>'`.

## Practical Examples

### Example 1

Itô, Stratonovich and Klimontovich integrals of `W` against itself. On any sampled path they are exactly `(W_T² − [W])/2`, `W_T²/2` and `(W_T² + [W])/2`, and the Stratonovich–Itô gap is half the quadratic variation `[W] ≈ 1`.

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

  const auto value = [&](stochastic::IntegralOptions options) { return stochastic::stochastic_integral("W", options)(integrand)({times, path.w}).item<double>(); };
  const double ito = value({}), strat = value({.convention = stochastic::Convention::Stratonovich}), klim = value({.convention = stochastic::Convention::Klimontovich});
  out.writeln("Ito          ", ito, "   (W_T^2 - [W]) / 2 =", (w_final * w_final - qv) / 2.0);
  out.writeln("Stratonovich ", strat, "   W_T^2 / 2         =", w_final * w_final / 2.0);
  out.writeln("Klimontovich ", klim, "   (W_T^2 + [W]) / 2 =", (w_final * w_final + qv) / 2.0);
  out.writeln("Strat - Ito  ", strat - ito, "   [W] / 2 =", qv / 2.0);
}
```

Output

```text
Ito           -0.480473    (W_T^2 - [W]) / 2 = -0.480473
Stratonovich  0.034595    W_T^2 / 2         = 0.034595
Klimontovich  0.549662    (W_T^2 + [W]) / 2 = 0.549662
Strat - Ito   0.515068    [W] / 2 = 0.515068
```

### Example 2

The general `θ` convention. With `θ = 0.25` the integral is `(W_T² + (2θ − 1)[W])/2`, between the Itô and Stratonovich values.

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

  const double theta = 0.25;
  const double value = stochastic::stochastic_integral("W", {.convention = stochastic::Convention::Theta, .theta = theta})(integrand)({times, path.w}).item<double>();
  out.writeln("theta = 0.25:", value, "   (W_T^2 + (2 theta - 1) [W]) / 2 =", (w_final * w_final + (2.0 * theta - 1.0) * qv) / 2.0);
}
```

Output

```text
theta = 0.25: -0.222939    (W_T^2 + (2 theta - 1) [W]) / 2 = -0.222939
```

### Example 3

A running integral. With `cumulative = true` the result keeps the path axis: the integral up to every sample, starting at 0. Its last entry is the terminal value.

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

  const pluss::Series running = stochastic::stochastic_integral("W", {.cumulative = true})(integrand)({times, path.w});
  const double terminal = stochastic::stochastic_integral("W")(integrand)({times, path.w}).item<double>();
  out.writeln("shape:", running.sizes(), "  first:", running.select(0, 0).item<double>(), "  last:", running.select(0, steps).item<double>(), "  terminal:", terminal);
}
```

Output

```text
shape: [2001]   first: 0.000000   last: -0.480473   terminal: -0.480473
```

### Example 4

Batches of paths. Arrays with a leading axis of independent paths integrate in one call: three copies of the path give three equal values. `path_axis` says which axis is the path (the last by default); here it is axis 0 of a transposed array.

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
  out.writeln("path on the last axis:  ", stochastic::stochastic_integral("W")(integrand)({t3, w3}).items<double>());
  const pluss::Series t3t = times.unsqueeze(1).repeat({1, 3}), w3t = path.w.unsqueeze(1).repeat({1, 3});
  out.writeln("path on axis 0:         ", stochastic::stochastic_integral("W", {.path_axis = 0})(integrand)({t3t, w3t}).items<double>());
}
```

Output

```text
path on the last axis:   [-0.480473, -0.480473, -0.480473]
path on axis 0:          [-0.480473, -0.480473, -0.480473]
```

### Example 5

What is refused: an opaque callable (it could look ahead), a noise name the function does not have, and `θ` outside `[0, 1]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const math::Function integrand = math::function({"t", "W"}, [](math::Function, math::Function w) { return w; });
  const math::Function opaque = math::function({"W"}, [](const pluss::Series &s) { return s; });
  try { (void)stochastic::stochastic_integral("W")(opaque); } catch (const pluss::Error &e) { out.writeln("opaque integrand:", first_line(e)); }
  try { (void)stochastic::stochastic_integral("Z")(integrand); } catch (const pluss::Error &e) { out.writeln("unknown noise:   ", first_line(e)); }
  try { (void)stochastic::stochastic_integral("W", {.convention = stochastic::Convention::Theta, .theta = 1.5})(integrand); } catch (const pluss::Error &e) { out.writeln("theta = 1.5:     ", first_line(e)); }
}
```

Output

```text
opaque integrand: operators::stochastic::stochastic_integral: an opaque callable can look ahead along the path, so it is not a non-anticipating integrand; build the integrand from Function operations, or use sde::ito_integral with its adaptedness probe
unknown noise:    math::Function: unknown variable 'Z'
theta = 1.5:      operators::stochastic::stochastic_integral: theta must lie in [0, 1]
```

## See Also

- [`IntegralOptions`](IntegralOptions.md): the options
- [`quadratic_variation`](quadratic_variation.md): the correction between conventions
- [`theta_of`](theta_of.md): the θ of each convention

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stochastic_differential.html">
    <span class="label">Previous</span>
    <span class="title">stochastic_differential</span>
  </a>
  <a class="next-link" href="theta_of.html">
    <span class="label">Next</span>
    <span class="title">theta_of</span>
  </a>
</div>
<!-- apiref:nav:end -->
