<!-- generated from price.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c5334dc4fc48 -->

# `module::Field::price`

[Back to `module::Field`](../Field.md)

Prices every option in the field with the chosen engine and returns the prices, and the Greeks if asked.

## Signature

```cpp
module::Field::Result module::Field::price(pluss::Config cfg = {}) const;
module::Field::Result module::Field::price(const module::Model &candidate, pluss::Config cfg = {}) const;
template <SupportedModel M> module::Field::Result module::Field::price(const M &model, pluss::Config cfg = {}) const;
```

## What It Does

Validates the field, then prices every option in the strip with `controls.engine` and returns a [`module::Field::Result`](../Result.md): the prices, the five Greeks when `controls.risk` is `All` (undefined otherwise), and the strike grid for the FFT engine.

- `price()` prices the held `model`.
- `price(candidate)` prices a different [`Model`](../../model/Model.md) in the same instrument, market and controls **without changing the field**: this is what a calibration calls at every trial of the parameters.
- `price(model)` is the same with a concrete model struct (`model::Heston`, ...) in place of a `Model`.

**Which engine for which model.** Black-Scholes: Analytic, Cos, MonteCarlo, Lattice, Spectral. SABR: Analytic, MonteCarlo. Heston: Fft, Cos, Pde, MonteCarlo. Bates: Fft, Cos, MonteCarlo. Rough Heston and rough Bergomi: MonteCarlo only. Asking for another combination throws an error that lists the supported engines. American and Bermudan exercise narrows the choice further (see [`Vanilla`](../../instrument/Vanilla.md)).

**Greeks.** With `Risk::All` the Analytic engine on Black-Scholes returns the closed-form Greeks; the Heston and Bates COS engine returns them in closed form from the same pass over the characteristic function as the price; the FFT engine adds three transforms; the lattice and the Heston PDE read most of them off the solved grid; Monte Carlo reads them from the same paths (Heston, Bates and the rough models) or pathwise (Black-Scholes); SABR differentiates Black's formula exactly; the American spectral and COS engines read most of them off the solved boundary or the final expansion; only Monte Carlo with American exercise reprices at shifted inputs, at the cost of about nine pricings. Use `PriceOnly` in any loop where you do not need them. `cfg` selects the device of the result.

## Parameters

`const module::Model &candidate`
: Another model to price in the same instrument, market and controls.

`const M &model`
: A concrete model struct (`model::Heston`, ...).

`pluss::Config cfg`
: Device of the result.

## Returns

A [`module::Field::Result`](../Result.md).

## Errors

- An invalid field, or an engine that does not support the model or the exercise style, throws (`pluss::Error`) with every problem listed.

## Practical Examples

### Example 1

A Black-Scholes strip by the analytic engine. One option, price and delta; the price agrees with the plain Black-Scholes function.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.05, 0.02),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::All}};

  const auto result = field.price();
  out.writeln("field price :", result.price.items<double>(), ", delta :", result.greeks.delta.items<double>());
  out.writeln("closed form :", [&] {   // closed form, for comparison
    const double d1 = (std::log(100.0 / 100.0) + (0.05 - 0.02 + 0.5 * 0.04) * 1.0) / 0.2, d2 = d1 - 0.2;
    const auto N = [](double x) { return 0.5 * std::erfc(-x / std::sqrt(2.0)); };
    return 100.0 * std::exp(-0.02) * N(d1) - 100.0 * std::exp(-0.05) * N(d2);
  }());
}
```

Output

```text
field price : [9.22700551] , delta : [0.58685115]
closed form : 9.22700551
```

### Example 2

A strip of strikes. Five strikes with a one-element spot, rate and maturity are priced together: five prices come back, falling as the strike rises.

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
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.05}), .dividend_yield = pluss::make_series({0.02})},
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};

  const auto result = field.price();
  out.writeln("strikes :", field.instrument.strike.items<double>());
  out.writeln("prices  :", result.price.items<double>());
}
```

Output

```text
strikes : [80.0000, 90.0000, 100.0000, 110.0000, 120.0000]
prices  : [22.7641, 15.1237, 9.2270, 5.1886, 2.7118]
```

### Example 3

The same field, another engine. Heston priced by the Fourier-cosine and the FFT engines: two different methods, one answer.

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

  const double cos = field.price().price.items<double>()[0];
  field.controls.engine = mod::Engine::Fft;
  const double fft = field.price().price.items<double>()[0];
  out.writeln("COS =", cos, ", FFT =", fft, ", difference =", cos - fft);
}
```

Output

```text
COS = 8.62810810 , FFT = 8.62810810 , difference = 0.00000000
```

### Example 4

Pricing a candidate. A different mean-reversion speed is priced without changing the field: the field's own model is untouched.

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

  const double base = field.price().price.items<double>()[0];
  const fin::module::Model candidate = fin::model::make(fin::model::Kind::Heston, {4.0, 0.04, 0.3, -0.7, 0.04});
  out.writeln("held model :", base, ", candidate :", field.price(candidate).price.items<double>()[0]);
  out.writeln("field still prices to", field.price().price.items<double>()[0]);
}
```

Output

```text
held model : 8.62810810 , candidate : 8.80668548
field still prices to 8.62810810
```

### Example 5

Pricing a concrete model struct. Copy the held Heston parameters out with `as`, change one, and price that.

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

  fin::model::Heston tweaked = field.model.as<fin::model::Heston>();
  tweaked.rho = pluss::make_series({-0.2});
  out.writeln("rho = -0.7 :", field.price().price.items<double>()[0], ", rho = -0.2 :", field.price(tweaked).price.items<double>()[0]);
}
```

Output

```text
rho = -0.7 : 8.62810810 , rho = -0.2 : 8.66112985
```

### Example 6

Greeks. With `Risk::All` the result carries the five Greeks; with `PriceOnly` it does not (the members are undefined), and the pricing is cheaper.

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
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::All}};

  const auto all = field.price();
  field.controls.risk = mod::Risk::PriceOnly;
  const auto only = field.price();
  out.writeln("All      : delta defined =", all.greeks.delta.defined(), ", vega =", all.greeks.vega.items<double>());
  out.writeln("PriceOnly: delta defined =", only.greeks.delta.defined());
}
```

Output

```text
All      : delta defined = true , vega = [37.901158]
PriceOnly: delta defined = false
```

### Example 7

An engine the model does not support is refused with the list of those it does.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.05, 0.02),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::All}};

  field.controls.engine = mod::Engine::Fft;
  try {
    (void)(field.price());
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::module::Field: controls.engine Fft is not supported for model BlackScholes. Supported engines: Analytic, Cos, MonteCarlo, Lattice, Spectral
```

### Example 8

Every problem is reported together. A field with a non-positive strike and a negative volatility fails once, listing both.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, -5.0, 1.0),
                   .market = fin::market::make(100.0, 0.05, 0.02),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::All}};

  field.model = fin::model::BlackScholes{.volatility = pluss::make_series({-0.2})};
  try {
    (void)(field.price());
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find("Triggered")));
  }
}
```

Output

```text
subsp::math::finance::module::Field (2 problems):
  - instrument.strike must be > 0
  - model.BlackScholes.volatility must be > 0
```

## See Also

- [`Field`](../Field.md): the class
- [`validate`](validate.md): check without pricing
- [`Controls`](../Controls.md): the engine and its settings

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="objective.html">
    <span class="label">Previous</span>
    <span class="title">objective</span>
  </a>
  <a class="next-link" href="residuals.html">
    <span class="label">Next</span>
    <span class="title">residuals</span>
  </a>
</div>
<!-- apiref:nav:end -->
