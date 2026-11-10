<!-- generated from RoughBergomi.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `model::RoughBergomi`

The rough Bergomi model: four parameters, plus a drift and a variance floor.

The instantaneous variance is `ξ exp(η W^H_t − ½ η² t^{2H})`, an exponential of a fractional Brownian motion with Hurst exponent `H = α + ½`, so `ξ` is the (flat) forward variance level, `η` the volatility of volatility and `α ∈ (−0.5, 0)` the roughness. The asset correlates with the driving noise through `ρ`. As with rough Heston, Monte Carlo is the only engine. The vector form is `xi, eta, alpha, rho`.

## Fields

### `xi`

`pluss::Series`, default `undefined`.

Forward variance level `ξ`, positive; `√ξ` is the starting volatility.

### `eta`

`pluss::Series`, default `undefined`.

Volatility of volatility `η`, positive.

### `alpha`

`pluss::Series`, default `undefined`.

The roughness `H − ½`, in (−0.5, 0).

### `rho`

`pluss::Series`, default `undefined`.

Correlation in (−1, 1).

### `mu`

`pluss::Series`, default `undefined`.

Drift for the standalone simulators; ignored inside a `Field`.

### `variance_floor`

`pluss::Series`, default `undefined`.

Lower bound on the simulated variance (default `1e-12`).

## Guidance

Held by `Model` when its kind is `RoughBergomi`; accepted by `Field::price`.

## Practical Examples

### Example 1

Price against the forward variance.

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
                   .model = fin::model::make(fin::model::Kind::RoughBergomi, {0.04, 1.5, -0.1, -0.7}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;
  for (const double v : {0.02, 0.04, 0.08}) {
    fin::model::RoughBergomi m = field.model.as<fin::model::RoughBergomi>();
    m.xi = pluss::make_series({v});
    out.writeln("xi", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
xi 0.0200 : price = 6.0710
xi 0.0400 : price = 7.9980
xi 0.0800 : price = 10.6749
```

### Example 2

Price of a wing call (strike 130) against `η`.

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
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 130.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::RoughBergomi, {0.04, 1.5, -0.1, -0.7}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;
  for (const double v : {0.5, 1.5, 3.0}) {
    fin::model::RoughBergomi m = field.model.as<fin::model::RoughBergomi>();
    m.eta = pluss::make_series({v});
    out.writeln("eta", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
eta 0.5000 : price = 0.7308
eta 1.5000 : price = 0.2381
eta 3.0000 : price = 0.1022
```

### Example 3

Price of a strike-110 call against the roughness.

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
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 110.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::RoughBergomi, {0.04, 1.5, -0.1, -0.7}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;
  for (const double v : {-0.45, -0.3, -0.1}) {
    fin::model::RoughBergomi m = field.model.as<fin::model::RoughBergomi>();
    m.alpha = pluss::make_series({v});
    out.writeln("alpha", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
alpha -0.4500 : price = 3.7971
alpha -0.3000 : price = 3.2170
alpha -0.1000 : price = 3.2501
```

### Example 4

Price of a strike-90 call against the correlation.

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
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 90.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::RoughBergomi, {0.04, 1.5, -0.1, -0.7}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;
  for (const double v : {-0.9, -0.5, 0.0}) {
    fin::model::RoughBergomi m = field.model.as<fin::model::RoughBergomi>();
    m.rho = pluss::make_series({v});
    out.writeln("rho", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
rho -0.9000 : price = 14.8979
rho -0.5000 : price = 14.7827
rho 0.0000 : price = 14.4112
```

### Example 5

Ignored: the price does not move with `mu`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::RoughBergomi, {0.04, 1.5, -0.1, -0.7}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;
  for (const double v : {0.0, 0.3}) {
    fin::model::RoughBergomi m = field.model.as<fin::model::RoughBergomi>();
    m.mu = pluss::make_series({v});
    out.writeln("mu", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
mu 0.00000000 : price = 7.99797925
mu 0.30000000 : price = 7.99797925
```

### Example 6

A floor of 0.04 bounds the simulated variance below.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::RoughBergomi, {0.04, 1.5, -0.1, -0.7}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;

  fin::model::RoughBergomi m = field.model.as<fin::model::RoughBergomi>();
  m.variance_floor = pluss::make_series({0.04});
  out.writeln("smallest simulated variance =", field.simulate_paths(m).variance.amin().item<double>());
}
```

Output

```text
smallest simulated variance = 0.04000000
```

## See Also

- [`Model`](Model.md): holds one
- [`Kind`](Kind.md): the parameter order

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Model.html">
    <span class="label">Previous</span>
    <span class="title">Model</span>
  </a>
  <a class="next-link" href="RoughHeston.html">
    <span class="label">Next</span>
    <span class="title">RoughHeston</span>
  </a>
</div>
<!-- apiref:nav:end -->
