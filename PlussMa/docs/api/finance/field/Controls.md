<!-- generated from Controls.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `module::Controls`

The engine, whether Greeks are wanted, the loss, and the settings of every engine side by side.

`engine` and `risk` decide how a [`Field`](Field.md) is priced; `loss` and `huber_delta` decide how a fit is scored; and the seven engine members (`fft`, `cos`, `mc`, `analytic`, `lattice`, `spectral`, `pde`) hold each engine's own settings. They exist side by side, so switching `engine` never loses the settings of another and one `Controls` can be reused across engines. The defaults are chosen so that an unconfigured field prices: `Analytic` engine, `All` Greeks, `MSE` loss.

## Fields

### `engine`

`module::Engine`, default `Analytic`.

How to price. The default prices Black-Scholes and SABR only; set it for any other model.

### `risk`

`module::Risk`, default `All`.

`All` returns the Greeks with the price; `PriceOnly` does not. Use `PriceOnly` in loops.

### `loss`

`module::Loss`, default `MSE`.

How residuals are reduced to a number by `loss`, `objective` and friends.

### `huber_delta`

`double`, default `0.05`.

The threshold of the `Huber` loss; must be positive.

### `fft`

`pricer::Fft`, default `{}`.

Settings of the FFT engine (Heston, Bates).

### `cos`

`pricer::COS`, default `{}`.

Settings of the cosine-expansion engine, including early exercise.

### `mc`

`pricer::MonteCarlo`, default `{}`.

Settings of the simulation engine: paths, steps, scheme, random numbers, variance reduction, seed.

### `analytic`

`pricer::Analytic`, default `{}`.

Settings of the closed-form engines.

### `lattice`

`pricer::Lattice`, default `{}`.

Settings of the binomial tree: steps.

### `spectral`

`pricer::Spectral`, default `{}`.

Settings of the collocation engine.

### `pde`

`pricer::Pde`, default `{}`.

Settings of the Heston finite-difference engine.

## Guidance

Member `Field::controls`.

## Practical Examples

### Example 1

The engine. The default is Analytic, which prices only Black-Scholes and SABR; naming another engine is how a Heston field is priced.

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
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};

  mod::Controls defaults;
  out.writeln("default engine:", defaults.engine == mod::Engine::Analytic, "; this field uses", field.engine_name());
}
```

Output

```text
default engine: true ; this field uses Cos
```

### Example 2

The risk switch. The default asks for the Greeks.

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
  mod::Controls defaults;
  out.writeln("default risk is All:", defaults.risk == mod::Risk::All, ", default loss is MSE:", defaults.loss == mod::Loss::MSE);
}
```

Output

```text
default risk is All: true , default loss is MSE: true
```

### Example 3

The loss. Switching the loss changes the score of the same residuals.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.05}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};

  const Series quotes = field.price().price + pluss::make_series({0.1, -0.1, 0.1, -0.1, 0.4});
  field.controls.loss = mod::Loss::MSE;
  const double mse = field.loss_value(quotes);
  field.controls.loss = mod::Loss::MAE;
  out.writeln("MSE =", mse, ", MAE =", field.loss_value(quotes));
}
```

Output

```text
MSE = 0.040000 , MAE = 0.160000
```

### Example 4

The Huber threshold: a larger `huber_delta` extends the quadratic region, so the loss moves toward half the MSE.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.05}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};

  const Series quotes = field.price().price + pluss::make_series({0.1, -0.1, 0.1, -0.1, 0.4});
  field.controls.loss = mod::Loss::Huber;
  for (const double delta : {0.05, 0.2, 1.0}) {
    field.controls.huber_delta = delta;
    out.writeln("huber_delta", delta, ": loss =", field.loss_value(quotes));
  }
}
```

Output

```text
huber_delta 0.050000 : loss = 0.006750
huber_delta 0.200000 : loss = 0.016000
huber_delta 1.000000 : loss = 0.020000
```

### Example 5

