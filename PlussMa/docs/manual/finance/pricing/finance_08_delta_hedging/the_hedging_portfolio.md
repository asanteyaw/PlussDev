<!-- generated from the_hedging_portfolio.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The hedging portfolio

[Back to Delta Hedging](../finance_08_delta_hedging.md)

A trader who has sold a European option owes a random payoff at maturity. **Delta hedging** removes most of that randomness: the trader holds {math}`\Delta_t` shares of the asset, where {math}`\Delta_t=\partial V/\partial S` is the delta of the option, and adjusts the position as the price moves. If the position were adjusted continuously the payoff would be replicated exactly; adjusted at discrete dates it is replicated with an error, which is what `hedging::delta_hedge` measures.

## The self-financing portfolio

At time 0 the trader receives the option price {math}`V_0`, buys {math}`\Delta_0` shares, and puts the rest in a cash account earning the rate {math}`r`:

```{math}
B_0=V_0-\Delta_0S_0.
```

At each later date {math}`t_i=i\,\Delta t` the cash grows with interest, collects the dividends of the shares held, and pays for the change in the position:

```{math}
B_{i}=B_{i-1}e^{r\Delta t}+\Delta_{i-1}\,q\,\Delta t\,\tfrac{S_{i-1}+S_i}{2}-\big(\Delta_i-\Delta_{i-1}\big)S_i .
```

No money is added or removed: the portfolio is **self-financing**. Its value is {math}`X_i=\Delta_iS_i+B_i`, and the **hedge error** is {math}`X_i-V_i`, the difference between what the hedge portfolio is worth and what the option is worth. At maturity {math}`V_T` is the payoff, so the terminal error is the profit of the trader who sold the option and hedged it:

```{math}
\text{P\&L}=X_T-(S_T-K)^+ .
```

The option value {math}`V_i` and the delta are the field's own, evaluated at the spot of date {math}`i` and the time to maturity {math}`\tau_i=T-t_i` left then; at maturity the option is worth its payoff and the delta is the step function. For Black-Scholes they are the closed forms at the hedge volatility {math}`\sigma`; for Heston, say, the delta is obtained by bumping the spot and repricing with the field's engine. The path of the asset is data: any path, simulated or observed, on equally spaced dates.

```cpp
hedging::DeltaHedgeResult hedging::delta_hedge(const module::Field &field, const Series &spot_path, Config cfg = {});
```

| Argument | Meaning |
|---|---|
| `field` | the option being hedged, as a `Field` of one European option: its strike and maturity (the step is {math}`\Delta t=\text{maturity}/(n-1)`), its rate and dividend yield, and the model and engine that give the option's value and delta. The hedge works for any model the field prices; the programs here use Black-Scholes, where the delta is a closed form |
| `spot_path` | the asset at equally spaced dates from 0 to the field's maturity, a vector or a column, at least 2 values, all positive and finite |

| `DeltaHedgeResult` member | Contents |
|---|---|
| `option_value` | {math}`V_i`, the option's Black-Scholes value at each date |
| `delta` | {math}`\Delta_i`, the shares held after the rebalancing at each date |
| `cash` | {math}`B_i`, the cash account |
| `portfolio` | {math}`X_i=\Delta_iS_i+B_i` |
| `hedge_error` | {math}`X_i-V_i` |
| `terminal_pnl` | the last `hedge_error`, the profit at maturity |

All five series have the length of the path and are `Float64`.

## Program 1: one hedge, date by date

