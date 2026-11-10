<!-- generated from SdeResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `sde::SdeResult`

The state at the end time and the realised Wiener increment of a simulation.

`y_final` has the shape of the initial state (one entry per path). `wiener_final` is `W(t1) − W(t0)` for every entry, the total noise that path used; exposing it lets you compare the scheme with a closed-form solution that is a function of the *same* noise, which is a far sharper test than comparing distributions.

## Fields

### `y_final`

`pluss::Series`, default `undefined`.

The state at `t1`, shaped like `y0`.

### `wiener_final`

`pluss::Series`, default `undefined`.

`W(t1) − W(t0)` per entry, shaped like `y0`: the realised noise of each path.

### `steps`

`std::int64_t`, default `0`.

The number of steps taken, equal to `SdeOptions::steps`.

### `converged`

`bool`, default `false`.

True for a completed run. The fixed-step solver has no failure mode short of throwing, so it is true on every returned result.

## Guidance

Returned by `fluctua`.

## Practical Examples

### Example 1

Reading a result.

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
  options.steps = 50;
  options.seed = 7;

  const sde::SdeResult res = sde::fluctua(drift, vol, y0, 0.0, T, pluss::series::undefined(), options);
  out.writeln("y_final =", res.y_final.items<double>());
  out.writeln("wiener_final =", res.wiener_final.items<double>());
  out.writeln("steps =", res.steps, ", converged =", res.converged);
}
```

Output

```text
y_final = [97.6169, 261.6471, 52.1283, 113.3437]
wiener_final = [0.0084, 2.4652, -1.5764, 0.3501]
steps = 50 , converged = true
```

### Example 2

The path total is a Wiener increment: over many paths `wiener_final` has mean 0 and variance `t1 − t0`.

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
  const std::int64_t paths = 100000;
  const Series y0 = Series::full({paths}, 100.0, pluss::ScalarType::Float64);
  sde::SdeOptions options;
  options.method = sde::SdeMethod::EulerMaruyama;
  options.steps = 10;
  options.seed = 7;

  const Series w = sde::fluctua(drift, vol, y0, 0.0, 2.0, pluss::series::undefined(), options).wiener_final;
  out.writeln("mean =", w.mean().item<double>(), ", variance =", w.square().mean().item<double>(), " (T = 2)");
}
```

Output

```text
mean = -0.0046 , variance = 1.9874  (T = 2)
```

### Example 3

A closed-form check on the same noise. For geometric Brownian motion the scheme should agree with `Y₀ exp((r − σ²/2)T + σW)` up to its discretisation error.

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
  const std::int64_t paths = 3;
  const Series y0 = Series::full({paths}, 100.0, pluss::ScalarType::Float64);
  sde::SdeOptions options;
  options.method = sde::SdeMethod::Milstein;
  options.steps = 400;
  options.seed = 7;

  const sde::SdeResult res = sde::fluctua(drift, vol, y0, 0.0, T, pluss::series::undefined(), options);
  out.writeln("scheme :", res.y_final.items<double>());
  out.writeln("exact  :", (y0 * (res.wiener_final * s + (r - 0.5 * s * s) * T).exp()).items<double>());
}
```

Output

```text
scheme : [103.4629, 203.9180, 98.6602]
exact  : [103.4517, 203.9223, 98.6578]
```

## See Also

- [`fluctua`](fluctua.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SdeOptions.html">
    <span class="label">Previous</span>
    <span class="title">SdeOptions</span>
  </a>
  <a class="next-link" href="fluctua.html">
    <span class="label">Next</span>
    <span class="title">fluctua</span>
  </a>
</div>
<!-- apiref:nav:end -->
