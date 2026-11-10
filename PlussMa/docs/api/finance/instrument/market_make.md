<!-- generated from market_make.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `market::make`

A market state with one spot, one rate and one dividend yield.

## Signature

```cpp
market::State market::make(double spot, double rate, double dividend_yield, pluss::Config cfg = f64_default());
```

## What It Does

Builds a [`State`](State.md) whose three `Series` have one element each (Float64 by default). It is the usual way to describe the market of a single pricing; fill the `Series` yourself for scenarios or a curve.

## Parameters

`double spot`
: The underlying price.

`double rate`
: Annual continuously compounded rate.

`double dividend_yield`
: Annual continuous yield.

`pluss::Config cfg`
: Type and device.

## Returns

A `State`.

## Practical Examples

### Example 1

A market and a price.

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
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0), .market = fin::market::make(100.0, 0.05, 0.02),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}), .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
  out.writeln(field.price().price.items<double>());
}
```

Output

```text
[9.227006]
```

### Example 2

The three members.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto m = fin::market::make(105.0, 0.03, 0.01);
  out.writeln("spot", m.spot.items<double>(), "rate", m.rate.items<double>(), "yield", m.dividend_yield.items<double>());
}
```

Output

```text
spot [105.000] rate [0.030] yield [0.010]
```

### Example 3

Re-marking: replace the market and price again.

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
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0), .market = fin::market::make(100.0, 0.05, 0.0),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}), .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
  const double before = field.price().price.items<double>()[0];
  field.market = fin::market::make(110.0, 0.05, 0.0);
  out.writeln("spot 100:", before, ", spot 110:", field.price().price.items<double>()[0]);
}
```

Output

```text
spot 100: 10.4506 , spot 110: 17.6630
```

## See Also

- [`State`](State.md): what it builds

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="make.html">
    <span class="label">Previous</span>
    <span class="title">make</span>
  </a>
</div>
<!-- apiref:nav:end -->
