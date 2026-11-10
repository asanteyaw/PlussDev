<!-- generated from DeltaHedgeResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `hedging::DeltaHedgeResult`

The option value, the hedge, the cash and the hedging error along a path.

All the `Series` have one entry per observation of the spot path (including the start). `option_value` is the field's price of the option along the path, `delta` the hedge ratio, `cash` the cash account, `portfolio = delta·spot + cash` the hedge portfolio and `hedge_error = portfolio − option_value`. `terminal_pnl` is the hedging error at the last date: the profit (positive) or loss of the hedger who sold the option and held the hedge portfolio to maturity.

## Fields

### `option_value`

`pluss::Series`, default `undefined`.

Black-Scholes value along the path.

### `delta`

`pluss::Series`, default `undefined`.

Hedge ratio along the path.

### `cash`

`pluss::Series`, default `undefined`.

Cash account.

### `portfolio`

`pluss::Series`, default `undefined`.

`delta·spot + cash`.

### `hedge_error`

`pluss::Series`, default `undefined`.

`portfolio − option_value`.

### `terminal_pnl`

`double`, default `0.0`.

The hedging error at the last date.

## Guidance

Returned by `delta_hedge`.

## Practical Examples

### Example 1

The members along a path of five dates.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
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
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series path = pluss::make_series({100.0, 102.0, 99.0, 104.0, 107.0});
  const auto r = hedge(path, fin::pricing::Call, 100.0, 0.02, 0.0, 1.0, 0.2);
  out.writeln("option value:", r.option_value.items<double>());
  out.writeln("delta       :", r.delta.items<double>());
  out.writeln("hedge error :", r.hedge_error.items<double>(), "terminal", r.terminal_pnl);
}
```

Output

```text
option value: [8.9160, 8.8021, 5.5785, 6.6969, 7.0000]
delta       : [0.5793, 0.6131, 0.5280, 0.6887, 1.0000]
hedge error : [0.0000, 1.0268, 2.1468, 3.4452, 4.9001] terminal 4.9001
```

### Example 2

The portfolio starts equal to the option: the hedge is funded by the option premium, so the first hedging error is zero.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
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
  out.print_defaults(PrintOptions{}.fixed(8));
  const Series path = pluss::make_series({100.0, 101.0, 100.5, 102.0});
  const auto r = hedge(path, fin::pricing::Put, 100.0, 0.02, 0.0, 0.5, 0.2);
  out.writeln("first error", r.hedge_error.items<double>()[0], ", portfolio[0]", r.portfolio.items<double>()[0], ", option[0]", r.option_value.items<double>()[0]);
}
```

Output

```text
first error 0.00000000 , portfolio[0] 5.12563749 , option[0] 5.12563749
```

### Example 3

The decomposition: `portfolio = delta·spot + cash`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
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
  const Series path = pluss::make_series({100.0, 102.0, 99.0, 104.0});
  const auto r = hedge(path, fin::pricing::Call, 100.0, 0.02, 0.0, 1.0, 0.2);
  out.writeln("max |portfolio - (delta spot + cash)| =", (r.portfolio - (r.delta * path + r.cash)).abs().max().item<double>());
}
```

Output

```text
max |portfolio - (delta spot + cash)| = 0.000000000000
```

## See Also

- [`delta_hedge`](delta_hedge.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="delta_hedge.html">
    <span class="label">Next</span>
    <span class="title">delta_hedge</span>
  </a>
</div>
<!-- apiref:nav:end -->
