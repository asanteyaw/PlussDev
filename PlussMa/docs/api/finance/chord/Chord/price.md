<!-- generated from price.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7097b407e42b -->

# `module::Chord::price`

[Back to `module::Chord`](../Chord.md)

Prices every asset, merging assets that can share a call, and returns a result per asset.

## Signature

```cpp
module::Chord::Result module::Chord::price(Config cfg = {}) const;
```

## What It Does

Validates the Chord, then prices each asset with its own model and engine and returns a [`Chord::Result`](../Result.md): the [`Result`](../../field/Result.md) of every asset by name, and every price laid end to end. Assets that can share a call are merged into one vectorised call (see [`batches`](batches.md)), and the result of a merged call equals pricing each asset alone; the calls that cannot be merged run concurrently unless `parallel` is false, and the numbers are identical either way. An asset gets Greeks when its own `controls.risk` is `All`.

Call it to price a whole book at once: it is the same as calling `Field::price` on every asset, only faster, and it keeps the assets' own models, engines and Greeks settings. An empty Chord prices to an empty result (no prices, no offsets beyond the first).

## Parameters

`Config cfg`
: Device and dtype of the results; the default prices on the CPU in double precision.

## Returns

A [`Chord::Result`](../Result.md): `names`, `results`, `price` and `offsets`.

## Errors

Throws the report of [`validate`](validate.md) if any asset is invalid, naming each; nothing is priced in that case. Errors of an asset's own engine, such as an unsupported model-engine pair, propagate with the asset's message.

## Practical Examples

### Example 1: Black-Scholes assets in one call

Three Black-Scholes assets share one vectorised call, and each price equals the asset priced alone.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  mod::Chord chord;
  chord.add("a", bs(100.0, 100.0, 0.20)).add("b", bs(50.0, 55.0, 0.35, 0.5)).add("c", bs(230.0, 200.0, 0.15, 2.0));
  const auto result = chord.price();
  out.writeln("calls to the engine:", chord.batches());
  for (const auto &name : chord.names()) out.writeln(name, result[name].price.items<double>()[0], " alone", chord.at(name).price().price.items<double>()[0]);
}
```

Output

```text
calls to the engine: 1
a 8.349406  alone 8.349406
b 3.115882  alone 3.115882
c 39.029254  alone 39.029254
```

### Example 2: Heston assets with different parameters

Three Heston assets with different strikes, spots and parameters share one COS call; the prices are laid end to end.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field third = heston(100.0, {100.0});
  third.model = fin::model::make(fin::model::Kind::Heston, {2.0, 0.09, 0.4, -0.5, 0.09});
  mod::Chord chord;
  chord.add("h1", heston(100.0, {90.0, 100.0, 110.0})).add("h2", heston(80.0, {75.0, 85.0})).add("h3", third);
  const auto result = chord.price();
  out.writeln("calls:", chord.batches(), " prices end to end:", result.price.items<double>());
  out.writeln("offsets:", result.offsets);
}
```

Output

```text
calls: 1  prices end to end: [15.1137, 8.6281, 4.0596, 9.9855, 4.4245, 12.4770]
offsets: [0, 3, 5, 6]
```

### Example 3: different engines in one book

A Black-Scholes asset, a Heston asset and an American put each use their own engine; the Chord makes one call for each family.

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

static mod::Field american_put(double spot) {
  return mod::Field{.instrument = fin::instrument::american(fin::pricing::Put, 100.0, 1.0), .market = fin::market::make(spot, 0.02, 0.01),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({0.25})}, .controls = {.engine = mod::Engine::Lattice}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Chord chord;
  chord.add("flat", bs(100.0, 100.0, 0.20)).add("smile", heston(100.0, {90.0, 100.0, 110.0})).add("american", american_put(100.0));
  const auto result = chord.price();
  out.writeln("calls", chord.batches());
  for (const auto &name : chord.names()) out.writeln(name, result[name].price.items<double>());
}
```

Output

```text
calls 3
flat [8.3494]
smile [15.1137, 8.6281, 4.0596]
american [9.4120]
```

### Example 4: where each asset starts

`offsets` marks where each asset's prices start in the concatenated `price`: asset i owns `price[offsets[i], offsets[i + 1])`.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Chord chord;
  chord.add("one", bs(100.0, 100.0, 0.20)).add("three", heston(100.0, {90.0, 100.0, 110.0})).add("two", heston(90.0, {85.0, 95.0}));
  const auto result = chord.price();
  const auto all = result.price.items<double>();
  for (std::size_t i = 0; i < chord.size(); ++i) {
    out.writeln(result.names[i], " owns", result.offsets[i], "to", result.offsets[i + 1], " first price", all[static_cast<std::size_t>(result.offsets[i])]);
  }
}
```

Output

```text
one  owns 0 to 1  first price 8.3494
three  owns 1 to 4  first price 15.1137
two  owns 4 to 6  first price 10.8174
```

### Example 5: each asset keeps its own Greeks setting

One asset asks for Greeks and one does not; only the first has them.

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
  mod::Field with = bs(100.0, 100.0, 0.20);
  with.controls.risk = mod::Risk::All;
  mod::Field without = bs(100.0, 100.0, 0.20);
  without.controls.risk = mod::Risk::PriceOnly;
  mod::Chord chord;
  chord.add("with", with).add("without", without);
  const auto result = chord.price();
  out.writeln("with: delta defined", result["with"].greeks.delta.defined(), " delta", result["with"].greeks.delta.items<double>());
  out.writeln("without: delta defined", result["without"].greeks.delta.defined());
}
```

Output

```text
with: delta defined true  delta [0.55405]
without: delta defined false
```

### Example 6: threads change nothing

`parallel` only decides whether the independent calls run on several threads; the numbers are bit for bit the same.

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
  mod::Chord chord;
  chord.add("a", american_put(100.0)).add("b", american_put(90.0)).add("c", bs(100.0, 100.0, 0.20));
  chord.parallel = true;
  const auto threaded = chord.price();
  chord.parallel = false;
  const auto serial = chord.price();
  out.writeln("identical:", threaded.price.items<double>() == serial.price.items<double>());
}
```

Output

```text
identical: true
```

### Example 7: an empty chord

Pricing nothing is allowed and returns an empty result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord chord;
  const auto result = chord.price();
  out.writeln("assets", result.names.size(), " prices defined", result.price.defined());
}
```

Output

```text
assets 0  prices defined false
```

### Example 8: the errors

An invalid asset stops the whole call and is named, so no partial result is returned.

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
  chord.add("fine", bs(100.0, 100.0, 0.20)).add("broken", bs(100.0, -5.0, 0.20));
  report([&] { (void)chord.price(); });
}
```

Output

```text
subsp::math::finance::module::Chord:
asset 'broken': subsp::math::finance::module::Field: instrument.strike must be > 0
```

## See Also

- [`Chord`](../Chord.md): the class
- [`Chord::Result`](../Result.md): the result
- [`batches`](batches.md): how many calls
- [`validate`](validate.md): the check that runs first

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="operator_index.html">
    <span class="label">Previous</span>
    <span class="title">operator_index</span>
  </a>
  <a class="next-link" href="simulate_paths.html">
    <span class="label">Next</span>
    <span class="title">simulate_paths</span>
  </a>
</div>
<!-- apiref:nav:end -->
