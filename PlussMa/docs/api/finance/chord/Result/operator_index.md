<!-- generated from operator_index.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 14fef46434e8 -->

# `module::Chord::Result::operator[]`

[Back to `module::Chord::Result`](../Result.md)

The result of one asset, by name.

## Signature

```cpp
const module::Field::Result &module::Chord::Result::operator[](const std::string &name) const;
```

## What It Does

Returns the [`Result`](../../field/Result.md) of the asset with that name: its prices and, if its `controls.risk` was `All`, its Greeks. It is the usual way to read a price, `result["spx"].price`, and it does not depend on the order of adding. The reference is into the result and lives as long as the result does. An unknown name throws; the lookup never creates an entry.

Use it for a named asset; use `results[i]` when you are iterating by position.

## Parameters

`const std::string &name`
: The asset's name, as given to `Chord::add`.

## Returns

A const reference to the asset's `module::Field::Result`.

## Errors

Throws `subsp::math::finance::module::Chord::Result: no asset named '<name>'` when no asset has that name.

## Practical Examples

### Example 1: a price by name

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
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord chord;
  chord.add("spx", bs(100.0, 100.0, 0.20)).add("ndx", bs(90.0, 95.0, 0.30));
  const auto result = chord.price();
  out.writeln("spx", result["spx"].price.items<double>(), " ndx", result["ndx"].price.items<double>());
}
```

Output

```text
spx [8.34941]  ndx [8.97264]
```

### Example 2: the Greeks of one asset

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
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord chord;
  chord.add("spx", bs(100.0, 100.0, 0.20));
  const auto result = chord.price();
  const auto &spx = result["spx"];
  out.writeln("delta", spx.greeks.delta.items<double>(), " gamma", spx.greeks.gamma.items<double>(), " vega", spx.greeks.vega.items<double>());
}
```

Output

```text
delta [0.55405]  gamma [0.01953]  vega [39.05542]
```

### Example 3: a strip of prices

An asset with several options returns one price per option.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston(double spot, std::vector<double> strikes, mod::Engine engine = mod::Engine::Cos) {
  return mod::Field{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series(strikes), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
                    .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = engine, .risk = mod::Risk::PriceOnly}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord chord;
  chord.add("smile", heston(100.0, {90.0, 100.0, 110.0}));
  out.writeln(chord.price()["smile"].price.items<double>());
}
```

Output

```text
[15.11367, 8.62811, 4.05957]
```

### Example 4: the same as the position

The name and the position reach the same result.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord chord;
  chord.add("a", bs(100.0, 100.0, 0.20)).add("b", bs(90.0, 95.0, 0.30));
  const auto result = chord.price();
  out.writeln("same object:", &result["b"] == &result.results[1]);
}
```

Output

```text
same object: true
```

### Example 5: the error

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
  chord.add("spx", bs(100.0, 100.0, 0.20));
  const auto result = chord.price();
  report([&] { (void)result["ndx"]; });
}
```

Output

```text
subsp::math::finance::module::Chord::Result: no asset named 'ndx'
```

## See Also

- [`Chord::Result`](../Result.md): the result
- [`Result`](../../field/Result.md): the per-asset result
- [`Chord::price`](../Chord/price.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
