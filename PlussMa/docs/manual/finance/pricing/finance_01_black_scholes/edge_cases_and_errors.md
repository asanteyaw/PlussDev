<!-- generated from edge_cases_and_errors.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Edge cases and errors

[Back to Black-Scholes](../finance_01_black_scholes.md)

## Inputs outside the model

A field validates all of its inputs before it prices anything, and reports **every** problem it finds in one `pluss::Error`, one per line, each naming the field of the object it belongs to. The conditions are the same for a single option and for a strip (where each element is checked), and for the price and the Greeks alike.

| Condition | Message |
|---|---|
| `spot` not finite and positive | `market.spot must be finite`, `market.spot must be > 0` |
| `strike` not positive | `instrument.strike must be > 0` |
| `volatility` not positive | `model.BlackScholes.volatility must be > 0` |
| `maturity` negative or not finite | `instrument.maturity must be >= 0`, `instrument.maturity must be finite` |
| `rate` or `dividend_yield` not finite | `market.rate must be finite`, `market.dividend_yield must be finite` |
| `type` is `OptionType::Auto` with no flag | `instrument.is_call must be defined when instrument.type is Auto` (`Auto` is for a strip whose options carry their own call/put flag) |

A zero volatility is an error, not the limit of the formula; use a small positive one, or the intrinsic value of the forward directly. A zero maturity is allowed and prices the intrinsic value.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;

// One Black-Scholes option as a field: instrument, market and model, priced by the analytic engine.
mod::Field bs_field(prc::OptionType type, double S, double K, double r, double q, double T, double vol) {
  return mod::Field{.instrument = fin::instrument::make(type, K, T),
                    .market = fin::market::make(S, r, q),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                    .controls = {.engine = mod::Engine::Analytic}};
}
double bs_price(prc::OptionType type, double S, double K, double r, double q, double T, double vol) {
  return bs_field(type, S, K, r, q, T, vol).price().price.items<double>()[0];
}

int main() {
  OutputEngine out;
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  auto attempt = [&](const char *label, double S, double K, double r, double q, double T, double vol, prc::OptionType type = prc::Call) {
    try {
      const double price = bs_price(type, S, K, r, q, T, vol);
      out.writeln(label, "-> no error, price", price);
    } catch (const std::exception &e) {
      const std::string message = e.what();
      out.writeln(label, "->", message.substr(0, message.find('\n')));
    }
  };
  attempt("volatility 0     ", 100, 100, 0.05, 0.0, 1.0, 0.0);
  attempt("volatility -0.2  ", 100, 100, 0.05, 0.0, 1.0, -0.2);
  attempt("spot 0           ", 0.0, 100, 0.05, 0.0, 1.0, 0.2);
  attempt("spot NaN         ", nan, 100, 0.05, 0.0, 1.0, 0.2);
  attempt("strike 0         ", 100, 0.0, 0.05, 0.0, 1.0, 0.2);
  attempt("maturity -1      ", 100, 100, 0.05, 0.0, -1.0, 0.2);
  attempt("maturity infinity", 100, 100, 0.05, 0.0, inf, 0.2);
  attempt("rate infinity    ", 100, 100, inf, 0.0, 1.0, 0.2);
  attempt("dividend NaN     ", 100, 100, 0.05, nan, 1.0, 0.2);
  attempt("OptionType::Auto ", 100, 100, 0.05, 0.0, 1.0, 0.2, prc::OptionType::Auto);
}
```

Output

```text
volatility 0      -> subsp::math::finance::module::Field: model.BlackScholes.volatility must be > 0
volatility -0.2   -> subsp::math::finance::module::Field: model.BlackScholes.volatility must be > 0
spot 0            -> subsp::math::finance::module::Field: market.spot must be > 0
spot NaN          -> subsp::math::finance::module::Field: market.spot must be finite
strike 0          -> subsp::math::finance::module::Field: instrument.strike must be > 0
maturity -1       -> subsp::math::finance::module::Field: instrument.maturity must be >= 0
maturity infinity -> subsp::math::finance::module::Field: instrument.maturity must be finite
rate infinity     -> subsp::math::finance::module::Field: market.rate must be finite
dividend NaN      -> subsp::math::finance::module::Field: market.dividend_yield must be finite
OptionType::Auto  -> subsp::math::finance::module::Field: instrument.is_call must be defined when instrument.type is Auto
```

Each call is rejected with the message in the table, and the first line of the message names the field of the object at fault. Several mistakes at once are all reported together:

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;

// One Black-Scholes option as a field: instrument, market and model, priced by the analytic engine.
mod::Field bs_field(prc::OptionType type, double S, double K, double r, double q, double T, double vol) {
  return mod::Field{.instrument = fin::instrument::make(type, K, T),
                    .market = fin::market::make(S, r, q),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                    .controls = {.engine = mod::Engine::Analytic}};
}
double bs_price(prc::OptionType type, double S, double K, double r, double q, double T, double vol) {
  return bs_field(type, S, K, r, q, T, vol).price().price.items<double>()[0];
}

int main() {
  OutputEngine out;
  try {
    (void)bs_field(prc::Call, -5.0, 0.0, 0.05, 0.0, -1.0, 0.0).price();
  } catch (const std::exception &e) {
    const std::string message = e.what();
    out.writeln(message.substr(0, message.find("Triggered")));
  }
}
```

