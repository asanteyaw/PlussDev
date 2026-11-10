<!-- generated from size.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2d6e4cc8c44e -->

# `module::Chord::size`

[Back to `module::Chord`](../Chord.md)

The number of assets in the Chord.

## Signature

```cpp
std::size_t module::Chord::size() const;
```

## What It Does

The number of assets added. It is the number of entries of `names()`, the number of results in a `Chord::Result`, and the length of the asset axis of a `Chord::Paths`. It counts assets, not options: an asset whose Field holds ten strikes is one asset.

Call it to test for an empty Chord, to size a loop, or to compare with `batches()`, which counts the engine calls and is never more than `size()`.

## Returns

The number of assets, zero for an empty Chord.

## Practical Examples

### Example 1: growing a chord

Each `add` raises the size by one.

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
  out.writeln("empty", chord.size());
  chord.add("a", bs(100.0, 100.0, 0.20));
  out.writeln("one", chord.size());
  chord.add("b", bs(100.0, 110.0, 0.20));
  out.writeln("two", chord.size());
}
```

Output

```text
empty 0
one 1
two 2
```

### Example 2: assets are not options

An asset with a strip of five strikes counts once; the strip has five prices.

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
  chord.add("strip", heston(100.0, {80.0, 90.0, 100.0, 110.0, 120.0}));
  out.writeln("assets", chord.size(), " prices", chord.price().price.numel());
}
```

Output

```text
assets 1  prices 5
```

### Example 3: size and batches

`batches` counts engine calls, never more than `size`: three Black-Scholes assets merge into one call.

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
  chord.add("a", bs(100.0, 100.0, 0.20)).add("b", bs(100.0, 110.0, 0.25)).add("c", bs(90.0, 95.0, 0.30));
  out.writeln("assets", chord.size(), " engine calls", chord.batches());
}
```

Output

```text
assets 3  engine calls 1
```

### Example 4: the result has one entry per asset

The size equals the number of per-asset results.

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
  chord.add("a", bs(100.0, 100.0, 0.20)).add("b", bs(100.0, 110.0, 0.25));
  out.writeln("size", chord.size(), " results", chord.price().results.size());
}
```

Output

```text
size 2  results 2
```

## See Also

- [`names`](names.md): which
- [`batches`](batches.md): how many calls
- [`Chord`](../Chord.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="simulate_paths.html">
    <span class="label">Previous</span>
    <span class="title">simulate_paths</span>
  </a>
  <a class="next-link" href="validate.html">
    <span class="label">Next</span>
    <span class="title">validate</span>
  </a>
</div>
<!-- apiref:nav:end -->
