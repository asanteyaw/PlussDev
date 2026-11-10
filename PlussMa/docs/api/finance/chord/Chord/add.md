<!-- generated from add.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 08a8c9182957 -->

# `module::Chord::add`

[Back to `module::Chord`](../Chord.md)

Adds a Field, or a dictionary of Fields, to the Chord under unique names.

## Signature

```cpp
module::Chord &module::Chord::add(std::string name, module::Field field);
module::Chord &module::Chord::add(pluss::RangeView<module::Chord::Entry> assets);
```

## What It Does

Appends `field` and returns the `Chord`, so calls chain. The Field is stored by value: the Chord keeps its own copy, so editing the original afterwards does not change the asset (reach the stored one with [`at`](at.md)). The name identifies the asset in [`at`](at.md), in a [`Chord::Result`](../Result.md) and in validation messages. The order of adding is the order of every result, of [`names`](names.md) and of the asset axis of a [`Chord::Paths`](../Paths.md).

The second overload adds several assets at once, in the order given, from a dictionary written in braces, `chord.add({{"spx", spx}, {"ndx", ndx}})`, or from any contiguous list of [`module::Chord::Entry`](Chord.md) (a name and a Field), such as a `std::vector` built in a loop. It is all or none: the whole list is checked first, and if any name is empty, appears twice in the list or is already in the Chord, nothing is added. Use it whenever there are more than two or three assets, so a book is not a long chain of `add` calls; a dictionary given at construction (see [`Chord`](Chord.md)) is the same without the empty Chord first.

Call the single-asset form once per asset when assets arrive one at a time. A Field is added without being checked: a Field that cannot be priced is accepted here and reported by [`validate`](validate.md) or by `price`, which names it.

## Parameters

`std::string name`
: A non-empty name, unique within the Chord. Names are case sensitive.

`module::Field field`
: A complete single-asset Field. It is copied.

`pluss::RangeView<module::Chord::Entry> assets`
: The assets as `{name, field}` pairs, in the order they are to be added. The list is only read.

## Returns

The Chord itself, so calls chain.

## Errors

Throws `subsp::math::finance::module::Chord::add: the asset name must not be empty` for an empty name, and `subsp::math::finance::module::Chord::add: there is already an asset named '<name>'` for a repeated one. Nothing is added in either case. The list overload throws the same two, and `subsp::math::finance::module::Chord::add: the asset name '<name>' appears twice in the list` for a name repeated within the list; it leaves the Chord unchanged.

## Practical Examples

### Example 1: two assets in one chain

`add` returns the Chord, so a book is built in one expression.

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
  out.writeln("assets", chord.size(), chord.names());
}
```

Output

```text
assets 2 [spx, ndx]
```

### Example 2: different models in one chord

The assets need nothing in common: a Black-Scholes asset on the analytic engine and a Heston asset on the COS engine.

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
  const auto result = chord.price();
  out.writeln("flat ", result["flat"].price.items<double>());
  out.writeln("smile", result["smile"].price.items<double>());
}
```

Output

```text
flat  [8.34941]
smile [15.11367, 8.62811, 4.05957]
```

### Example 3: the order of adding is the order of results

Results follow the order of adding, whatever the names sort like.

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
  chord.add("zeta", bs(100.0, 100.0, 0.20)).add("alpha", bs(100.0, 110.0, 0.20)).add("mid", bs(100.0, 90.0, 0.20));
  const auto result = chord.price();
  out.writeln("names  ", result.names);
  out.writeln("prices ", result.price.items<double>());
  out.writeln("offsets", result.offsets);
}
```

Output

```text
names   [zeta, alpha, mid]
prices  [8.34941, 4.56424, 14.05170]
offsets [0, 1, 2, 3]
```

### Example 4: the field is copied

Changing the Field after `add` does not change the asset in the Chord; `at` reaches the stored copy.

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
  mod::Field field = bs(100.0, 100.0, 0.20);
  mod::Chord chord;
  chord.add("a", field);
  field.market.spot = pluss::make_series({150.0});
  out.writeln("original after the edit", field.price().price.items<double>());
  out.writeln("stored asset            ", chord.price()["a"].price.items<double>());
}
```

