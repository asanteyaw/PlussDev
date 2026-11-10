<!-- generated from Heston.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `model::Heston`

The Heston stochastic-variance model: five parameters, plus a drift and a variance floor.

`dS = (r − q) S dt + √v S dW¹`, `dv = κ(θ − v) dt + σ √v dW²`, `d⟨W¹, W²⟩ = ρ dt`. The variance `v` reverts to the long-run level `θ` at speed `κ`; `σ` is the volatility of variance; `ρ` (usually negative for equities) makes the volatility rise as the asset falls, which skews the smile; `v₀` is the starting variance. The transform engines (Fft, Cos), the PDE engine and Monte Carlo price it. All five parameters are `Series` of one element for pricing; the vector form is `kappa, theta, sigma, rho, v0`. The Feller condition `2κθ ≥ σ²` keeps the variance strictly positive; violating it is allowed.

## Fields

### `kappa`

`pluss::Series`, default `undefined`.

Speed of mean reversion of the variance, positive. Large `κ` makes the variance snap back to `θ`, flattening the term structure of the smile.

### `theta`

`pluss::Series`, default `undefined`.

Long-run variance, positive. The level the variance reverts to; `√θ` is the long-run volatility.

### `sigma`

`pluss::Series`, default `undefined`.

Volatility of variance, positive. Fattens the tails of the return distribution and bends the smile.

### `rho`

`pluss::Series`, default `undefined`.

Correlation between the asset and variance shocks, in (−1, 1). Negative `ρ` skews the smile down (cheap upside, dear downside).

### `v0`

`pluss::Series`, default `undefined`.

Initial variance, non-negative. The volatility the market starts at today.

### `mu`

`pluss::Series`, default `undefined`.

A drift for the standalone simulators. Inside a `Field` the drift of the asset is always `rate − dividend_yield`, so this member is ignored when pricing or simulating through a `Field`.

### `variance_floor`

`pluss::Series`, default `undefined`.

Lower bound on the simulated variance (default `1e-12` when undefined), used by the Monte Carlo scheme to keep the variance from going negative. It has no effect on the transform engines.

## Guidance

Held by `Model` when its kind is `Heston`; accepted by `Field::price`.

## Practical Examples

### Example 1

Price against the reversion speed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.5, 1.5, 3.0, 6.0}) {
    fin::model::Heston m = field.model.as<fin::model::Heston>();
    m.kappa = pluss::make_series({v});
    out.writeln("kappa", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
kappa 0.50000 : price = 8.43111
kappa 1.50000 : price = 8.62811
kappa 3.00000 : price = 8.76157
kappa 6.00000 : price = 8.85446
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
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.02, 0.04, 0.06, 0.09}) {
    fin::model::Heston m = field.model.as<fin::model::Heston>();
    m.theta = pluss::make_series({v});
    out.writeln("theta", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
theta 0.02000 : price = 7.58928
theta 0.04000 : price = 8.62811
theta 0.06000 : price = 9.53541
theta 0.09000 : price = 10.73311
```

### Example 3

Price against the volatility of variance at the money, and a wing option (strike 130) where it matters more.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.1, 0.3, 0.6, 0.9}) {
    fin::model::Heston m = field.model.as<fin::model::Heston>();
    m.sigma = pluss::make_series({v});
    out.writeln("sigma", v, ": price =", field.price(m).price.items<double>()[0]);
  }
  mod::Field wing{.instrument = fin::instrument::make(fin::pricing::Call, 130.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.1, 0.3, 0.6, 0.9}) {
    fin::model::Heston m = wing.model.as<fin::model::Heston>();
    m.sigma = pluss::make_series({v});
    out.writeln("sigma", v, ": wing price =", wing.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
sigma 0.10000 : price = 8.88306
sigma 0.30000 : price = 8.62811
sigma 0.60000 : price = 7.95613
sigma 0.90000 : price = 7.27191
sigma 0.10000 : wing price = 0.91615
sigma 0.30000 : wing price = 0.42449
sigma 0.60000 : wing price = 0.20673
sigma 0.90000 : wing price = 0.17835
```

### Example 4

A negative correlation makes the out-of-the-money put dearer and the out-of-the-money call cheaper; use strike 90 calls to see the skew.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 90.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {-0.9, -0.5, 0.0, 0.5}) {
    fin::model::Heston m = field.model.as<fin::model::Heston>();
    m.rho = pluss::make_series({v});
    out.writeln("rho", v, ": K = 90 call =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
rho -0.90000 : K = 90 call = 15.21196
rho -0.50000 : K = 90 call = 15.00710
rho 0.00000 : K = 90 call = 14.69135
rho 0.50000 : K = 90 call = 14.27213
```

### Example 5

Price against the starting variance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.01, 0.04, 0.09, 0.16}) {
    fin::model::Heston m = field.model.as<fin::model::Heston>();
    m.v0 = pluss::make_series({v});
    out.writeln("v0", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
v0 0.01000 : price = 6.97530
v0 0.04000 : price = 8.62811
v0 0.09000 : price = 10.81270
v0 0.16000 : price = 13.23656
```

### Example 6

It is ignored: a wild `mu` leaves the price unchanged.

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
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.0, 0.3, -0.5}) {
    fin::model::Heston m = field.model.as<fin::model::Heston>();
    m.mu = pluss::make_series({v});
    out.writeln("mu", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
mu 0.00000000 : price = 8.62810810
mu 0.30000000 : price = 8.62810810
mu -0.50000000 : price = 8.62810810
```

### Example 7

A large floor visibly bounds the simulated variance from below.

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
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;

  for (const double floor : {1e-12, 0.04}) {
    fin::model::Heston m = field.model.as<fin::model::Heston>();
    m.variance_floor = pluss::make_series({floor});
    const mod::Paths p = field.simulate_paths(m);
    out.writeln("floor", floor, ": smallest simulated variance =", p.variance.amin().item<double>());
  }
}
```

Output

```text
floor 0.00000000 : smallest simulated variance = 0.00000000
floor 0.04000000 : smallest simulated variance = 0.04000000
```

## See Also

- [`Model`](Model.md): holds one
- [`Kind`](Kind.md): the parameter order

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="BlackScholes.html">
    <span class="label">Previous</span>
    <span class="title">BlackScholes</span>
  </a>
  <a class="next-link" href="Kind.html">
    <span class="label">Next</span>
    <span class="title">Kind</span>
  </a>
</div>
<!-- apiref:nav:end -->