The program takes one year of the asset in monthly steps (a path simulated with a real-world drift of 8% and a volatility of 20%; the hedger's profit does not depend on the drift, only on the path) as data, and hedges a one-year at-the-money call at 20% volatility with 12 rebalances. It prints, month by month, the asset, the option's value, its delta, the cash, the portfolio and the hedge error.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace hdg = fin::hedging;
using namespace pluss::tils;

// A Black-Scholes field for the option being hedged: spot, strike, maturity, rate, yield and the volatility the hedge is computed with.
mod::Field option(fin::pricing::OptionType type, double strike, double maturity, double rate, double yield, double sigma) {
  return mod::Field{.instrument = fin::instrument::make(type, strike, maturity),
                    .market = fin::market::make(100.0, rate, yield),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({sigma})},
                    .controls = {.engine = mod::Engine::Analytic}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double strike = 100.0, rate = 0.02, sigma = 0.2, maturity = 1.0;
  // one year of the asset in monthly steps (a path simulated at a real-world drift of 8% and a volatility of 20%), as data
  const auto path = pluss::make_series({100.0, 106.5354, 113.2112, 115.2376, 107.4928, 115.1139, 121.9530, 118.0589, 109.8058, 104.6612, 105.4515, 110.8091, 112.2183}, {13}, as_f64());
  out.writeln("path", path.sizes());

  const auto h = hdg::delta_hedge(option(fin::pricing::Call, strike, maturity, rate, 0.0, sigma), path, as_f64());
  const auto S = path.items<double>(), V = h.option_value.items<double>(), D = h.delta.items<double>(), B = h.cash.items<double>(), X = h.portfolio.items<double>(), E = h.hedge_error.items<double>();
  out.writeln("month  spot      option    delta    cash       portfolio   portfolio - option");
  for (std::size_t i = 0; i < S.size(); ++i) {
    out.writeln(i, S[i], V[i], D[i], B[i], X[i], E[i]);
  }
  out.writeln("terminal P&L", h.terminal_pnl, " payoff", std::max(S.back() - strike, 0.0), " portfolio at expiry", X.back());
}
```

Output

```text
path [13]
month  spot      option    delta    cash       portfolio   portfolio - option
0 100.0000 8.9160 0.5793 -49.0099 8.9160 0.0000
1 106.5354 12.6799 0.6992 -61.8694 12.6200 -0.0599
2 113.2112 17.2949 0.8057 -74.0315 17.1845 -0.1105
3 115.2376 18.5572 0.8394 -78.0379 18.6937 0.1365
4 107.4928 12.0149 0.7277 -66.1561 12.0625 0.0475
5 115.1139 17.6344 0.8586 -81.3427 17.4977 -0.1367
6 121.9530 23.4302 0.9388 -91.2553 23.2343 -0.1959
7 118.0589 19.4586 0.9215 -89.3615 19.4263 -0.0324
8 109.8058 11.7532 0.8227 -78.6614 11.6722 -0.0809
9 104.6612 7.1596 0.7108 -67.0794 7.3087 0.1491
10 105.4515 6.9982 0.7678 -73.2115 7.7585 0.7604
11 110.8091 11.0609 0.9668 -95.3782 11.7502 0.6894
12 112.2183 12.2183 1.0000 -99.2648 12.9535 0.7352
terminal P&L 0.7352  payoff 12.2183  portfolio at expiry 12.9535
```

The option starts at 8.9160, the Black-Scholes price, with a delta of 0.5793: the trader holds 0.5793 shares worth 57.93, financed by borrowing 49.01 (the negative cash). As the asset rises, to 113 in month 2 and 122 in month 6, the delta rises to 0.81 and then 0.94, and the trader buys more shares with more borrowing: a delta hedge buys when the price rises and sells when it falls. In month 12 the asset is 112.2, the option is in the money and the delta is exactly 1.0000: the trader holds one share and owes 99.26, the strike discounted along the path. The payoff is 12.2183, the hedge portfolio is worth 12.9535, and the **terminal P&L is 0.7352**: the discrete hedge cost less than the option paid, and the trader made 0.74 on this path. On another path it can be negative; the next page measures the distribution.

In months 1 to 9 the error stays below 0.2 in absolute value, and then it jumps in month 10 from 0.15 to 0.76. That is the gamma profit of one calm step: the asset moved by 0.8, or 0.75%, when a move of {math}`\sigma\sqrt{\Delta t}=5.8\%` is typical, and the option, with two months left and the asset at the strike, had a gamma of 0.033, so the seller earned about {math}`\tfrac12\Gamma S^2\sigma^2\Delta t=0.60` of time decay without paying for movement. The next page shows that such steps average out.

## Program 2: the cash recursion by hand

The numbers of Program 1 follow from the recursion above. The program checks it on a five-date path: it takes the hedge's option value and delta, applies the cash recursion itself, and prints the hedge's cash beside its own at every date; it then compares the initial option value with the price of the field.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace hdg = fin::hedging;
using namespace pluss::tils;

// A Black-Scholes field for the option being hedged: spot, strike, maturity, rate, yield and the volatility the hedge is computed with.
mod::Field option(fin::pricing::OptionType type, double strike, double maturity, double rate, double yield, double sigma) {
  return mod::Field{.instrument = fin::instrument::make(type, strike, maturity),
                    .market = fin::market::make(100.0, rate, yield),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({sigma})},
                    .controls = {.engine = mod::Engine::Analytic}};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double rate = 0.02, sigma = 0.2;
  const std::vector<double> path{100.0, 101.0, 99.5, 103.0, 102.0};
  const double maturity = 1.0, strike = 100.0;
  const double dt = maturity / 4.0, growth = std::exp(rate * dt);

  const auto h = hdg::delta_hedge(option(fin::pricing::Call, strike, maturity, rate, 0.0, sigma), pluss::make_series(path, {5}, as_f64()), as_f64());
  const auto V = h.option_value.items<double>(), D = h.delta.items<double>(), B = h.cash.items<double>();
  out.writeln("step  cash from the hedge     cash by hand");
  double cash = V[0] - D[0] * path[0];
  out.writeln(0, B[0], cash);
  for (std::size_t t = 1; t < path.size(); ++t) {
    cash = cash * growth - (D[t] - D[t - 1]) * path[t];
    out.writeln(t, B[t], cash);
  }
  out.writeln("price at the start", V[0], " price of the field", option(fin::pricing::Call, strike, maturity, rate, 0.0, sigma).price().price.items<double>()[0]);
}
```

Output

```text
step  cash from the hedge     cash by hand
0 -49.00993 -49.00993
1 -50.46237 -50.46237
2 -45.83901 -45.83901
3 -57.56313 -57.56313
4 -93.16452 -93.16452
price at the start 8.91604  price of the field 8.91604
```

The hedge's cash and the hand-computed cash agree to the five decimals shown at each date, and the initial option value, 8.91604, is the field's price: the hedge starts from the model price. Anyone checking an alternative hedging strategy against this one can reuse the recursion with a different delta.

## Program 3: inputs, and what is rejected

`delta_hedge` checks its input before computing anything. The program offers it a series of bad inputs, and some that are fine.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace hdg = fin::hedging;
using namespace pluss::tils;

// A Black-Scholes field for the option being hedged: spot, strike, maturity, rate, yield and the volatility the hedge is computed with.
mod::Field option(fin::pricing::OptionType type, double strike, double maturity, double rate, double yield, double sigma) {
  return mod::Field{.instrument = fin::instrument::make(type, strike, maturity),
                    .market = fin::market::make(100.0, rate, yield),
                    .model = fin::model::BlackScholes{.volatility = pluss::make_series({sigma})},
                    .controls = {.engine = mod::Engine::Analytic}};
}

int main() {
  OutputEngine out;
  const std::vector<double> flat{100.0, 100.0, 100.0};
  const auto attempt = [&](const char *what, auto &&call) {
    try {
      call();
      out.writeln(std::string(what) + ": accepted");
    } catch (const std::exception &e) {
      std::string message = e.what();
      out.writeln(std::string(what) + ": " + message.substr(0, message.find('\n')));
    }
  };
  const auto hedge = [&](std::vector<double> path, double maturity = 1.0, double volatility = 0.2, fin::pricing::OptionType type = fin::pricing::Call) {
    return hdg::delta_hedge(option(type, 100.0, maturity, 0.02, 0.0, volatility), pluss::make_series(path, {static_cast<std::int64_t>(path.size())}, as_f64()), as_f64());
  };
  attempt("three dates", [&] { hedge(flat); });
  attempt("one date", [&] { hedge({100.0}); });
  attempt("negative spot", [&] { hedge({100.0, -1.0, 100.0}); });
  attempt("zero maturity", [&] { hedge(flat, 0.0); });
  attempt("a NaN spot", [&] { hedge({100.0, std::nan(""), 100.0}); });
  attempt("zero volatility", [&] { hedge(flat, 1.0, 0.0); });
  attempt("a put", [&] { hedge(flat, 1.0, 0.2, fin::pricing::Put); });
  attempt("a matrix of dates", [&] { hdg::delta_hedge(option(fin::pricing::Call, 100.0, 1.0, 0.02, 0.0, 0.2), pluss::series::full({3, 2}, 100.0, as_f64()), as_f64()); });
  attempt("a column of dates", [&] { hdg::delta_hedge(option(fin::pricing::Call, 100.0, 1.0, 0.02, 0.0, 0.2), pluss::series::full({3, 1}, 100.0, as_f64()), as_f64()); });
}
```

Output

```text
three dates: accepted
one date: subsp::math::finance::hedging::delta_hedge: spot_path requires at least 2 observations
negative spot: subsp::math::finance::hedging::delta_hedge: spot_path values must be > 0
zero maturity: subsp::math::finance::hedging::delta_hedge: maturity must be finite and > 0
a NaN spot: subsp::math::finance::hedging::delta_hedge: spot_path contains non-finite values
zero volatility: subsp::math::finance::module::Field: model.BlackScholes.volatility must be > 0
a put: accepted
a matrix of dates: subsp::math::finance::hedging::delta_hedge: expected vector or Nx1 matrix
a column of dates: accepted
```

A path needs at least two dates (the "one date" case), values that are finite ("a NaN spot") and positive ("negative spot"), and a field with a positive finite maturity; each is named in the message. A volatility of zero is rejected by the field's validation. A path of three equal prices is accepted: the hedge is then computed with nothing moving. A put works, with the put's value and delta, and a column vector (shape `[n, 1]`) is accepted as a path; a matrix of several columns is not, and the message says "expected vector or Nx1 matrix", since one call hedges one path. To hedge a set of paths, call it for each row of the simulated array, as the next page does.

## Checklist

1. Give the path on equally spaced dates from 0 to `maturity`; the number of rebalances is `n - 1`.
2. The hedge error is `portfolio - option_value`; at the last date it is the profit of the seller, `terminal_pnl`.
3. Hedge one path per call; select one path of a `[paths, steps + 1]` array with `select(0, i)`.
4. Put the volatility you hedge with in the field's model: it need not be the volatility the asset really has (next page).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="hedging_error.html">
    <span class="label">Next</span>
    <span class="title">The hedging error</span>
  </a>
</div>
<!-- apiref:nav:end -->
