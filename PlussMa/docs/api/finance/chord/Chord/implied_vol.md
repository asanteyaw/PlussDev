<!-- generated from implied_vol.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4b24cd33fca1 -->

# `module::Chord::implied_vol`

[Back to `module::Chord`](../Chord.md)

The Black-Scholes implied volatilities of every asset's own model prices.

## Signature

```cpp
std::vector<Series> module::Chord::implied_vol(const pricing::ImpliedVolOptions &options = {}, Config cfg = {}) const;
```

## What It Does

Prices the Chord with the batching of [`price`](price.md) and inverts each asset's prices with [`Field::implied_vol`](../../field/Field/implied_vol.md), returning one `Series` per asset in the order of adding, each with one entry per option of that asset. A Black-Scholes asset returns its own volatility; any other model returns its smile, the volatility that a Black-Scholes model would need to give the same price at each strike. Only European assets have a Black-Scholes implied volatility.

Call it to compare the smiles of the assets in a book on one scale, or to check that a model reproduces a quoted volatility. Where a price cannot be inverted (below the lower bound or above the upper bound for its parameters) the entry is NaN, as for the single-asset method.

## Parameters

`const pricing::ImpliedVolOptions &options`
: The root-finding settings: method, volatility bracket, tolerance, iteration limit; see [`ImpliedVolOptions`](../../implied/ImpliedVolOptions.md).

`Config cfg`
: Device and dtype of the results.

## Returns

One `Series` per asset, in the order of adding, each of the length of that asset's strike strip.

## Errors

Throws the report of [`validate`](validate.md) for an invalid asset, and `subsp::math::finance::module::Field::implied_vol: only European instruments have a Black-Scholes implied volatility` when an asset is not European.

## Practical Examples

### Example 1: a flat asset and a skewed one

A Black-Scholes asset returns its own 20% at every strike; a Heston asset with negative correlation returns a skew that falls with the strike.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field bs(double spot, double strike, double vol, double maturity = 1.0) {
  return mod::Field{.instrument = fin::instrument::make(fin::pricing::Call, strike, maturity), .market = fin::market::make(spot, 0.02, 0.01),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})}, .controls = {.engine = mod::Engine::Analytic}};
}

static mod::Field heston(double spot, std::vector<double> strikes, mod::Engine engine = mod::Engine::Cos) {
  return mod::Field{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series(strikes), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
                    .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = engine, .risk = mod::Risk::PriceOnly}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord chord;
  chord.add("flat", bs(100.0, 100.0, 0.20)).add("smile", heston(100.0, {90.0, 100.0, 110.0}));
  const auto vols = chord.implied_vol();
  out.writeln("flat ", vols[0].items<double>());
  out.writeln("smile", vols[1].items<double>());
}
```

Output

```text
flat  [0.20000]
smile [0.20994, 0.19264, 0.17677]
```

### Example 2: the order of the assets

One series per asset, in the order of adding, whatever the strip lengths.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field bs(double spot, double strike, double vol, double maturity = 1.0) {
  return mod::Field{.instrument = fin::instrument::make(fin::pricing::Call, strike, maturity), .market = fin::market::make(spot, 0.02, 0.01),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})}, .controls = {.engine = mod::Engine::Analytic}};
}

static mod::Field heston(double spot, std::vector<double> strikes, mod::Engine engine = mod::Engine::Cos) {
  return mod::Field{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series(strikes), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
                    .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = engine, .risk = mod::Risk::PriceOnly}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord chord;
  chord.add("two", heston(100.0, {95.0, 105.0})).add("one", bs(100.0, 100.0, 0.30)).add("three", heston(90.0, {80.0, 90.0, 100.0}));
  const auto vols = chord.implied_vol();
  for (std::size_t i = 0; i < chord.size(); ++i) out.writeln(chord.names()[i], vols[i].numel(), "volatilities");
}
```

Output

```text
two 2 volatilities
one 1 volatilities
three 3 volatilities
```

### Example 3: tightening the solver

The options apply to every asset; a tighter tolerance and a named method change how the volatility is found, not what it is.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field bs(double spot, double strike, double vol, double maturity = 1.0) {
  return mod::Field{.instrument = fin::instrument::make(fin::pricing::Call, strike, maturity), .market = fin::market::make(spot, 0.02, 0.01),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})}, .controls = {.engine = mod::Engine::Analytic}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  mod::Chord chord;
  chord.add("flat", bs(100.0, 100.0, 0.20));
  fin::pricing::ImpliedVolOptions options;
  options.method = fin::pricing::ImpliedVolMethod::Brent;
  options.tolerance = 1e-12;
  out.writeln("default", chord.implied_vol()[0].items<double>(), " Brent at 1e-12", chord.implied_vol(options)[0].items<double>());
}
```

Output

```text
default [0.2000000000]  Brent at 1e-12 [0.2000000000]
```

### Example 4: the round trip

Black-Scholes prices inverted give back the volatility that produced them, for each asset.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field bs(double spot, double strike, double vol, double maturity = 1.0) {
  return mod::Field{.instrument = fin::instrument::make(fin::pricing::Call, strike, maturity), .market = fin::market::make(spot, 0.02, 0.01),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})}, .controls = {.engine = mod::Engine::Analytic}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  mod::Chord chord;
  chord.add("a", bs(100.0, 100.0, 0.15)).add("b", bs(100.0, 110.0, 0.35)).add("c", bs(90.0, 80.0, 0.50));
  const auto vols = chord.implied_vol();
  out.writeln(vols[0].items<double>(), vols[1].items<double>(), vols[2].items<double>());
}
```

Output

```text
[0.15000000] [0.35000000] [0.50000000]
```

### Example 5: the errors

An American asset has no Black-Scholes implied volatility, and the whole call is refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field bs(double spot, double strike, double vol, double maturity = 1.0) {
  return mod::Field{.instrument = fin::instrument::make(fin::pricing::Call, strike, maturity), .market = fin::market::make(spot, 0.02, 0.01),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})}, .controls = {.engine = mod::Engine::Analytic}};
}

static mod::Field american_put(double spot) {
  return mod::Field{.instrument = fin::instrument::american(fin::pricing::Put, 100.0, 1.0), .market = fin::market::make(spot, 0.02, 0.01),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({0.25})}, .controls = {.engine = mod::Engine::Lattice}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(text);
    }
  };
  mod::Chord chord;
  chord.add("european", bs(100.0, 100.0, 0.20)).add("american", american_put(100.0));
  report([&] { (void)chord.implied_vol(); });
}
```

Output

```text
subsp::math::finance::module::Field::implied_vol: only European instruments have a Black-Scholes implied volatility
```

## See Also

- [`Chord`](../Chord.md): the class
- [`Field::implied_vol`](../../field/Field/implied_vol.md): the single-asset method
- [`price`](price.md): prices first

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_model_params.html">
    <span class="label">Previous</span>
    <span class="title">from_model_params</span>
  </a>
  <a class="next-link" href="names.html">
    <span class="label">Next</span>
    <span class="title">names</span>
  </a>
</div>
<!-- apiref:nav:end -->
