<!-- generated from recovering_a_volatility.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Recovering a volatility

[Back to Implied Volatility](../finance_02_implied_volatility.md)

Option markets quote prices, but traders compare options by their *implied volatility*: the value of σ that makes the Black-Scholes price equal the observed price. Because the Black-Scholes price rises strictly with σ, that value is unique when it exists, and `implied_vol` finds it.

```cpp
double prc::implied_vol(prc::OptionType type, double option_price, double spot, double strike, double rate,
                        double dividend_yield, double maturity, const prc::ImpliedVolOptions &options = {});
Series prc::implied_vol(type, const Series &option_price, const Series &spot, double strike, double rate,
                        double dividend_yield, double maturity, const prc::ImpliedVolOptions &options = {}, Config cfg = {});
Series prc::implied_vol(type, const Series &option_price, const Series &spot, const Series &time_to_maturity, double strike,
                        double rate, double dividend_yield, const prc::ImpliedVolOptions &options = {}, Config cfg = {});
```

The arguments are those of `black_scholes_price`, with the observed `option_price` in place of the volatility. The result is a volatility, or `NaN` when no volatility in the search range reproduces the price ([Controls and the no-arbitrage bounds](controls_and_the_no_arbitrage_bounds.md)).

## A round trip

The program prices calls and puts at volatilities from 5% to 200%, at three strikes, and recovers each volatility from its price. The largest error over all of them is reported.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  const double spot = 100.0, rate = 0.03, dividend = 0.01, maturity = 0.5;
  double worst = 0.0, worst_vol = 0.0, worst_strike = 0.0, worst_price = 0.0;
  int recovered = 0, total = 0;
  for (auto type : {prc::Call, prc::Put}) {
    for (double strike : {80.0, 100.0, 125.0}) {
      for (double vol : {0.05, 0.1, 0.2, 0.4, 0.8, 1.5, 2.0}) {
        const double price = prc::black_scholes_price(type, spot, strike, rate, dividend, maturity, vol);
        const double iv = prc::implied_vol(type, price, spot, strike, rate, dividend, maturity);
        ++total;
        if (std::isfinite(iv)) {
          ++recovered;
          if (std::abs(iv - vol) > worst) {
            worst = std::abs(iv - vol);
            worst_vol = vol;
            worst_strike = strike;
            worst_price = price;
          }
        }
      }
    }
  }
  out.writeln("recovered", recovered, "of", total, " largest error", worst, " (volatility", worst_vol, "strike", worst_strike, "price", worst_price, ")");
  out.print_defaults(PrintOptions{}.fixed(8));
  const double price = prc::black_scholes_price(prc::Call, spot, 100.0, rate, dividend, maturity, 0.2345);
  out.writeln("one price", price, " implies", prc::implied_vol(prc::Call, price, spot, 100.0, rate, dividend, maturity));
}
```

Output

```text
recovered 42 of 42  largest error 7.85e-06  (volatility 5.00e-02 strike 8.00e+01 price 2.07e+01 )
one price 7.04878020  implies 0.23450000
```

All 42 prices, calls and puts at three strikes and seven volatilities, return a volatility. The largest error is 7.9 × 10⁻⁶, for the call at strike 80 and a volatility of 5%: its price, 20.7, is almost entirely intrinsic value, and its time value is so small that the price, at the precision of a double, pins the volatility down only to about the sixth decimal. Everywhere else the error is far smaller. The single price 7.0488 implies 0.2345, exactly the volatility that produced it.

## The five methods

`ImpliedVolOptions::method` selects the root finder: `Newton` (using the vega as the derivative), `Bisection` (slow and certain), `Secant`, `Brent` (the default, with `Auto` an alias for it), each safeguarded so that it stays inside the bracket [`lower_vol`, `upper_vol`]. All five solve the same equation, so they agree to the tolerance. The program recovers a volatility from a normal price, from a nearly worthless price (deep out of the money, short maturity, where the vega is tiny and the price is almost flat in σ) and from a deep in-the-money price.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  struct Case {
    const char *label;
    prc::OptionType type;
    double strike, maturity, vol;
  };
  const std::vector<Case> cases{{"at the money, 1 year        ", prc::Call, 100.0, 1.0, 0.3},
                                {"deep out of the money, 2 weeks", prc::Call, 160.0, 0.04, 0.3},
                                {"deep in the money, 1 year     ", prc::Call, 40.0, 1.0, 0.3},
                                {"put out of the money          ", prc::Put, 70.0, 0.5, 0.45}};
  const std::vector<std::pair<const char *, prc::ImpliedVolMethod>> methods{
      {"Auto", prc::Auto}, {"Newton", prc::Newton}, {"Bisection", prc::Bisection}, {"Secant", prc::Secant}, {"Brent", prc::Brent}};
  out.writeln("error of the recovered volatility, by method:");
  for (const auto &c : cases) {
    const double price = prc::black_scholes_price(c.type, 100.0, c.strike, 0.02, 0.0, c.maturity, c.vol);
    std::vector<double> errors;
    for (const auto &[name, method] : methods) {
      prc::ImpliedVolOptions options;
      options.method = method;
      errors.push_back(std::abs(prc::implied_vol(c.type, price, 100.0, c.strike, 0.02, 0.0, c.maturity, options) - c.vol));
    }
    out.writeln(c.label, " price", price, errors);
  }
}
```

