<!-- generated from RoughHeston.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `model::RoughHeston`

A Heston-type model with a rough variance kernel: six parameters.

The variance follows a Volterra process driven by a fractional kernel with Hurst exponent `H = α + ½`, which is rough (`H < ½`, so `α ∈ (−0.5, 0)`). Rough models reproduce the steep short-maturity skew of equity options with few parameters. No closed-form characteristic function is used here: **Monte Carlo is the only engine**. The vector form is `kappa, theta, sigma, alpha, rho, v0`.

## Fields

### `kappa`

`pluss::Series`, default `undefined`.

Mean-reversion speed, positive.

### `theta`

`pluss::Series`, default `undefined`.

Long-run variance, positive.

### `sigma`

`pluss::Series`, default `undefined`.

Volatility of variance, positive.

### `alpha`

`pluss::Series`, default `undefined`.

The roughness `H − ½`, in (−0.5, 0). Closer to −0.5 is rougher.

### `rho`

`pluss::Series`, default `undefined`.

Correlation in (−1, 1).

### `v0`

`pluss::Series`, default `undefined`.

Initial variance, non-negative.

### `mu`

`pluss::Series`, default `undefined`.

Drift for the standalone simulators; ignored inside a `Field`.

### `variance_floor`

`pluss::Series`, default `undefined`.

Lower bound on the simulated variance (default `1e-12`).

## Guidance

Held by `Model` when its kind is `RoughHeston`; accepted by `Field::price`.

## Practical Examples

### Example 1

Price against the reversion speed (Monte Carlo, fixed seed).

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
                   .model = fin::model::make(fin::model::Kind::RoughHeston, {1.5, 0.04, 0.3, -0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;
  for (const double v : {0.5, 1.5, 4.0}) {
    fin::model::RoughHeston m = field.model.as<fin::model::RoughHeston>();
    m.kappa = pluss::make_series({v});
    out.writeln("kappa", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
kappa 0.5000 : price = 8.4454
kappa 1.5000 : price = 8.7117
kappa 4.0000 : price = 8.9003
```

### Example 2

Price against the long-run variance.

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
                   .model = fin::model::make(fin::model::Kind::RoughHeston, {1.5, 0.04, 0.3, -0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;
  for (const double v : {0.02, 0.04, 0.08}) {
    fin::model::RoughHeston m = field.model.as<fin::model::RoughHeston>();
    m.theta = pluss::make_series({v});
    out.writeln("theta", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
theta 0.0200 : price = 7.6020
theta 0.0400 : price = 8.7117
theta 0.0800 : price = 10.5462
```

### Example 3

Price against the volatility of variance.

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
                   .model = fin::model::make(fin::model::Kind::RoughHeston, {1.5, 0.04, 0.3, -0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;
  for (const double v : {0.1, 0.3, 0.6}) {
    fin::model::RoughHeston m = field.model.as<fin::model::RoughHeston>();
    m.sigma = pluss::make_series({v});
    out.writeln("sigma", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
sigma 0.1000 : price = 9.0153
sigma 0.3000 : price = 8.7117
sigma 0.6000 : price = 8.2870
```

### Example 4

Price of an out-of-the-money call (strike 110) against the roughness.

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
                   .model = fin::model::make(fin::model::Kind::RoughHeston, {1.5, 0.04, 0.3, -0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;
  for (const double v : {-0.45, -0.3, -0.1}) {
    fin::model::RoughHeston m = field.model.as<fin::model::RoughHeston>();
    m.alpha = pluss::make_series({v});
    out.writeln("alpha", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
alpha -0.4500 : price = 4.3146
alpha -0.3000 : price = 4.1108
alpha -0.1000 : price = 4.1254
```

### Example 5

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
                   .model = fin::model::make(fin::model::Kind::RoughHeston, {1.5, 0.04, 0.3, -0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;
  for (const double v : {-0.9, -0.5, 0.0}) {
    fin::model::RoughHeston m = field.model.as<fin::model::RoughHeston>();
    m.rho = pluss::make_series({v});
    out.writeln("rho", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
rho -0.9000 : price = 15.3267
rho -0.5000 : price = 15.1529
rho 0.0000 : price = 14.8811
```

### Example 6

Price against the initial variance.

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
                   .model = fin::model::make(fin::model::Kind::RoughHeston, {1.5, 0.04, 0.3, -0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;
  for (const double v : {0.01, 0.04, 0.09}) {
    fin::model::RoughHeston m = field.model.as<fin::model::RoughHeston>();
    m.v0 = pluss::make_series({v});
    out.writeln("v0", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
v0 0.0100 : price = 7.1806
v0 0.0400 : price = 8.7117
v0 0.0900 : price = 10.7571
```

### Example 7

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
                   .model = fin::model::make(fin::model::Kind::RoughHeston, {1.5, 0.04, 0.3, -0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;
  for (const double v : {0.0, 0.3}) {
    fin::model::RoughHeston m = field.model.as<fin::model::RoughHeston>();
    m.mu = pluss::make_series({v});
    out.writeln("mu", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
mu 0.00000000 : price = 8.71174743
mu 0.30000000 : price = 8.71174743
```

### Example 8

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
                   .model = fin::model::make(fin::model::Kind::RoughHeston, {1.5, 0.04, 0.3, -0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;

  fin::model::RoughHeston m = field.model.as<fin::model::RoughHeston>();
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
  <a class="prev-link" href="RoughBergomi.html">
    <span class="label">Previous</span>
    <span class="title">RoughBergomi</span>
  </a>
  <a class="next-link" href="Sabr.html">
    <span class="label">Next</span>
    <span class="title">Sabr</span>
  </a>
</div>
<!-- apiref:nav:end -->
