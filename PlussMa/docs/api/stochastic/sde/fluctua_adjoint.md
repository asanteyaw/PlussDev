<!-- generated from fluctua_adjoint.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `sde::fluctua_adjoint`

Differentiates a simulation backwards: the pathwise gradients of a loss of the end state with respect to the initial state and the parameters.

## Signature

```cpp
sde::SdeAdjointResult sde::fluctua_adjoint(const sde::DriftFn &mu, const sde::DiffusionFn &sigma, const pluss::Series &y0, double t0, double t1, const pluss::Series &theta, const sde::LossFn &loss, const sde::SdeOptions &options = {});
sde::SdeAdjointResult sde::fluctua_adjoint(const sde::DriftFn &mu, const sde::DiffusionFn &sigma, const pluss::Series &y0, double t0, double t1, const pluss::Series &theta, const pluss::Series &grad_y_final, const sde::SdeOptions &options = {});
```

## What It Does

Both overloads run the simulation of [`fluctua`](fluctua.md) with the given options, then a backward sweep that returns the gradient of a scalar loss `L(Y_T)` with respect to the initial state and `θ`. The difference is how the loss enters:

- **Loss form** (first signature): you pass `loss(Y_T)`, any differentiable `Series` function returning a scalar (or single-element) `Series`; the gradient with respect to `Y_T` is derived for you. A batch of paths needs a loss that reduces over the batch (`.mean()` or `.sum()`), exactly as a training loss would.
- **Seed form** (second signature): you pass `∂L/∂Y_T` yourself, shaped like `y0`, when you already have it (or want the vector-Jacobian product for a specific cotangent).

**What the gradient is.** The sampled Wiener increments are fixed and treated as constants. The discretised simulation is then an ordinary deterministic function of `(Y₀, θ)`, and the backward sweep is its exact reverse-mode derivative, using stored states (O(steps) memory) and one local vector-Jacobian product per step. This is the *pathwise* (reparameterisation) estimator: for a smooth payoff its expectation is the derivative of the expected payoff. It is **not** valid for a discontinuous payoff (a digital option, a barrier): there the payoff is flat almost everywhere (no pathwise derivative exists to compute) while the true derivative is not, and a likelihood-ratio or Malliavin method is needed instead.

## Parameters

`const sde::DriftFn &mu / const sde::DiffusionFn &sigma`
: The drift and the (elementwise) diffusion, as in `fluctua`.

`const pluss::Series &y0`
: Initial state; leading axes are independent paths.

`double t0, double t1`
: Start and end times.

`const pluss::Series &theta`
: Parameters passed to the callables; undefined for none (then `grad_theta` is undefined).

`const sde::LossFn &loss`
: Loss form: `Series -> scalar Series`, differentiable.

`const pluss::Series &grad_y_final`
: Seed form: `∂L/∂Y_T`, shaped like `y0`.

`const sde::SdeOptions &options`
: Scheme, steps and seed; the same options reproduce the same paths as `fluctua`.

## Returns

An [`SdeAdjointResult`](SdeAdjointResult.md).

## Errors

- A missing callback or loss, an undefined `y0` or seed, or `steps <= 0` throws (`pluss::Error`).

## Practical Examples

### Example 1

A pathwise delta. A European call on geometric Brownian motion: the mean of the per-path gradients estimates `e^{rT} N(d₁)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double r = 0.05, s = 0.4, T = 1.0;
  const sde::DriftFn drift = [=](double, const Series &y, const Series &) { return y * r; };
  const sde::DiffusionFn vol = [=](double, const Series &y, const Series &) { return y * s; };
  const std::int64_t paths = 40000;
  const Series y0 = Series::full({paths}, 100.0, pluss::ScalarType::Float64);
  sde::SdeOptions options;
  options.method = sde::SdeMethod::Milstein;
  options.steps = 64;
  options.seed = 7;

  const double K = 105.0;
  const auto adj = sde::fluctua_adjoint(drift, vol, y0, 0.0, T, pluss::series::undefined(), [=](const Series &yT) { return (yT - K).relu().mean(); }, options);
  const double d1 = (std::log(100.0 / K) + (r + 0.5 * s * s) * T) / (s * std::sqrt(T));
  out.writeln("pathwise delta =", adj.grad_y0.sum().item<double>(), ", closed form =", std::exp(r * T) * 0.5 * std::erfc(-d1 / std::sqrt(2.0)));
}
```

Output

```text
pathwise delta = 0.6104 , closed form = 0.6102
```

### Example 2

