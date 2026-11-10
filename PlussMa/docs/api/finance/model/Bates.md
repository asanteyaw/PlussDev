<!-- generated from Bates.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `model::Bates`

Heston with log-normal jumps in the asset: eight parameters.

The Heston dynamics plus a compound-Poisson jump in the asset: jumps arrive at rate `λ` (`jump_intensity`, per year), and each multiplies the price by `e^J` with `J ~ N(jump_mean, jump_vol²)`. The drift is compensated so the discounted price remains a martingale. Jumps add short-maturity skew and kurtosis that a diffusive stochastic-volatility model needs extreme parameters to produce. The vector form is `kappa, theta, sigma, rho, v0, jump_intensity, jump_mean, jump_vol`. Fft, Cos and Monte Carlo price it.

## Fields

### `kappa`

`pluss::Series`, default `undefined`.

Mean-reversion speed of the variance; as for Heston.

### `theta`

`pluss::Series`, default `undefined`.

Long-run variance; as for Heston.

### `sigma`

`pluss::Series`, default `undefined`.

Volatility of variance; as for Heston.

### `rho`

`pluss::Series`, default `undefined`.

Asset-variance correlation in (−1, 1).

### `v0`

`pluss::Series`, default `undefined`.

Initial variance, non-negative.

### `mu`

`pluss::Series`, default `undefined`.

Drift for the standalone simulators; ignored inside a `Field` (the drift is `rate − dividend_yield`).

### `variance_floor`

`pluss::Series`, default `undefined`.

Lower bound on the simulated variance (default `1e-12`); Monte Carlo only.

### `jump_intensity`

`pluss::Series`, default `undefined`.

Expected number of jumps per year `λ`, non-negative. With `λ = 0` the model is Heston.

### `jump_mean`

`pluss::Series`, default `undefined`.

Mean of the log jump size `J`. Negative means crashes: more downside skew.

### `jump_vol`

`pluss::Series`, default `undefined`.

Standard deviation of the log jump size, non-negative. Larger jumps fatten the tails.

## Guidance

Held by `Model` when its kind is `Bates`; accepted by `Field::price`.

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
                   .model = fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.5, -0.1, 0.2}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.5, 1.5, 3.0}) {
    fin::model::Bates m = field.model.as<fin::model::Bates>();
    m.kappa = pluss::make_series({v});
    out.writeln("kappa", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
kappa 0.50000 : price = 10.40305
kappa 1.50000 : price = 10.50872
kappa 3.00000 : price = 10.58068
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
                   .model = fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.5, -0.1, 0.2}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.02, 0.04, 0.08}) {
    fin::model::Bates m = field.model.as<fin::model::Bates>();
    m.theta = pluss::make_series({v});
    out.writeln("theta", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
theta 0.02000 : price = 9.68569
theta 0.04000 : price = 10.50872
theta 0.08000 : price = 11.95328
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
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.5, -0.1, 0.2}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.1, 0.3, 0.6}) {
    fin::model::Bates m = field.model.as<fin::model::Bates>();
    m.sigma = pluss::make_series({v});
    out.writeln("sigma", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
sigma 0.10000 : price = 10.63956
sigma 0.30000 : price = 10.50872
sigma 0.60000 : price = 10.09579
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
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 90.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.5, -0.1, 0.2}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {-0.9, -0.5, 0.0}) {
    fin::model::Bates m = field.model.as<fin::model::Bates>();
    m.rho = pluss::make_series({v});
    out.writeln("rho", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
rho -0.90000 : price = 16.70363
rho -0.50000 : price = 16.56662
rho 0.00000 : price = 16.33874
```

### Example 5

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
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.5, -0.1, 0.2}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.01, 0.04, 0.09}) {
    fin::model::Bates m = field.model.as<fin::model::Bates>();
    m.v0 = pluss::make_series({v});
    out.writeln("v0", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
v0 0.01000 : price = 9.17936
v0 0.04000 : price = 10.50872
v0 0.09000 : price = 12.36403
```

### Example 6

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
                   .model = fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.5, -0.1, 0.2}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.0, 0.4}) {
    fin::model::Bates m = field.model.as<fin::model::Bates>();
    m.mu = pluss::make_series({v});
    out.writeln("mu", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
mu 0.00000000 : price = 10.50872194
mu 0.40000000 : price = 10.50872194
```

### Example 7

A floor of 0.04 bounds the simulated variance.

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
                   .model = fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.5, -0.1, 0.2}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;

  fin::model::Bates m = field.model.as<fin::model::Bates>();
  m.variance_floor = pluss::make_series({0.04});
  out.writeln("smallest simulated variance =", field.simulate_paths(m).variance.amin().item<double>());
}
```

Output

```text
smallest simulated variance = 0.04000000
```

### Example 8

With no jumps Bates equals Heston; more jumps change the price.

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
                   .model = fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.5, -0.1, 0.2}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.0, 0.5, 2.0}) {
    fin::model::Bates m = field.model.as<fin::model::Bates>();
    m.jump_intensity = pluss::make_series({v});
    out.writeln("jump_intensity", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
jump_intensity 0.00000 : price = 8.62811
jump_intensity 0.50000 : price = 10.50872
jump_intensity 2.00000 : price = 14.81901
```

### Example 9

Price of a strike-90 call against the mean jump size.

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
                   .model = fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.5, -0.1, 0.2}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {-0.3, -0.1, 0.0, 0.1}) {
    fin::model::Bates m = field.model.as<fin::model::Bates>();
    m.jump_mean = pluss::make_series({v});
    out.writeln("jump_mean", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
jump_mean -0.30000 : price = 18.66615
jump_mean -0.10000 : price = 16.63946
jump_mean 0.00000 : price = 16.24284
jump_mean 0.10000 : price = 16.35827
```

### Example 10

Price of a wing option (strike 130) against the jump volatility.

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
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 130.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.5, -0.1, 0.2}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  for (const double v : {0.05, 0.2, 0.4}) {
    fin::model::Bates m = field.model.as<fin::model::Bates>();
    m.jump_vol = pluss::make_series({v});
    out.writeln("jump_vol", v, ": price =", field.price(m).price.items<double>()[0]);
  }
}
```

Output

```text
jump_vol 0.05000 : price = 0.66175
jump_vol 0.20000 : price = 1.37050
jump_vol 0.40000 : price = 4.22942
```

## See Also

- [`Model`](Model.md): holds one
- [`Kind`](Kind.md): the parameter order

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="BlackScholes.html">
    <span class="label">Next</span>
    <span class="title">BlackScholes</span>
  </a>
</div>
<!-- apiref:nav:end -->
