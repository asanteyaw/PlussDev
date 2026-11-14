<!-- generated from fluctua.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `sde::fluctua`

Simulates `dY = μ(t, Y, θ) dt + σ(t, Y, θ) dW` from `t0` to `t1` for one path or a batch and returns the end state.

## Signature

```cpp
sde::SdeResult sde::fluctua(const sde::DriftFn &mu, const sde::DiffusionFn &sigma, const pluss::Series &y0, double t0, double t1, const pluss::Series &theta = pluss::series::undefined(), const sde::SdeOptions &options = {});
```

## What It Does

`mu(t, y, theta)` and `sigma(t, y, theta)` are callables returning a `Series` of the shape of `y`; `t` is a plain number. `y0` is the initial state, and any leading axis is a batch of **independent** paths: each entry of `y0` is driven by its own scalar Wiener increment, so a call with `y0` of shape `[N]` simulates `N` paths at once, and the whole step is a handful of vectorised `Series` expressions. `theta` is an optional `Series` of parameters passed to both callables (undefined if you have none).

The interval is divided into `options.steps` equal steps and advanced by the chosen [scheme](SdeMethod.md). The result carries the end state and the total Wiener increment each path used. Diffusion is **elementwise**: `sigma` has the shape of `y` and entry `i` of the noise drives entry `i` of the state, independent of the others. Correlated multi-factor noise is not modelled by a single call; build it by transforming independent factors inside the callables.

**Choosing the step.** The Euler-Maruyama error in a path is of order `√h`, in an expectation of order `h`. For pricing by averaging, 50 to 100 steps over a year is usually plenty; for pathwise quantities (hedging error, individual-path comparisons) use Milstein or more steps. Use a closed form when one exists: simulating a geometric Brownian motion only to read its terminal value is a convenience, not an efficiency.

## Parameters

`const sde::DriftFn &mu`
: `Series (double t, const Series &y, const Series &theta)`, the drift, shaped like `y`.

`const sde::DiffusionFn &sigma`
: The same signature; the diffusion, shaped like `y`, one independent noise per entry.

`const pluss::Series &y0`
: Initial state; leading axes are independent paths.

`double t0, double t1`
: Start and end times.

`const pluss::Series &theta`
: Optional parameters, passed to both callables.

`const sde::SdeOptions &options`
: Scheme, steps and seed.

## Returns

An [`SdeResult`](SdeResult.md).

## Errors

- A missing callback, an undefined `y0`, or `steps <= 0` throws (`pluss::Error`).

## Practical Examples

### Example 1

Geometric Brownian motion. `dY = rY dt + σY dW` has mean `Y₀ e^{rT}`; 40,000 paths give it to within sampling error.

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
  const double r = 0.05, s = 0.4, T = 1.0;
  const sde::DriftFn drift = [=](double, const Series &y, const Series &) { return y * r; };
  const sde::DiffusionFn vol = [=](double, const Series &y, const Series &) { return y * s; };
  const std::int64_t paths = 40000;
  const Series y0 = Series::full({paths}, 100.0, pluss::ScalarType::Float64);
  sde::SdeOptions options;
  options.method = sde::SdeMethod::EulerMaruyama;
  options.steps = 64;
  options.seed = 7;

  const Series yT = sde::fluctua(drift, vol, y0, 0.0, T, pluss::series::undefined(), options).y_final;
  out.writeln("mean =", yT.mean().item<double>(), ", exact =", 100.0 * std::exp(r * T));
  out.writeln("mean of log Y_T =", yT.log().mean().item<double>(), ", exact =", std::log(100.0) + (r - 0.5 * s * s) * T);
}
```

Output

```text
mean = 104.967 , exact = 105.127
mean of log Y_T = 4.574 , exact = 4.575
```

### Example 2

Mean reversion. The Ornstein-Uhlenbeck process `dY = κ(m − Y)dt + σ dW` settles to variance `σ²/(2κ)`, here `0.09/(2·2) = 0.0225`, independent of where it starts.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double kappa = 2.0, m = 1.0, s = 0.3;
  const sde::DriftFn drift = [=](double, const Series &y, const Series &) { return (y - m) * -kappa; };
  const sde::DiffusionFn vol = [=](double, const Series &y, const Series &) { return y * 0.0 + s; };
  const Series y0 = Series::full({50000}, 3.0, pluss::ScalarType::Float64);
  sde::SdeOptions options;
  options.steps = 400;
  options.seed = 11;
  const Series yT = sde::fluctua(drift, vol, y0, 0.0, 4.0, pluss::series::undefined(), options).y_final;
  const double mean = yT.mean().item<double>();
  out.writeln("mean =", mean, ", exact long-run mean =", m);
  out.writeln("variance =", (yT - mean).square().mean().item<double>(), ", exact =", s * s / (2.0 * kappa));
}
```

