<!-- generated from edge_cases_and_errors.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Edge cases and errors

[Back to Black-Scholes](../finance_01_black_scholes.md)

## Inputs outside the model

The pricing functions reject any input for which the model is undefined, with a `pluss::Error` that names the function and the argument. The conditions are the same for the price, for the Greeks and for the batch forms (where each element is checked).

| Condition | Message (after the function name) |
|---|---|
| `spot` not finite and positive | `spot must be finite and > 0` |
| `strike` not finite and positive | `strike must be finite and > 0` |
| `volatility` not finite and positive | `volatility must be finite and > 0` |
| `maturity` negative or not finite | `maturity must be finite and >= 0` |
| `rate` or `dividend_yield` not finite | `rate must be finite`, `dividend_yield must be finite` |
| `type` is `OptionType::Auto` | `option type must be Call or Put` (`Auto` is for an instrument that carries its own call/put flag, in the object API) |
| batch inputs of different lengths | `spot and time_to_maturity size mismatch` |
| a bad element in a batch | `spot contains non-finite values`, `spot values must be > 0`, `time_to_maturity values must be >= 0` |

A zero volatility is an error, not the limit of the formula; use a small positive one, or the intrinsic value of the forward directly.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <limits>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  auto attempt = [&](const char *label, auto &&call) {
    try {
      call();
      out.writeln(label, "-> no error");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      const auto start = message.find("black_scholes_price: ") + 21;
      out.writeln(label, "->", message.substr(start, message.find('\n') - start));
    }
  };
  attempt("volatility 0     ", [&] { prc::black_scholes_price(prc::Call, 100, 100, 0.05, 0.0, 1.0, 0.0); });
  attempt("volatility -0.2  ", [&] { prc::black_scholes_price(prc::Call, 100, 100, 0.05, 0.0, 1.0, -0.2); });
  attempt("spot 0           ", [&] { prc::black_scholes_price(prc::Call, 0.0, 100, 0.05, 0.0, 1.0, 0.2); });
  attempt("spot NaN         ", [&] { prc::black_scholes_price(prc::Call, nan, 100, 0.05, 0.0, 1.0, 0.2); });
  attempt("strike 0         ", [&] { prc::black_scholes_price(prc::Call, 100, 0.0, 0.05, 0.0, 1.0, 0.2); });
  attempt("maturity -1      ", [&] { prc::black_scholes_price(prc::Call, 100, 100, 0.05, 0.0, -1.0, 0.2); });
  attempt("maturity infinity", [&] { prc::black_scholes_price(prc::Call, 100, 100, 0.05, 0.0, inf, 0.2); });
  attempt("rate infinity    ", [&] { prc::black_scholes_price(prc::Call, 100, 100, inf, 0.0, 1.0, 0.2); });
  attempt("dividend NaN     ", [&] { prc::black_scholes_price(prc::Call, 100, 100, 0.05, nan, 1.0, 0.2); });
  attempt("OptionType::Auto ", [&] { prc::black_scholes_price(prc::OptionType::Auto, 100, 100, 0.05, 0.0, 1.0, 0.2); });
  attempt("gamma, volatility 0", [&] { prc::black_scholes_gamma(100, 100, 0.05, 0.0, 1.0, 0.0); });
}
```

Output

```text
volatility 0      -> volatility must be finite and > 0
volatility -0.2   -> volatility must be finite and > 0
spot 0            -> spot must be finite and > 0
spot NaN          -> spot must be finite and > 0
strike 0          -> strike must be finite and > 0
maturity -1       -> maturity must be finite and >= 0
maturity infinity -> maturity must be finite and >= 0
rate infinity     -> rate must be finite
dividend NaN      -> dividend_yield must be finite
OptionType::Auto  -> option type must be Call or Put (OptionType::Auto is only valid for an instrument that supplies is_call)
gamma, volatility 0 -> ::pricing::black_scholes_greeks: volatility must be finite and > 0
```

Each of the eleven calls is rejected with the message in the table.

## Extremes stay finite and inside the bounds

The formula is evaluated stably at the extremes: far out of the money the price underflows to zero rather than to `NaN`, far in the money it approaches the discounted intrinsic value, a very small volatility gives the discounted forward intrinsic value, and a very large one gives the discounted spot. Negative rates and dividend yields are valid and shift the forward.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const double r = 0.05, q = 0.02, T = 0.25;
  out.writeln("far out of the money call (spot 100, strike 500)         ", prc::black_scholes_price(prc::Call, 100, 500, r, 0.0, T, 0.2));
  out.writeln("far in the money put (spot 100, strike 500)              ", prc::black_scholes_price(prc::Put, 100, 500, r, 0.0, T, 0.2), " discounted intrinsic", 500 * std::exp(-r * T) - 100);
  out.writeln("far in the money call (spot 500, strike 100)             ", prc::black_scholes_price(prc::Call, 500, 100, r, 0.0, T, 0.2), " discounted intrinsic", 500 - 100 * std::exp(-r * T));
  out.writeln("volatility 1e-6, strike 90: price", prc::black_scholes_price(prc::Call, 100, 90, r, q, 1.0, 1e-6), " discounted forward intrinsic", 100 * std::exp(-q) - 90 * std::exp(-r));
  out.writeln("volatility 50:              price", prc::black_scholes_price(prc::Call, 100, 100, r, q, 1.0, 50.0), " discounted spot", 100 * std::exp(-q));
  out.writeln("maturity 1e-8, strike 95: price", prc::black_scholes_price(prc::Call, 100, 95, r, q, 1e-8, 0.2), " intrinsic 5");
  out.writeln("negative rate -1%:          call", prc::black_scholes_price(prc::Call, 100, 100, -0.01, 0.0, 1.0, 0.2), " put", prc::black_scholes_price(prc::Put, 100, 100, -0.01, 0.0, 1.0, 0.2));
  out.writeln("negative dividend yield -1%: call", prc::black_scholes_price(prc::Call, 100, 100, 0.02, -0.01, 1.0, 0.2), " put", prc::black_scholes_price(prc::Put, 100, 100, 0.02, -0.01, 1.0, 0.2));
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
