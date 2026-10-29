<!-- generated from controls_and_the_no_arbitrage_bounds.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Controls and the no-arbitrage bounds

[Back to Implied Volatility](../finance_02_implied_volatility.md)

```cpp
struct prc::ImpliedVolOptions {
  prc::ImpliedVolMethod method = prc::ImpliedVolMethod::Auto;
  double lower_vol = 1e-8;
  double upper_vol = 10.0;
  double tolerance = 1e-8;
  std::int64_t max_iterations = 128;
  double vega_floor = 1e-12;
};
```

| Option | Meaning |
|---|---|
| `method` | `Auto` and `Brent` (the default), `Newton`, `Secant` or `Bisection` |
| `lower_vol`, `upper_vol` | the search bracket; the answer must lie inside it. `lower_vol` must be positive and `upper_vol` greater than it |
| `tolerance` | the answer is accepted when the price error is at most `tolerance` times the option's time value (its price less the discounted intrinsic value, the whole price out of the money), or the bracket is narrower than `tolerance`; must be positive |
| `max_iterations` | the iteration cap of the chosen method; must be positive |
| `vega_floor` | Newton's step divides by the vega; where it is below this floor a bisection step is taken instead; must be positive |

## When there is no answer: `NaN`

An option price must respect the no-arbitrage bounds: for a call, max(S e^(−qT) − K e^(−rT), 0) < C < S e^(−qT), and for a put the mirror image. A price outside them, or inside them but outside the volatilities the search bracket allows, has no implied volatility. `implied_vol` then returns `NaN`, in the scalar form as in the `Series` forms, so that one bad quote in a batch does not stop the rest. The same applies when an input is not valid for pricing (a non-positive spot, strike or maturity, a negative or non-finite price). Invalid *options* and `OptionType::Auto` are errors, not `NaN`.

The program asks for the implied volatility of a series of prices for a call with spot 100 and strike 100, one year, rate 3%: a sensible one, and one for each way the question can fail.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <limits>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double nan = std::numeric_limits<double>::quiet_NaN();
  static constexpr double S = 100.0, K = 100.0, T = 1.0;
  const double r = 0.03, q = 0.0;
  const double floor_price = std::max(S - K * std::exp(-r * T), 0.0);
  out.writeln("no-arbitrage bounds for the call: (", floor_price, ", ", S, ")");
  auto ask = [&](const char *label, double price, double spot = S, double strike = K, double maturity = T) {
    out.writeln(label, " price", price, " ->", prc::implied_vol(prc::Call, price, spot, strike, r, q, maturity));
  };
  ask("a sensible price               ", 9.41);
  ask("below the intrinsic value      ", 2.0);
  ask("equal to the intrinsic value   ", floor_price);
  ask("zero                           ", 0.0);
  ask("negative                       ", -1.0);
  ask("not a number                   ", nan);
  ask("above the spot                 ", 120.0);
  ask("equal to the spot              ", 100.0);
  ask("zero maturity                  ", 9.41, S, K, 0.0);
  ask("negative spot                  ", 9.41, -100.0);
  ask("zero strike                    ", 9.41, S, 0.0);
}
```

Output

```text
no-arbitrage bounds for the call: ( 2.955447 ,  100.000000 )
a sensible price                 price 9.410000  -> 0.199912
below the intrinsic value        price 2.000000  -> nan
equal to the intrinsic value     price 2.955447  -> 0.000000
zero                             price 0.000000  -> nan
negative                         price -1.000000  -> nan
not a number                     price nan  -> nan
above the spot                   price 120.000000  -> nan
equal to the spot                price 100.000000  -> nan
zero maturity                    price 9.410000  -> nan
negative spot                    price 9.410000  -> nan
zero strike                      price 9.410000  -> nan
```

For a call with spot 100, strike 100 and one year at 3%, prices must lie between 2.9554 and 100. A price of 9.41 implies 0.1999. A price below the intrinsic value, zero, negative, not a number, above the spot or equal to the spot has no implied volatility and gives `NaN`; so do a zero maturity, a negative spot and a zero strike. A price exactly at the intrinsic value corresponds to a volatility of zero and returns the lower end of the bracket.

## The search bracket

The answer is searched for between `lower_vol` and `upper_vol`. A price whose volatility lies outside that range returns `NaN`, so the bracket is also a plausibility filter: restrict it to the volatilities you are willing to accept. The program inverts the price of a 120% volatility with brackets that exclude and include it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double price = prc::black_scholes_price(prc::Call, 100.0, 100.0, 0.03, 0.0, 1.0, 1.2);
  for (auto [lower, upper] : {std::pair<double, double>{1e-8, 10.0}, {1e-8, 1.0}, {1.3, 5.0}, {0.5, 1.2}, {1.2, 2.0}}) {
    prc::ImpliedVolOptions options;
    options.lower_vol = lower;
    options.upper_vol = upper;
    out.writeln("bracket [", lower, ",", upper, "] ->", prc::implied_vol(prc::Call, price, 100.0, 100.0, 0.03, 0.0, 1.0, options));
  }
}
```

