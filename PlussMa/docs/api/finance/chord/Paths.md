<!-- generated from Paths.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `module::Chord::Paths`

The simulated paths of all assets of a Chord, with the asset axis last.

## Signature

```cpp
module::Chord::Paths module::Chord::simulate_paths(Config cfg = {}) const;
```

A `Chord::Paths` is returned by value from [`Chord::simulate_paths`](Chord/simulate_paths.md). All four `Series` have one entry per path first and one per asset last: `asset` and `variance` are `[paths, steps + 1, assets]` (the first column is the starting value) and `asset_shocks` and `vol_shocks` are `[paths, steps, assets]`, the standard normals that drove the price and the variance of each asset, kept so that estimators can reweight them. For a Heston, Bates, rough Heston or rough Bergomi asset `variance` is its variance process; for a Black-Scholes asset it is the constant σ², and for a SABR asset `asset` is the forward and `variance` is α², the square of its volatility factor; `vol_shocks` holds the shock of the second factor of each (zero-correlation filler for a Black-Scholes asset, which has none). Select an asset with `select(2, i)` and a date with `select(1, t)`. Every example below simulates two Heston assets with 20,000 paths and 20 steps.

## Fields

### `names`

`std::vector<std::string>`, default `empty`.

The asset on the last axis, in the order the assets were added: `names[i]` is index *i* of the last axis of every array.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston_mc(double spot, double strike) {
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({strike}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 20000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 3;
  return f;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord chord;
  chord.add("a", heston_mc(100.0, 100.0)).add("b", heston_mc(90.0, 90.0));
  const auto p = chord.simulate_paths();
  out.writeln(p.names, " asset axis size", p.asset.size(2));
}
```

Output

```text
[a, b]  asset axis size 2
```

### `asset`

`Series` of shape `[paths, steps + 1, assets]`.

The simulated asset prices. The first column is the starting spot of each asset, and the last the price at the horizon. Each asset's terminal mean is its forward, since the simulation is risk neutral with growth `rate − dividend_yield`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston_mc(double spot, double strike) {
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({strike}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 20000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 3;
  return f;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Chord chord;
  chord.add("a", heston_mc(100.0, 100.0)).add("b", heston_mc(90.0, 90.0));
  const auto p = chord.simulate_paths();
  out.writeln("shape", p.asset.sizes());
  out.writeln("starting values", p.asset.select(1, 0).select(1, 0).mean().item<double>(), p.asset.select(1, 0).select(1, 1).mean().item<double>());
  out.writeln("mean at the horizon", p.asset.select(1, 20).select(1, 0).mean().item<double>(), " forward 100 * exp(0.02) =", 100.0 * std::exp(0.02));
}
```

Output

```text
shape [20000, 21, 2]
starting values 100.0000 90.0000
mean at the horizon 102.1956  forward 100 * exp(0.02) = 102.0201
```

### `variance`

`Series` of shape `[paths, steps + 1, assets]`.

The instantaneous variance of each asset along each path. The first column is the model's initial variance `v0`; the variance is mean reverting, so the mean over paths drifts towards the long-run level.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston_mc(double spot, double strike) {
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({strike}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 20000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 3;
  return f;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Chord chord;
  chord.add("a", heston_mc(100.0, 100.0)).add("b", heston_mc(90.0, 90.0));
  const auto p = chord.simulate_paths();
  out.writeln("shape", p.variance.sizes());
  out.writeln("starting variance", p.variance.select(1, 0).select(1, 0).mean().item<double>(), " mean at the horizon", p.variance.select(1, 20).select(1, 0).mean().item<double>());
}
```

Output

```text
shape [20000, 21, 2]
starting variance 0.0400  mean at the horizon 0.0398
```

### `asset_shocks`

`Series` of shape `[paths, steps, assets]`.

The standard normal shocks that drove the price of each asset, correlated across assets by `price_corr`. They are kept so that an estimator can reweight or reuse them. Their sample mean is near zero and their sample variance near one, whatever the correlation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston_mc(double spot, double strike) {
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({strike}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 20000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 3;
  return f;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Chord chord;
  chord.add("a", heston_mc(100.0, 100.0)).add("b", heston_mc(90.0, 90.0));
  const auto p = chord.simulate_paths();
  out.writeln("shape", p.asset_shocks.sizes());
  const auto shocks = p.asset_shocks.select(2, 0);
  out.writeln("mean", shocks.mean().item<double>(), " variance", shocks.square().mean().item<double>());
}
```

Output

```text
shape [20000, 20, 2]
mean 0.0023  variance 1.0027
```

### `vol_shocks`

`Series` of shape `[paths, steps, assets]`.

The standard normal shocks that drove the variance of each asset. Within an asset they are correlated with the price shocks at the model's ρ; across assets by the matrices of the Chord.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston_mc(double spot, double strike) {
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({strike}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 20000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 3;
  return f;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Chord chord;
  chord.add("a", heston_mc(100.0, 100.0)).add("b", heston_mc(90.0, 90.0));
  const auto p = chord.simulate_paths();
  out.writeln("shape", p.vol_shocks.sizes());
  out.writeln("correlation of the price and variance shocks of asset a", p.asset_shocks.select(2, 0).mul(p.vol_shocks.select(2, 0)).mean().item<double>(), " (the model has rho = -0.7)");
}
```

Output

```text
shape [20000, 20, 2]
correlation of the price and variance shocks of asset a -0.7025  (the model has rho = -0.7)
```

### `horizon`

`double`, default `0.0`.

The time of the last column in years: the longest maturity among the assets. The step is `horizon / steps`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston_mc(double spot, double strike) {
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({strike}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 20000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 3;
  return f;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Chord chord;
  chord.add("a", heston_mc(100.0, 100.0)).add("b", heston_mc(90.0, 90.0));
  const auto p = chord.simulate_paths();
  out.writeln("horizon", p.horizon, " steps", p.asset_shocks.size(1), " step size", p.horizon / static_cast<double>(p.asset_shocks.size(1)));
}
```

Output

```text
horizon 1.0000  steps 20  step size 0.0500
```

## See Also

[`Chord::simulate_paths`](Chord/simulate_paths.md) produces it, and [`Chord::Result`](Result.md) is the result of pricing.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Parameters.html">
    <span class="label">Previous</span>
    <span class="title">Parameters</span>
  </a>
</div>
<!-- apiref:nav:end -->
