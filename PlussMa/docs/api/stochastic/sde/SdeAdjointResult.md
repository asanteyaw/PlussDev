<!-- generated from SdeAdjointResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `sde::SdeAdjointResult`

The end state of a simulation and the pathwise gradients of a loss with respect to the initial state and the parameters.

`grad_y0` is `∂L/∂Y₀` per path (shaped like `y0`) and `grad_theta` is `∂L/∂θ` (shaped like `theta`, undefined if no parameters were given). Both are **pathwise**: the realised noise is held fixed, and the gradient is that of the discretised simulation. The end state and total Wiener increment are returned too, so you can audit the realisation the gradient belongs to.

## Fields

### `y_final`

`pluss::Series`, default `undefined`.

The end state of the forward pass.

### `wiener_final`

`pluss::Series`, default `undefined`.

`W(t1) − W(t0)` per path, as in `SdeResult`.

### `grad_y0`

`pluss::Series`, default `undefined`.

`∂L/∂Y₀`, shaped like `y0`: one pathwise gradient per path.

### `grad_theta`

`pluss::Series`, default `undefined`.

`∂L/∂θ`, shaped like `theta`; undefined when `theta` was.

### `forward_steps`

`std::int64_t`, default `0`.

Steps of the forward sweep.

### `backward_steps`

`std::int64_t`, default `0`.

Steps of the backward sweep; equal to the forward count.

## Guidance

Returned by `fluctua_adjoint`.

## Practical Examples

### Example 1

Reading a result. A European call payoff on 5 paths of geometric Brownian motion: the per-path gradient with respect to the start is `1{Y_T > K} · Y_T / Y₀`.

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
  options.steps = 32;
  options.seed = 7;

  const double K = 100.0;
  const auto adj = sde::fluctua_adjoint(drift, vol, y0, 0.0, T, pluss::series::undefined(), [=](const Series &yT) { return (yT - K).relu().sum(); }, options);
  out.writeln("y_final =", adj.y_final.items<double>());
  out.writeln("grad_y0 =", adj.grad_y0.items<double>());
  out.writeln("1{Y>K} Y/Y0 =", ((adj.y_final > K).to(pluss::ScalarType::Float64) * adj.y_final / 100.0).items<double>());
  out.writeln("forward steps =", adj.forward_steps, ", backward steps =", adj.backward_steps);
}
```

Output

```text
y_final = [204.3770, 108.8919, 67.6465, 92.5432, 124.9493]
grad_y0 = [2.0438, 1.0889, 0.0000, 0.0000, 1.2495]
1{Y>K} Y/Y0 = [2.0438, 1.0889, 0.0000, 0.0000, 1.2495]
forward steps = 32 , backward steps = 32
```

### Example 2

`grad_theta` is defined only when parameters were passed. The drift `θ·Y` with `θ = 0.05`: `∂Y_T/∂θ` summed over paths is positive.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const double s = 0.4, T = 1.0;
  const sde::DriftFn drift = [](double, const Series &y, const Series &theta) { return y * theta.select(0, 0); };
  const sde::DiffusionFn vol = [=](double, const Series &y, const Series &) { return y * s; };
  const Series y0 = Series::full({1000}, 100.0, pluss::ScalarType::Float64);
  const Series theta = pluss::make_series({0.05});
  sde::SdeOptions options;
  options.steps = 32;
  options.seed = 2;
  const auto with = sde::fluctua_adjoint(drift, vol, y0, 0.0, T, theta, [](const Series &yT) { return yT.mean(); }, options);
  const auto without = sde::fluctua_adjoint(sde::DriftFn{[](double, const Series &y, const Series &) { return y * 0.05; }}, vol, y0, 0.0, T, pluss::series::undefined(), [](const Series &yT) { return yT.mean(); }, options);
  out.writeln("with theta   : grad_theta =", with.grad_theta.items<double>(), "(defined)");
  out.writeln("without theta: grad_theta defined =", without.grad_theta.defined());
}
```

Output

```text
with theta   : grad_theta = [105.215] (defined)
without theta: grad_theta defined = false
```

### Example 3

The same noise is reported. `wiener_final` of the adjoint equals that of the plain simulation with the same seed.

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
  const std::int64_t paths = 4;
  const Series y0 = Series::full({paths}, 100.0, pluss::ScalarType::Float64);
  sde::SdeOptions options;
  options.method = sde::SdeMethod::EulerMaruyama;
  options.steps = 16;
  options.seed = 9;

  const auto adj = sde::fluctua_adjoint(drift, vol, y0, 0.0, T, pluss::series::undefined(), [](const Series &yT) { return yT.sum(); }, options);
  const auto plain = sde::fluctua(drift, vol, y0, 0.0, T, pluss::series::undefined(), options);
  out.writeln("same noise:", (adj.wiener_final - plain.wiener_final).abs().max().item<double>() == 0.0, ", same end state:", (adj.y_final - plain.y_final).abs().max().item<double>() == 0.0);
}
```

Output

```text
same noise: true , same end state: true
```

## See Also

- [`fluctua_adjoint`](fluctua_adjoint.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="SdeMethod.html">
    <span class="label">Next</span>
    <span class="title">SdeMethod</span>
  </a>
</div>
<!-- apiref:nav:end -->