Output

```text
bracket [ 0.000000 , 10.000000 ] -> 1.200000
bracket [ 0.000000 , 1.000000 ] -> nan
bracket [ 1.300000 , 5.000000 ] -> nan
bracket [ 0.500000 , 1.200000 ] -> 1.200000
bracket [ 1.200000 , 2.000000 ] -> 1.200000
```

A bracket that contains 1.2 returns it, including a bracket whose end is 1.2 (the endpoints are included). Brackets [10⁻⁸, 1] and [1.3, 5] exclude it and give `NaN`.

## Tolerance and iterations

`tolerance` bounds the price error that is accepted, relative to the time value, which is the part of the price that carries the volatility, so a price of 10⁻¹⁵ is resolved as sharply as a price of 10. The program shows the error of the recovered volatility as the tolerance tightens, and then, for each method, the fewest iterations that suffice: a solver that is cut off before it converges returns `NaN`, not its last iterate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  const double vol = 0.35;
  const double price = prc::black_scholes_price(prc::Call, 100.0, 110.0, 0.03, 0.0, 0.5, vol);

  out.writeln("error by tolerance:");
  for (double tolerance : {1e-2, 1e-4, 1e-6, 1e-8, 1e-10, 1e-12}) {
    prc::ImpliedVolOptions options;
    options.tolerance = tolerance;
    out.writeln("  tolerance", tolerance, " error", std::abs(prc::implied_vol(prc::Call, price, 100.0, 110.0, 0.03, 0.0, 0.5, options) - vol));
  }

  out.print_defaults(PrintOptions{}.fixed(0));
  out.writeln("fewest iterations that give an answer, by method (tolerance 1e-8):");
  const std::vector<std::pair<const char *, prc::ImpliedVolMethod>> methods{{"Newton   ", prc::Newton}, {"Secant   ", prc::Secant}, {"Brent    ", prc::Brent}, {"Bisection", prc::Bisection}};
  for (const auto &[name, method] : methods) {
    int needed = -1;
    for (int limit = 1; limit <= 200 && needed < 0; ++limit) {
      prc::ImpliedVolOptions options;
      options.method = method;
      options.max_iterations = limit;
      if (std::isfinite(prc::implied_vol(prc::Call, price, 100.0, 110.0, 0.03, 0.0, 0.5, options))) {
        needed = limit;
      }
    }
    out.writeln("  ", name, needed);
  }
}
```

Output

```text
error by tolerance:
  tolerance 1.00e-02  error 2.22e-04
  tolerance 1.00e-04  error 5.88e-07
  tolerance 1.00e-06  error 1.29e-12
  tolerance 1.00e-08  error 1.29e-12
  tolerance 1.00e-10  error 1.29e-12
  tolerance 1.00e-12  error 5.55e-17
fewest iterations that give an answer, by method (tolerance 1e-8):
   Newton    5
   Secant    5
   Brent     6
   Bisection 29
