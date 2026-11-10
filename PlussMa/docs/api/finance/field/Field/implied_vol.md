<!-- generated from implied_vol.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8bf80d88f4dc -->

# `module::Field::implied_vol`

[Back to `module::Field`](../Field.md)

The Black-Scholes implied volatility of the field's own model prices, or of market quotes for the field's instrument.

## Signature

```cpp
pluss::Series module::Field::implied_vol(const prc::ImpliedVolOptions &options = {}, pluss::Config cfg = {}) const;
pluss::Series module::Field::implied_vol(const pluss::Series &quotes, const prc::ImpliedVolOptions &options = {}, pluss::Config cfg = {}) const;
```

## What It Does

The Black-Scholes price is a strictly increasing function of volatility, so a market price has at most one volatility that reproduces it: the **implied volatility**, the model-free way to quote an option. `implied_vol` inverts prices for every option of the field's strip at once, using the field's strikes, maturities, spot, rate and dividend yield (each may be one value or one per option; they are broadcast to a common length). The solver brackets the root in `[lower_vol, upper_vol]` and finds it with the chosen [method](../../implied/ImpliedVolMethod.md), falling back to bisection if that method misses the tolerance.

The two forms answer two questions. **Without quotes** it prices the field with its own model and engine and inverts those prices: the smile the model implies (flat for Black-Scholes, skewed for Heston, and so on). **With quotes** it inverts the prices you pass, for this instrument and market: the smile the market implies. Because it is a method, the model of the field is irrelevant to the second form.

**Where it returns NaN.** A price that no volatility can produce does not throw; its entry is NaN, so one bad quote does not spoil the strip. A price is unattainable if it is below the discounted intrinsic value (an arbitrage), above the value at `upper_vol`, or negative. Invalid *options* (a bad bracket, a non-positive tolerance) throw. Only European instruments have a Black-Scholes implied volatility; an American or Bermudan field throws.

## Parameters

`const Series &quotes`
: Observed option prices, one per option or a single value (second form). Omit to invert the field's own prices (first form).

`const prc::ImpliedVolOptions &options`
: Method, bracket, tolerance, iteration limit.

`pluss::Config cfg`
: Device of the result; its type is always Float64.

## Returns

A `Series` with one implied volatility per option, NaN where none exists.

## Errors

- Invalid `options` throw (`pluss::Error`).
- A non-European instrument throws.
- Quotes, strikes, maturities and spots whose lengths are neither one nor equal throw.

## Practical Examples

### Example 1

The smile a model implies. A Black-Scholes field gives back its own flat volatility; a Heston field with negative correlation gives a smile that falls with strike.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace prc = fin::pricing;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series strikes = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64);
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = strikes, .maturity = pluss::make_series({1.0})},
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.25}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
  out.writeln("Black-Scholes:", field.implied_vol().items<double>());
  field.model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04});
  field.controls.engine = mod::Engine::Cos;
  out.writeln("Heston (COS) :", field.implied_vol().items<double>());
}
```

Output

```text
Black-Scholes: [0.2500, 0.2500, 0.2500, 0.2500, 0.2500]
Heston (COS) : [0.2281, 0.2099, 0.1926, 0.1768, 0.1634]
```

### Example 2

Invert market quotes. Quotes generated with a volatility that varies by strike are inverted and recover it; the field's own model plays no part.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace prc = fin::pricing;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const Series strikes = pluss::make_series({80.0, 90.0, 100.0, 110.0, 120.0});
  const std::vector<double> vols = {0.32, 0.26, 0.22, 0.21, 0.23};
  std::vector<double> quotes;
  for (std::size_t i = 0; i < vols.size(); ++i) {
    mod::Field f{.instrument = fin::instrument::make(fin::pricing::Call, strikes.items<double>()[i], 1.0), .market = fin::market::make(100.0, 0.02, 0.01),
                 .model = fin::model::make(fin::model::Kind::BlackScholes, {vols[i]}), .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
    quotes.push_back(f.price().price.items<double>()[0]);
  }
  mod::Field strip{.instrument = {.type = fin::pricing::Call, .strike = strikes, .maturity = pluss::make_series({1.0})}, .market = fin::market::make(100.0, 0.02, 0.01),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2})};
  out.writeln("input vols  :", vols);
  out.writeln("implied vols:", strip.implied_vol(pluss::make_series(quotes)).items<double>());
}
```