Output

```text
original after the edit [50.65308]
stored asset             [8.34941]
```

### Example 5: the errors

An empty name and a repeated name are refused, and the Chord keeps only the assets that were added.

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
  chord.add("a", bs(100.0, 100.0, 0.20));
  report([&] { chord.add("", bs(100.0, 100.0, 0.20)); });
  report([&] { chord.add("a", bs(100.0, 110.0, 0.20)); });
  out.writeln("size", chord.size());
}
```

Output

```text
subsp::math::finance::module::Chord::add: the asset name must not be empty
subsp::math::finance::module::Chord::add: there is already an asset named 'a'
size 1
```

### Example 6: a dictionary of assets

Several assets added in one call, written like a map literal; the order is the order of the braces.

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
  chord.add({{"spx", bs(100.0, 100.0, 0.20)}, {"ndx", bs(90.0, 95.0, 0.30)}, {"rut", bs(50.0, 55.0, 0.35, 0.5)}});
  out.writeln(chord.names(), " engine calls", chord.batches());
  const auto result = chord.price();
  for (const auto &name : chord.names()) out.writeln(name, result[name].price.items<double>());
}
```

Output

```text
[spx, ndx, rut]  engine calls 1
spx [8.34941]
ndx [8.97264]
rut [3.11588]
```

### Example 7: a dictionary after a single add

The two forms mix, and the dictionary form returns the Chord so it chains.

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
  chord.add("first", bs(100.0, 100.0, 0.20)).add({{"second", bs(90.0, 95.0, 0.30)}, {"third", bs(80.0, 85.0, 0.25)}}).add("fourth", bs(70.0, 70.0, 0.20));
  out.writeln(chord.names());
}
```

Output

```text
[first, second, third, fourth]
```

### Example 8: a list built in a loop

A std::vector of entries built from data is added in one call.

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
  std::vector<mod::Chord::Entry> entries;
  for (const double vol : {0.15, 0.20, 0.25, 0.30}) entries.emplace_back("vol" + std::to_string(static_cast<int>(vol * 100)), bs(100.0, 100.0, vol));
  mod::Chord chord;
  chord.add(entries);
  const auto result = chord.price();
  for (const auto &name : chord.names()) out.writeln(name, result[name].price.items<double>());
}
```

Output

```text
vol15 [6.39521]
vol20 [8.34941]
vol25 [10.30002]
vol30 [12.24520]
```

### Example 9: all or nothing

A list with one bad entry adds none of its assets: the Chord is as it was.

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
  mod::Chord chord{{"a", bs(100.0, 100.0, 0.20)}};
  report([&] { chord.add({{"b", bs(90.0, 95.0, 0.30)}, {"a", bs(80.0, 85.0, 0.25)}}); });
  out.writeln("after a name already present:", chord.names());
  report([&] { chord.add({{"b", bs(90.0, 95.0, 0.30)}, {"b", bs(80.0, 85.0, 0.25)}}); });
  out.writeln("after a name repeated in the list:", chord.names());
  report([&] { chord.add({{"b", bs(90.0, 95.0, 0.30)}, {"", bs(80.0, 85.0, 0.25)}}); });
  out.writeln("after an empty name:", chord.names());
}
```

Output

```text
subsp::math::finance::module::Chord::add: there is already an asset named 'a'
after a name already present: [a]
subsp::math::finance::module::Chord::add: the asset name 'b' appears twice in the list
after a name repeated in the list: [a]
subsp::math::finance::module::Chord::add: the asset name must not be empty
after an empty name: [a]
```

## See Also

- [`Chord`](../Chord.md): the class
- [`Chord::Chord`](Chord.md): builds a Chord from a dictionary of assets at construction
- [`at`](at.md): reads an asset back
- [`names`](names.md) and [`size`](size.md): what the Chord holds

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Chord.html">
    <span class="label">Previous</span>
    <span class="title">Chord</span>
  </a>
  <a class="next-link" href="at.html">
    <span class="label">Next</span>
    <span class="title">at</span>
  </a>
</div>
<!-- apiref:nav:end -->