A pathwise vega. The volatility is a parameter `θ`; the gradient with respect to it gives the call's vega, which in the Black-Scholes model is `S φ(d₁) √T`. A vega estimate is noisier than a delta, so 300,000 paths agree with the closed form only to about one percent.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const double r = 0.05, T = 1.0, K = 105.0;
  const sde::DriftFn drift = [=](double, const Series &y, const Series &) { return y * r; };
  const sde::DiffusionFn vol = [](double, const Series &y, const Series &theta) { return y * theta.select(0, 0); };
  const Series y0 = Series::full({300000}, 100.0, pluss::ScalarType::Float64);
  const Series theta = pluss::make_series({0.4});
  sde::SdeOptions options;
  options.method = sde::SdeMethod::Milstein;
  options.steps = 64;
  options.seed = 21;
  const auto adj = sde::fluctua_adjoint(drift, vol, y0, 0.0, T, theta, [=](const Series &yT) { return (yT - K).relu().mean() * std::exp(-r * T); }, options);
  const double s = 0.4;
  const double d1 = (std::log(100.0 / K) + (r + 0.5 * s * s) * T) / (s * std::sqrt(T));
  out.writeln("pathwise vega =", adj.grad_theta.items<double>(), ", closed form =", 100.0 * std::exp(-0.5 * d1 * d1) / std::sqrt(2.0 * M_PI) * std::sqrt(T));
}
```

Output

```text
pathwise vega = [38.766] , closed form = 39.080
```

### Example 3

The seed form. Passing `∂L/∂Y_T = 1` for every path gives `∂Y_T/∂Y₀`; for geometric Brownian motion that equals `Y_T / Y₀`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double r = 0.05, s = 0.4, T = 1.0;
  const sde::DriftFn drift = [=](double, const Series &y, const Series &) { return y * r; };
  const sde::DiffusionFn vol = [=](double, const Series &y, const Series &) { return y * s; };
  const std::int64_t paths = 5;
  const Series y0 = Series::full({paths}, 100.0, pluss::ScalarType::Float64);
  sde::SdeOptions options;
  options.method = sde::SdeMethod::EulerMaruyama;
  options.steps = 64;
  options.seed = 7;

  const Series seed = pluss::series::ones({5}, pluss::ScalarType::Float64);
  const auto adj = sde::fluctua_adjoint(drift, vol, y0, 0.0, T, pluss::series::undefined(), seed, options);
  out.writeln("grad_y0  =", adj.grad_y0.items<double>());
  out.writeln("Y_T / Y0 =", (adj.y_final / 100.0).items<double>());
}
```

Output

```text
grad_y0  = [1.5478, 1.1151, 0.7672, 0.9949, 1.4870]
Y_T / Y0 = [1.5478, 1.1151, 0.7672, 0.9949, 1.4870]
```

### Example 4

Both forms agree. A loss `Σ Y_T²` has gradient `2 Y_T` with respect to the end state; passing that as the seed gives the same answer as passing the loss.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const double r = 0.05, s = 0.4, T = 1.0;
  const sde::DriftFn drift = [=](double, const Series &y, const Series &) { return y * r; };
  const sde::DiffusionFn vol = [=](double, const Series &y, const Series &) { return y * s; };
  const std::int64_t paths = 6;
  const Series y0 = Series::full({paths}, 100.0, pluss::ScalarType::Float64);
  sde::SdeOptions options;
  options.method = sde::SdeMethod::EulerMaruyama;
  options.steps = 32;
  options.seed = 5;

  const auto by_loss = sde::fluctua_adjoint(drift, vol, y0, 0.0, T, pluss::series::undefined(), [](const Series &yT) { return yT.square().sum(); }, options);
  const auto by_seed = sde::fluctua_adjoint(drift, vol, y0, 0.0, T, pluss::series::undefined(), by_loss.y_final * 2.0, options);
  out.writeln("max difference =", (by_loss.grad_y0 - by_seed.grad_y0).abs().max().item<double>());
}
```

Output

```text
max difference = 0.0000000000
```

### Example 5

What the pathwise gradient cannot do. A digital payoff `1{Y_T > K}` is flat almost everywhere, so there is no pathwise derivative to compute: the comparison cuts the graph and the library says there is nothing to differentiate. Use a smooth payoff (here a steep sigmoid stands in for the step), or a Malliavin/likelihood-ratio method, for such products.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double r = 0.05, s = 0.4, T = 1.0;
  const sde::DriftFn drift = [=](double, const Series &y, const Series &) { return y * r; };
  const sde::DiffusionFn vol = [=](double, const Series &y, const Series &) { return y * s; };
  const std::int64_t paths = 2000;
  const Series y0 = Series::full({paths}, 100.0, pluss::ScalarType::Float64);
  sde::SdeOptions options;
  options.method = sde::SdeMethod::EulerMaruyama;
  options.steps = 32;
  options.seed = 5;

  const double K = 105.0;
  try {
    (void)sde::fluctua_adjoint(drift, vol, y0, 0.0, T, pluss::series::undefined(), [=](const Series &yT) { return (yT > K).to(pluss::ScalarType::Float64).mean(); }, options);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find(';')));
  }
  const auto smooth = sde::fluctua_adjoint(drift, vol, y0, 0.0, T, pluss::series::undefined(), [=](const Series &yT) { return ((yT - K) * 0.5).sigmoid().mean(); }, options);
  out.writeln("delta of the smoothed digital =", smooth.grad_y0.sum().item<double>());
}
```

Output

```text
series_tape::backpropagate: objective does not track slope and has no source link
delta of the smoothed digital = 0.0105
```

### Example 6

Errors: a missing loss or callback, or a non-positive step count, is rejected.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double r = 0.05, s = 0.4, T = 1.0;
  const sde::DriftFn drift = [=](double, const Series &y, const Series &) { return y * r; };
  const sde::DiffusionFn vol = [=](double, const Series &y, const Series &) { return y * s; };

  const Series y0 = pluss::make_series({1.0});
  sde::SdeOptions bad;
  bad.steps = 0;
  try {
    (void)sde::fluctua_adjoint(drift, vol, y0, 0.0, 1.0, pluss::series::undefined(), [](const Series &yT) { return yT.sum(); }, bad);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
  try {
    (void)sde::fluctua_adjoint(drift, vol, y0, 0.0, 1.0, pluss::series::undefined(), sde::LossFn{});
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::sde::fluctua: steps must be > 0
subsp::math::sde::fluctua_adjoint: loss callback is required
```

## See Also

- [`fluctua`](fluctua.md): the forward simulation
- [`SdeAdjointResult`](SdeAdjointResult.md): what it returns

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fluctua.html">
    <span class="label">Previous</span>
    <span class="title">fluctua</span>
  </a>
</div>
<!-- apiref:nav:end -->