The engine settings live side by side. Raising the Monte Carlo path count does not touch the COS term count, and switching engines keeps both.

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
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};

  field.controls.cos.num_terms = 256;
  field.controls.mc.paths = 5000;
  field.controls.engine = mod::Engine::MonteCarlo;
  field.controls.engine = mod::Engine::Cos;
  out.writeln("cos terms still", field.controls.cos.num_terms, ", mc paths still", field.controls.mc.paths);
}
```

Output

```text
cos terms still 256 , mc paths still 5000
```

### Example 6

Each engine's settings change that engine's price. The COS term count: a coarse expansion is visibly worse than the default.

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

  for (const std::int64_t terms : {16, 64, 1024}) {
    field.controls.cos.num_terms = terms;
    out.writeln("num_terms", terms, ": price =", field.price().price.items<double>()[0]);
  }
}
```

Output

```text
num_terms 16 : price = 7.79718574
num_terms 64 : price = 8.62803387
num_terms 1024 : price = 8.62810810
```

### Example 7

The FFT settings: the number of transform points.

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
                   .controls = {.engine = mod::Engine::Fft, .risk = mod::Risk::PriceOnly}};

  for (const std::int64_t points : {512, 4096, 16384}) {
    field.controls.fft.num_fft = points;
    out.writeln("num_fft", points, ": price =", field.price().price.items<double>()[0]);
  }
}
```

Output

```text
num_fft 512 : price = 8.62810924
num_fft 4096 : price = 8.62810810
num_fft 16384 : price = 8.62810810
```

### Example 8

The Monte Carlo settings. More paths shrink the sampling error, but the time-discretisation bias of the Euler scheme stays: the estimates scatter about, and settle slightly below, the transform price 8.628.

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
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};

  for (const std::int64_t paths : {1000, 10000, 100000}) {
    field.controls.mc.paths = paths;
    field.controls.mc.seed = 5;
    out.writeln("paths", paths, ": price =", field.price().price.items<double>()[0]);
  }
}
```

Output

```text
paths 1000 : price = 8.67218
paths 10000 : price = 8.54625
paths 100000 : price = 8.55584
```

### Example 9

The lattice settings: the tree depth for Black-Scholes. More steps converge to the closed form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.05, 0.02),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Lattice, .risk = mod::Risk::PriceOnly}};

  for (const std::int64_t steps : {50, 500, 5000}) {
    field.controls.lattice.steps = steps;
    out.writeln("steps", steps, ": price =", field.price().price.items<double>()[0]);
  }
  field.controls.engine = mod::Engine::Analytic;
  out.writeln("closed form (Analytic engine):", field.price().price.items<double>()[0]);
}
```

Output

```text
steps 50 : price = 9.188225
steps 500 : price = 9.223118
steps 5000 : price = 9.226617
closed form (Analytic engine): 9.227006
```

### Example 10

The PDE settings: the grid of the Heston finite-difference solver.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Pde, .risk = mod::Risk::PriceOnly}};

  for (const std::int64_t nodes : {60, 120, 240}) {
    field.controls.pde.spot_nodes = nodes;
    out.writeln("spot_nodes", nodes, ": price =", field.price().price.items<double>()[0]);
  }
}
```

Output

```text
spot_nodes 60 : price = 8.618509
spot_nodes 120 : price = 8.625652
spot_nodes 240 : price = 8.627448
```

### Example 11

The spectral settings: collocation nodes of the American-boundary solver (Black-Scholes). An American put is priced; the answer is already stable at 8 nodes, so more nodes change it only in the eighth decimal.

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
  mod::Field field{.instrument = fin::instrument::american(fin::pricing::Put, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.05, 0.0),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Spectral, .risk = mod::Risk::PriceOnly}};
  for (const std::int64_t nodes : {8, 24, 48}) {
    field.controls.spectral.nodes = nodes;
    out.writeln("nodes", nodes, ": price =", field.price().price.items<double>()[0]);
  }
}
```

Output

```text
nodes 8 : price = 6.09037154
nodes 24 : price = 6.09037059
nodes 48 : price = 6.09037059
```

### Example 12

The analytic setting `with_greeks` is on by default.

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
  mod::Controls c;
  out.writeln("analytic.with_greeks =", c.analytic.with_greeks);
}
```

Output

```text
analytic.with_greeks = true
```

## See Also

- [`Field`](Field.md): holds it
- [`Engine`](Engine.md): the engines

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Engine.html">
    <span class="label">Next</span>
    <span class="title">Engine</span>
  </a>
</div>
<!-- apiref:nav:end -->
