<!-- generated from Paths.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `module::Paths`

The simulated paths of a Monte Carlo run: asset, volatility, variance and the shocks that drove them.

`simulate` stores a `Paths` in `field.paths`. All members share a layout with one row per path and one column per time point on a uniform grid from 0 to the longest maturity: `asset`, `variance` and `vol` have `steps + 1` columns (column 0 is the initial value), and the two shock arrays have `steps` columns. `vol` is `√max(variance, 0)`. For Black-Scholes the variance is the constant `σ²`. The shocks are the standard normals that drove the asset and the (correlated) variance, kept so that a run can be audited or reused. Reduce along the time axis with `axis = 1` and across paths with `axis = 0`.

## Fields

### `asset`

`pluss::Series`, default `undefined`.

`[paths, steps + 1]`: the asset price on the grid; column 0 is the spot.

### `vol`

`pluss::Series`, default `undefined`.

`[paths, steps + 1]`: `√max(variance, 0)`.

### `variance`

`pluss::Series`, default `undefined`.

`[paths, steps + 1]`: the instantaneous variance (for Black-Scholes, the constant `σ²`).

### `asset_shocks`

`pluss::Series`, default `undefined`.

`[paths, steps]`: the standard normals that drove the asset.

### `vol_shocks`

`pluss::Series`, default `undefined`.

`[paths, steps]`: the correlated normals that drove the variance.

## Guidance

Stored in `Field::paths`; returned by `simulate_paths`.

## Practical Examples

### Example 1

The five members and their shapes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 500;
  field.controls.mc.steps = 20;
  field.controls.mc.seed = 2;

  field.simulate();
  const mod::Paths &p = field.paths;
  out.writeln("asset", p.asset.sizes(), " vol", p.vol.sizes(), " variance", p.variance.sizes());
  out.writeln("asset_shocks", p.asset_shocks.sizes(), " vol_shocks", p.vol_shocks.sizes());
}
```

Output

```text
asset [500, 21]  vol [500, 21]  variance [500, 21]
asset_shocks [500, 20]  vol_shocks [500, 20]
```

### Example 2

Column 0 is the starting point: the spot and the initial variance for every path.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

static double bs(fin::pricing::OptionType type, double S, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = fin::market::make(S, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price.items<double>()[0];
}
static pluss::Series bsv(fin::pricing::OptionType type, const pluss::Series &spots, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = {.spot = spots, .rate = pluss::make_series({r}), .dividend_yield = pluss::make_series({q})},
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price;
}
static pluss::Series bst(fin::pricing::OptionType type, const pluss::Series &spots, const pluss::Series &tau, double K, double r, double q, double vol) {
  fin::module::Field f{.instrument = {.type = type, .strike = pluss::make_series({K}), .maturity = tau}, .market = {.spot = spots, .rate = pluss::make_series({r}), .dividend_yield = pluss::make_series({q})},
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price;
}
static fin::hedging::DeltaHedgeResult hedge(const pluss::Series &path, fin::pricing::OptionType type, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = fin::market::make(100.0, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic}};
  return fin::hedging::delta_hedge(f, path);
}
static double iv(double price, double S, double K, double r, double q, double T, const fin::pricing::ImpliedVolOptions &o = {}) {
  fin::module::Field f{.instrument = fin::instrument::make(fin::pricing::Call, K, T), .market = fin::market::make(S, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({0.2})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.implied_vol(pluss::make_series({price}), o).items<double>()[0];
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 500;
  field.controls.mc.steps = 20;
  field.controls.mc.seed = 2;
  field.simulate();

  out.writeln("asset[:,0] all 100:", (field.paths.asset.select(1, 0) - 100.0).abs().max().item<double>() == 0.0, ", variance[:,0] all 0.04:", (field.paths.variance.select(1, 0) - 0.04).abs().max().item<double>() == 0.0);
}
```

Output

```text
asset[:,0] all 100: true , variance[:,0] all 0.04: true
```

### Example 3

The shocks are standard normals with the model's correlation between the asset and variance streams (here `ρ = −0.7`).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 40000;
  field.controls.mc.steps = 10;
  field.controls.mc.seed = 4;
  field.simulate();

  const Series zs = field.paths.asset_shocks, zv = field.paths.vol_shocks;
  out.writeln("std", zs.std().item<double>(), zv.std().item<double>(), " correlation", ((zs * zv).mean() / (zs.std() * zv.std())).item<double>());
}
```

Output

```text
std 0.998 0.998  correlation -0.699
```

### Example 4

A path-dependent question from the stored paths: the probability that the asset touched 120 before maturity.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 100;
  field.controls.mc.seed = 4;
  field.simulate();

  const Series highest = field.paths.asset.amax(1, false);
  out.writeln("P(max S >= 120) =", (highest >= 120.0).to(pluss::ScalarType::Float64).mean().item<double>());
}
```

Output

```text
P(max S >= 120) = 0.2757
```

## See Also

- [`simulate`](Field/simulate.md): fills it
- [`Field`](Field.md): stores it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Loss.html">
    <span class="label">Previous</span>
    <span class="title">Loss</span>
  </a>
  <a class="next-link" href="Result.html">
    <span class="label">Next</span>
    <span class="title">Result</span>
  </a>
</div>
<!-- apiref:nav:end -->