Output

```text
mean = 1.0014 , exact long-run mean = 1.0000
variance = 0.0229 , exact = 0.0225
```

### Example 3

Parameters. The drift reads `θ`, so the same program runs with different rates: here `θ = 0.10` as the growth rate, and the mean follows `Y₀ e^{θT}`.

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
  const double r = 0.05, s = 0.4, T = 1.0;
  const sde::DriftFn drift = [=](double, const Series &y, const Series &) { return y * r; };
  const sde::DiffusionFn vol = [=](double, const Series &y, const Series &) { return y * s; };

  const sde::DriftFn param_drift = [](double, const Series &y, const Series &theta) { return y * theta.select(0, 0); };
  const Series y0 = Series::full({40000}, 100.0, pluss::ScalarType::Float64);
  const Series theta = pluss::make_series({0.10});
  sde::SdeOptions options;
  options.method = sde::SdeMethod::EulerMaruyama;
  options.steps = 64;
  options.seed = 7;

  const Series yT = sde::fluctua(param_drift, vol, y0, 0.0, T, theta, options).y_final;
  out.writeln("mean =", yT.mean().item<double>(), ", exact =", 100.0 * std::exp(0.10 * T));
}
```

Output

```text
mean = 110.342 , exact = 110.517
```

### Example 4

Time dependence. A volatility that grows with time, `σ(t) = 0.1 + 0.2t`, gives the log-variance `∫σ² dt`; `t` is passed to the callables at the start of each step.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const sde::DriftFn none = [](double, const Series &y, const Series &) { return y * 0.0; };
  const sde::DiffusionFn vol = [](double t, const Series &y, const Series &) { return y * 0.0 + (0.1 + 0.2 * t); };
  const Series y0 = Series::full({50000}, 0.0, pluss::ScalarType::Float64);
  sde::SdeOptions options;
  options.steps = 200;
  options.seed = 4;
  const Series yT = sde::fluctua(none, vol, y0, 0.0, 1.0, pluss::series::undefined(), options).y_final;
  const double exact = 0.01 + 0.2 * 0.1 + 0.04 / 3.0;   // integral of (0.1 + 0.2 t)^2 over [0, 1]
  out.writeln("variance of Y_T =", yT.square().mean().item<double>(), ", exact =", exact);
}
```

Output

```text
variance of Y_T = 0.0431 , exact = 0.0433
```

### Example 5

Batches have the shape of the initial state. A `[3, 2]` state is six independent paths; the end state keeps the shape.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const double r = 0.05, s = 0.4, T = 1.0;
  const sde::DriftFn drift = [=](double, const Series &y, const Series &) { return y * r; };
  const sde::DiffusionFn vol = [=](double, const Series &y, const Series &) { return y * s; };

  const Series y0 = pluss::make_series({100.0, 110.0, 120.0, 130.0, 140.0, 150.0}, {3, 2});
  sde::SdeOptions options;
  options.method = sde::SdeMethod::EulerMaruyama;
  options.steps = 20;
  options.seed = 7;

  const sde::SdeResult res = sde::fluctua(drift, vol, y0, 0.0, T, pluss::series::undefined(), options);
  out.writeln("shape =", res.y_final.sizes(), ", wiener shape =", res.wiener_final.sizes());
}
```

Output

```text
shape = [3, 2] , wiener shape = [3, 2]
```

### Example 6

A missing callback or a non-positive step count is rejected.

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
    (void)sde::fluctua(drift, vol, y0, 0.0, 1.0, pluss::series::undefined(), bad);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
  try {
    (void)sde::fluctua(sde::DriftFn{}, vol, y0, 0.0, 1.0);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::sde::fluctua: steps must be > 0
subsp::math::sde::fluctua: drift callback is required
```

## See Also

- [`SdeOptions`](SdeOptions.md): method, steps and seed
- [`SdeResult`](SdeResult.md): what it returns
- [`fluctua_adjoint`](fluctua_adjoint.md): gradients of the same simulation

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SdeResult.html">
    <span class="label">Previous</span>
    <span class="title">SdeResult</span>
  </a>
  <a class="next-link" href="fluctua_adjoint.html">
    <span class="label">Next</span>
    <span class="title">fluctua_adjoint</span>
  </a>
</div>
<!-- apiref:nav:end -->
