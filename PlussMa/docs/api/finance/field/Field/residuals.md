<!-- generated from residuals.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b940721c9282 -->

# `module::Field::residuals`

[Back to `module::Field`](../Field.md)

The model price minus the quote, for every option in the field.

## Signature

```cpp
pluss::Series module::Field::residuals(const pluss::Series &target_prices, pluss::Config cfg = {}) const;
```

## What It Does

Prices the field and subtracts `target_prices`, which must match the shape of the price (one quote per option). A positive residual means the model is too expensive for that option. Residuals are the raw material of a fit: look at them after a calibration to see where the model fails (typically at the wings of the smile), not just at the single number a loss reduces them to.

## Parameters

`const pluss::Series &target_prices`
: One quote per option.

`pluss::Config cfg`
: Device of the result.

## Returns

A `Series` shaped like the price.

## Errors

- An invalid field, or quotes whose shape does not match the prices, throws.

## Practical Examples

### Example 1

Residuals against made-up quotes.

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
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.02}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  const Series prices = field.price().price;
  const Series quotes = prices + pluss::make_series({0.30, -0.20, 0.10, -0.40, 0.25});

  out.writeln("residuals :", field.residuals(quotes).items<double>());
}
```

Output

```text
residuals : [-0.3000, 0.2000, -0.1000, 0.4000, -0.2500]
```

### Example 2

Zero when the quotes are the model's own prices.

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
  out.print_defaults(PrintOptions{}.fixed(12));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.02}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};

  const Series quotes = field.price().price;
  out.writeln("largest |residual| =", field.residuals(quotes).abs().max().item<double>());
}
```

Output

```text
largest |residual| = 0.000000000000
```

### Example 3

Where a model fails. Quotes made from a richer-skew Heston: a Black-Scholes field with one volatility has residuals that change sign across strikes, the signature of an unfitted smile.

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
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.02}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};

  mod::Field truth = field;
  truth.model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04});
  truth.controls.engine = mod::Engine::Cos;
  const Series quotes = truth.price().price;
  out.writeln("residuals :", field.residuals(quotes).items<double>());
}
```

Output

```text
residuals : [-0.5177, -0.3072, 0.2879, 0.8843, 1.0552]
```

### Example 4

A quote vector of the wrong shape is rejected.

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
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.02}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
  try {
    (void)(field.residuals(pluss::make_series({1.0, 2.0})));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
broadcast: incompatible shapes
```

## See Also

- [`Field`](../Field.md): the class
- [`loss`](loss.md): reduces them

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="price.html">
    <span class="label">Previous</span>
    <span class="title">price</span>
  </a>
  <a class="next-link" href="simulate.html">
    <span class="label">Next</span>
    <span class="title">simulate</span>
  </a>
</div>
<!-- apiref:nav:end -->