Output

```text
subsp::math::finance::module::Field (4 problems):
  - instrument.strike must be > 0
  - instrument.maturity must be >= 0
  - market.spot must be > 0
  - model.BlackScholes.volatility must be > 0
```

The four mistakes, a negative spot, a zero strike, a negative maturity and a zero volatility, come back together, so a bad configuration is fixed in one pass rather than one error at a time.

## Extremes stay finite and inside the bounds

The formula is evaluated stably at the extremes: far out of the money the price underflows to zero rather than to `NaN`, far in the money it approaches the discounted intrinsic value, a very small volatility gives the discounted forward intrinsic value, and a very large one gives the discounted spot. Negative rates and dividend yields are valid and shift the forward.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;

// One Black-Scholes option as a field: instrument, market and model, priced by the analytic engine.
mod::Field bs_field(prc::OptionType type, double S, double K, double r, double q, double T, double vol) {
  return mod::Field{.instrument = fin::instrument::make(type, K, T),
                    .market = fin::market::make(S, r, q),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                    .controls = {.engine = mod::Engine::Analytic}};
}
double bs_price(prc::OptionType type, double S, double K, double r, double q, double T, double vol) {
  return bs_field(type, S, K, r, q, T, vol).price().price.items<double>()[0];
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const double r = 0.05, q = 0.02, T = 0.25;
  out.writeln("far out of the money call (spot 100, strike 500)         ", bs_price(prc::Call, 100, 500, r, 0.0, T, 0.2));
  out.writeln("far in the money put (spot 100, strike 500)              ", bs_price(prc::Put, 100, 500, r, 0.0, T, 0.2), " discounted intrinsic", 500 * std::exp(-r * T) - 100);
  out.writeln("far in the money call (spot 500, strike 100)             ", bs_price(prc::Call, 500, 100, r, 0.0, T, 0.2), " discounted intrinsic", 500 - 100 * std::exp(-r * T));
  out.writeln("volatility 1e-6, strike 90: price", bs_price(prc::Call, 100, 90, r, q, 1.0, 1e-6), " discounted forward intrinsic", 100 * std::exp(-q) - 90 * std::exp(-r));
  out.writeln("volatility 50:              price", bs_price(prc::Call, 100, 100, r, q, 1.0, 50.0), " discounted spot", 100 * std::exp(-q));
  out.writeln("maturity 1e-8, strike 95: price", bs_price(prc::Call, 100, 95, r, q, 1e-8, 0.2), " intrinsic 5");
  out.writeln("negative rate -1%:          call", bs_price(prc::Call, 100, 100, -0.01, 0.0, 1.0, 0.2), " put", bs_price(prc::Put, 100, 100, -0.01, 0.0, 1.0, 0.2));
  out.writeln("negative dividend yield -1%: call", bs_price(prc::Call, 100, 100, 0.02, -0.01, 1.0, 0.2), " put", bs_price(prc::Put, 100, 100, 0.02, -0.01, 1.0, 0.2));
}
```

Output

```text
far out of the money call (spot 100, strike 500)          0.00000000
far in the money put (spot 100, strike 500)               393.78890025  discounted intrinsic 393.78890025
far in the money call (spot 500, strike 100)              401.24221995  discounted intrinsic 401.24221995
volatility 1e-6, strike 90: price 12.40921913  discounted forward intrinsic 12.40921913
volatility 50:              price 98.01986733  discounted spot 98.01986733
maturity 1e-8, strike 95: price 5.00000003  intrinsic 5
negative rate -1%:          call 7.51305824  put 8.51807495
negative dividend yield -1%: call 9.50800966  put 6.52286028
```

Far out of the money the call is 0, not `NaN`; far in the money it equals the discounted intrinsic value (393.7889 and 401.2422); a volatility of 10⁻⁶ gives the discounted forward intrinsic value 12.4092, and a volatility of 50 the discounted spot 98.0199. A maturity of 10⁻⁸ prices at the intrinsic value to eight decimals. A negative rate lowers the call to 7.513 and raises the put to 8.518, and a negative dividend yield raises the call to 9.508 and lowers the put to 6.523, as a lower forward and a higher forward would.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="batches_of_spots_and_maturities.html">
    <span class="label">Previous</span>
    <span class="title">Batches of spots and maturities</span>
  </a>
</div>
<!-- apiref:nav:end -->