Output

```text
input vols  : [0.32000000, 0.26000000, 0.22000000, 0.21000000, 0.23000000]
implied vols: [0.32000000, 0.26000000, 0.22000000, 0.21000000, 0.23000000]
```

### Example 3

Calls and puts mixed in one strip (`type = Auto` with `is_call`): both are inverted correctly, and a call and a put of the same strike give the same volatility because parity ties their prices.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace prc = fin::pricing;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  mod::Field f{.instrument = {.type = fin::pricing::OptionType::Auto, .is_call = pluss::make_series({1.0, 0.0}), .strike = pluss::make_series({110.0}), .maturity = pluss::make_series({1.0})},
               .market = fin::market::make(100.0, 0.05, 0.02),
               .model = fin::model::make(fin::model::Kind::BlackScholes, {0.3}),
               .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
  out.writeln("prices      :", f.price().price.items<double>());
  out.writeln("implied vols:", f.implied_vol().items<double>());
}
```

Output

```text
prices      : [9.0570619260, 15.6724312904]
implied vols: [0.3000000000, 0.3000000000]
```

### Example 4

Several maturities in one call: strike and spot are shared, the maturities differ.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace prc = fin::pricing;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({100.0}), .maturity = pluss::make_series({0.25, 1.0, 3.0})},
               .market = fin::market::make(100.0, 0.05, 0.02),
               .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
               .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
  out.writeln("implied vols:", f.implied_vol().items<double>());
}
```

Output

```text
implied vols: [0.2000000000, 0.1999999996, 0.2000000000]
```

### Example 5

Unattainable quotes give NaN, not an error: a price below intrinsic value (an arbitrage) and a negative price; the good quote beside them is unaffected.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace prc = fin::pricing;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  mod::Field f{.instrument = fin::instrument::make(fin::pricing::Call, 60.0, 1.0), .market = fin::market::make(100.0, 0.05, 0.02),
               .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2})};
  out.writeln("implied vols:", f.implied_vol(pluss::make_series({41.0, 0.001, -1.0})).items<double>());
}
```

Output

```text
implied vols: [0.23003578, nan, nan]
```

### Example 6

Options choose the solver. A narrow bracket that excludes the true volatility cannot reproduce the price, so the entry is NaN; widening the bracket recovers it. Invalid options throw.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace prc = fin::pricing;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  mod::Field f{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0), .market = fin::market::make(100.0, 0.05, 0.02),
               .model = fin::model::make(fin::model::Kind::BlackScholes, {0.6}), .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
  const Series quote = f.price().price;
  fin::pricing::ImpliedVolOptions narrow, wide, bad;
  narrow.upper_vol = 0.4;
  wide.upper_vol = 2.0;
  bad.tolerance = 0.0;
  out.writeln("upper_vol 0.4:", f.implied_vol(quote, narrow).items<double>());
  out.writeln("upper_vol 2.0:", f.implied_vol(quote, wide).items<double>());
  try {
    (void)(f.implied_vol(quote, bad));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
upper_vol 0.4: [nan]
upper_vol 2.0: [0.60000000]
subsp::math::finance::pricing::implied_vol: tolerance must be finite and > 0
```

## See Also

- [`ImpliedVolOptions`](../../implied/ImpliedVolOptions.md): the settings
- [`ImpliedVolMethod`](../../implied/ImpliedVolMethod.md): the algorithms
- [`price`](price.md): the prices it inverts

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="exercise_boundary.html">
    <span class="label">Previous</span>
    <span class="title">exercise_boundary</span>
  </a>
  <a class="next-link" href="loss.html">
    <span class="label">Next</span>
    <span class="title">loss</span>
  </a>
</div>
<!-- apiref:nav:end -->