Output

```text
error of the recovered volatility, by method:
at the money, 1 year          price 1.28e+01 [4.39e-15, 4.04e-10, 6.32e-09, 4.97e-13, 4.39e-15]
deep out of the money, 2 weeks  price 2.48e-15 [8.35e-12, 6.32e-09, 6.32e-09, 6.32e-09, 8.35e-12]
deep in the money, 1 year       price 6.08e+01 [1.47e-12, 4.30e-14, 6.32e-09, 1.52e-10, 1.47e-12]
put out of the money            price 1.62e+00 [1.44e-15, 6.11e-16, 5.08e-10, 1.78e-12, 1.44e-15]
```

All five methods recover the volatility to 10⁻⁸ or better in every case, including the price of 2.5 × 10⁻¹⁵ of an option 60% out of the money two weeks from expiry, where the price is nearly flat in the volatility. Newton and Secant use the vega or a secant to reach the answer in a few steps, Brent takes a similar number, and bisection reaches the tolerance (6 × 10⁻⁹) by halving.

## Many prices at once

With `Series` arguments each element is solved independently, and an element with no solution is `NaN` without affecting the others. The program recovers the volatilities of a path of one-month option prices, as the spot and the remaining maturity change, and then inserts an impossible price.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double strike = 100.0, rate = 0.03, dividend = 0.0;
  const std::vector<double> path{100.0, 100.8, 99.9, 101.5, 102.4, 101.1};
  std::vector<double> remaining, true_vol{0.22, 0.24, 0.23, 0.21, 0.20, 0.25};
  for (int day = 0; day < 6; ++day) {
    remaining.push_back((30.0 - day) / 365.0);
  }
  const auto spot = pluss::make_series(path, {6}, as_f64());
  const auto ttm = pluss::make_series(remaining, {6}, as_f64());

  // Each element is priced at its own volatility, then inverted.
  std::vector<double> prices;
  for (int i = 0; i < 6; ++i) {
    prices.push_back(prc::black_scholes_price(prc::Call, path[i], strike, rate, dividend, remaining[i], true_vol[i]));
  }
  const auto price_series = pluss::make_series(prices, {6}, as_f64());
  out.writeln("recovered", prc::implied_vol(prc::Call, price_series, spot, ttm, strike, rate, dividend).pull<double>());
  out.writeln("true     ", true_vol);

  // With one maturity for every element, the maturity is a scalar.
  out.writeln("fixed maturity of 30 days:", prc::implied_vol(prc::Call, price_series, spot, strike, rate, dividend, 30.0 / 365.0).pull<double>());

  // One impossible price (below the intrinsic value) yields NaN for that element only.
  prices[3] = 0.0;
  out.writeln("with a zero price in the fourth element:", prc::implied_vol(prc::Call, pluss::make_series(prices, {6}, as_f64()), spot, ttm, strike, rate, dividend).pull<double>());
}
```

Output

```text
recovered [0.220000, 0.240000, 0.230000, 0.210000, 0.200000, 0.250000]
true      [0.220000, 0.240000, 0.230000, 0.210000, 0.200000, 0.250000]
fixed maturity of 30 days: [0.220000, 0.235570, 0.221488, 0.197862, 0.184018, 0.226149]
with a zero price in the fourth element: [0.220000, 0.240000, 0.230000, nan, 0.200000, 0.250000]
```

Each element is solved independently and returns its own volatility, 0.22, 0.24, 0.23, 0.21, 0.20 and 0.25, using its own remaining maturity. The scalar-maturity form applies the same 30 days to every element, so it returns different volatilities (0.2356 for the second): the remaining maturity matters. A zero price in the fourth element gives `NaN` there and leaves the others unchanged.