```

The error in the volatility falls as the tolerance tightens, from 2.2 × 10⁻⁴ at 10⁻² to about 10⁻¹² at 10⁻⁶ and below, where the solver has reached the precision of the price. Newton and Secant need 5 iterations, Brent 6, and bisection 29, the number of halvings of the bracket [10⁻⁸, 10] down to the tolerance. With fewer iterations than these, the solver returns `NaN`.

## The vega floor, and invalid options

Deep out of the money the price hardly depends on σ, so the vega is almost zero and Newton's step would be enormous. Where the vega is below `vega_floor` the solver takes a bisection step instead, which is why the Newton method still finds an answer in that case. Settings that cannot define a search are errors: a non-positive `lower_vol`, tolerance, iteration count or vega floor, and an `upper_vol` that is not above `lower_vol`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  const double vol = 0.3;
  const double price = prc::black_scholes_price(prc::Call, 100.0, 220.0, 0.02, 0.0, 0.1, vol);
  out.writeln("a call 120% out of the money, 5 weeks: price", price, " vega", prc::black_scholes_vega(100.0, 220.0, 0.02, 0.0, 0.1, vol));
  for (double floor : {1e-12, 1e-3, 1.0}) {
    prc::ImpliedVolOptions options;
    options.method = prc::Newton;
    options.vega_floor = floor;
    out.writeln("Newton, vega_floor", floor, " -> error", std::abs(prc::implied_vol(prc::Call, price, 100.0, 220.0, 0.02, 0.0, 0.1, options) - vol));
  }

  auto attempt = [&](const char *label, auto edit) {
    prc::ImpliedVolOptions options;
    edit(options);
    try {
      prc::implied_vol(prc::Call, 9.41, 100.0, 100.0, 0.03, 0.0, 1.0, options);
      out.writeln(label, "-> no error");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      const auto start = message.find("implied_vol: ") + 13;
      out.writeln(label, "->", message.substr(start, message.find('\n') - start));
    }
  };
  attempt("lower_vol = 0         ", [](prc::ImpliedVolOptions &o) { o.lower_vol = 0.0; });
  attempt("upper_vol below lower ", [](prc::ImpliedVolOptions &o) { o.lower_vol = 1.0; o.upper_vol = 0.5; });
  attempt("tolerance = 0         ", [](prc::ImpliedVolOptions &o) { o.tolerance = 0.0; });
  attempt("max_iterations = 0    ", [](prc::ImpliedVolOptions &o) { o.max_iterations = 0; });
  attempt("vega_floor = 0        ", [](prc::ImpliedVolOptions &o) { o.vega_floor = 0.0; });
  try {
    prc::implied_vol(prc::OptionType::Auto, 9.41, 100.0, 100.0, 0.03, 0.0, 1.0);
  } catch (const std::exception &e) {
    const std::string message = e.what();
    const auto start = message.find("implied_vol: ") + 13;
    out.writeln("OptionType::Auto      ->", message.substr(start, message.find('\n') - start));
  }
}
```

Output

```text
a call 120% out of the money, 5 weeks: price 9.33e-17  vega 2.23e-14
Newton, vega_floor 1.00e-12  -> error 6.32e-09
Newton, vega_floor 1.00e-03  -> error 6.32e-09
Newton, vega_floor 1.00e+00  -> error 6.32e-09
lower_vol = 0          -> lower_vol must be finite and > 0
upper_vol below lower  -> upper_vol must be finite and > lower_vol
tolerance = 0          -> tolerance must be finite and > 0
max_iterations = 0     -> max_iterations must be > 0
vega_floor = 0         -> vega_floor must be finite and > 0
OptionType::Auto      -> option type must be Call or Put (OptionType::Auto is only valid for an instrument that supplies is_call)
```

A call 120% out of the money five weeks from expiry is worth 9 × 10⁻¹⁷, and its vega is 2 × 10⁻¹⁴, nearly nothing. Newton still recovers the volatility to 6 × 10⁻⁹ for every `vega_floor`, because where its step is unusable it falls back to a bisection step. Every invalid setting is rejected with the message shown, and so is `OptionType::Auto`.
