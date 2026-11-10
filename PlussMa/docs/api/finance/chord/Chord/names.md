<!-- generated from names.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 168be731c6c4 -->

# `module::Chord::names`

[Back to `module::Chord`](../Chord.md)

The names of the assets, in the order they were added.

## Signature

```cpp
const std::vector<std::string> &module::Chord::names() const;
```

## What It Does

Returns the asset names in the order of adding, which is the order of every result: `Chord::Result::names`, the segments of `Chord::Result::price` and the asset axis of a `Chord::Paths`. The reference is to the Chord's own list and is invalidated by the next `add`; copy it if you add assets while iterating.

Call it to loop over the assets, to label a table of results, or to read the order without pricing.

## Returns

A const reference to a vector of names.

## Practical Examples

### Example 1: the order of adding

Names come back in the order they were added.

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
  chord.add("zeta", bs(100.0, 100.0, 0.20)).add("alpha", bs(100.0, 110.0, 0.20));
  out.writeln(chord.names());
}
```

Output

```text
[zeta, alpha]
```

### Example 2: labelling the results

Loop over the names to print each asset's price.

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
  chord.add("a", bs(100.0, 100.0, 0.20)).add("b", bs(100.0, 110.0, 0.30));
  const auto result = chord.price();
  for (const auto &name : chord.names()) out.writeln(name, result[name].price.items<double>());
}
```

Output

```text
a [8.34941]
b [8.41325]
```

### Example 3: the same names as the result

The result carries the same list.

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
  chord.add("a", bs(100.0, 100.0, 0.20)).add("b", bs(100.0, 110.0, 0.30));
  out.writeln("same as Chord::Result::names:", chord.names() == chord.price().names);
}
```

Output

```text
same as Chord::Result::names: true
```

### Example 4: an empty chord

No assets, no names.

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
  out.writeln("names", chord.names().size());
}
```

Output

```text
names 0
```

## See Also

- [`size`](size.md): how many
- [`Chord`](../Chord.md): the class
- [`Chord::Result`](../Result.md): the result with the same order

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="implied_vol.html">
    <span class="label">Previous</span>
    <span class="title">implied_vol</span>
  </a>
  <a class="next-link" href="operator_index.html">
    <span class="label">Next</span>
    <span class="title">operator_index</span>
  </a>
</div>
<!-- apiref:nav:end -->
