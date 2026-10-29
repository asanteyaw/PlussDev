<!-- generated from batches_of_spots_and_maturities.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Batches of spots and maturities

[Back to Black-Scholes](../finance_01_black_scholes.md)

Every pricing function that takes a scalar spot has overloads that take a `Series` of spots, and a second form that takes a `Series` of times to maturity as well. A whole surface is then priced by one call, with no loop in the caller.

```cpp
Series prc::black_scholes_price(type, const Series &spot, double strike, double rate, double dividend_yield,
                                double maturity, double volatility, Config cfg = {});
Series prc::black_scholes_price(type, const Series &spot, const Series &time_to_maturity, double strike, double rate,
                                double dividend_yield, double volatility, Config cfg = {});
prc::BlackScholesSeriesGreeks prc::black_scholes_greeks(type, const Series &spot, ...);      // the same two forms
```

The spot (and the times to maturity) may have shape `[n]` or `[n, 1]`, and the two must have the same length: they are paired element by element, not combined into a grid. The result is `[n]`, in `Float64`, on the platform of the inputs. Spots must be positive and times to maturity non-negative and finite, element by element; a bad element is an error naming the argument. The Greeks of a batch are returned as a `BlackScholesSeriesGreeks` whose members `delta`, `gamma`, `vega`, `theta` and `rho` are `Series`.

## One call for a payoff diagram

The program prices a call and a put at 121 spots between 50 and 150, at 3 months to maturity, and compares each element with the scalar function.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double strike = 100.0, rate = 0.03, dividend = 0.01, maturity = 0.25, vol = 0.3;
  std::vector<double> grid;
  for (int i = 0; i <= 120; ++i) {
    grid.push_back(50.0 + i);
  }
  const auto spot = pluss::make_series(grid, {121}, as_f64());
  const auto call = prc::black_scholes_price(prc::Call, spot, strike, rate, dividend, maturity, vol);
  const auto put = prc::black_scholes_price(prc::Put, spot, strike, rate, dividend, maturity, vol);
  out.writeln("shape", call.sizes(), " dtype", call.dtype());

  double worst = 0.0;
  const auto c = call.pull<double>();
  for (int i = 0; i <= 120; ++i) {
    worst = std::max(worst, std::abs(c[i] - prc::black_scholes_price(prc::Call, grid[i], strike, rate, dividend, maturity, vol)));
  }
  out.writeln("largest difference from the scalar function", worst);
  out.writeln("spot    call       put        intrinsic call");
  for (int i : {0, 30, 50, 60, 70, 90, 120}) {
    out.writeln(grid[i], c[i], put.pull<double>()[i], std::max(grid[i] - strike, 0.0));
  }
}
```

Output

```text
shape [121]  dtype float64
largest difference from the scalar function 0.000000
spot    call       put        intrinsic call
50.000000 0.000005 49.377654 0.000000
80.000000 0.432972 19.885527 0.000000
100.000000 6.200781 5.703274 0.000000
110.000000 12.826263 2.353725 10.000000
120.000000 21.275118 0.827548 20.000000
140.000000 40.466731 0.069099 40.000000
170.000000 70.323580 0.000854 70.000000
```

The batch has shape `[121]` and `Float64`, and differs from the scalar function by less than 10⁻⁶ at every point (zero at the digits shown). The call lies above its intrinsic value everywhere and approaches it at both ends, the put mirrors it, and put-call parity relates the two columns.

## Spots and maturities together

When `time_to_maturity` is a `Series`, element *i* prices spot *i* at maturity *i*. A term structure of at-the-money options is one call with a constant spot and a range of maturities, and a path of an option as time passes is a spot path with a falling time to maturity. The program does the second: a spot path over 10 days, the option's remaining maturity shrinking from 30 days to 21 days, and its delta, for a one-month at-the-money call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double strike = 100.0, rate = 0.03, dividend = 0.0, vol = 0.25;
  const std::vector<double> path{100.0, 100.8, 99.9, 101.5, 102.4, 101.1, 100.2, 103.0, 104.1, 103.5};
  std::vector<double> remaining;
  for (int day = 0; day < 10; ++day) {
    remaining.push_back((30.0 - day) / 365.0);
  }
  const auto spot = pluss::make_series(path, {10}, as_f64());
  const auto ttm = pluss::make_series(remaining, {10}, as_f64());

  const auto value = prc::black_scholes_price(prc::Call, spot, ttm, strike, rate, dividend, vol);
  const auto greeks = prc::black_scholes_greeks(prc::Call, spot, ttm, strike, rate, dividend, vol);
  out.writeln("day  spot    days left   value   delta   gamma   theta per day");
  for (int day = 0; day < 10; ++day) {
    out.writeln(day, path[day], 30 - day, value.pull<double>()[day], greeks.delta.pull<double>()[day], greeks.gamma.pull<double>()[day], greeks.theta.pull<double>()[day] / 365.0);
  }

  // A term structure: one spot, many maturities.
  const auto terms = pluss::make_series(std::vector<double>{1.0 / 12, 0.25, 0.5, 1.0, 2.0}, {5}, as_f64());
  const auto flat = pluss::make_series(std::vector<double>(5, 100.0), {5}, as_f64());
  out.writeln("at-the-money call by maturity (1m, 3m, 6m, 1y, 2y):", prc::black_scholes_price(prc::Call, flat, terms, strike, rate, dividend, vol).pull<double>());
}
```

Output

```text
day  spot    days left   value   delta   gamma   theta per day
0 100.0000 30 2.9800 0.5280 0.0555 -0.0516
1 100.8000 29 3.3680 0.5723 0.0552 -0.0525
2 99.9000 28 2.8227 0.5213 0.0576 -0.0533
3 101.5000 27 3.6762 0.6124 0.0555 -0.0538
4 102.4000 26 4.1966 0.6631 0.0534 -0.0532
5 101.1000 25 3.3256 0.5915 0.0587 -0.0560
6 100.2000 24 2.7605 0.5374 0.0618 -0.0574
7 103.0000 23 4.4449 0.7028 0.0536 -0.0542
8 104.1000 22 5.1986 0.7626 0.0484 -0.0510
9 103.5000 21 4.6962 0.7365 0.0526 -0.0541
at-the-money call by maturity (1m, 3m, 6m, 1y, 2y): [3.0015, 5.3474, 7.7603, 11.3485, 16.7284]
```

As the days pass, the call's value follows the spot (2.98 at 100, 5.20 at 104.1) and its remaining time. The delta rises with the spot, from 0.528 to 0.763, and the daily theta is a loss of about five cents a day. The last line is a term structure: an at-the-money call is worth 3.00, 5.35, 7.76, 11.35 and 16.73 at maturities of one month to two years, rising roughly with the square root of the maturity.

## The Greeks of a batch, and the shape rules

The Greeks come back with one `Series` per Greek. The program prints the delta and gamma surfaces' profile across spots, confirms each equals the scalar Greek, and shows the accepted shapes and the errors for mismatched sizes and bad elements.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double strike = 100.0, rate = 0.03, dividend = 0.01, maturity = 0.5, vol = 0.2;
  const std::vector<double> grid{70, 85, 100, 115, 130};
  const auto spot = pluss::make_series(grid, {5}, as_f64());
  const auto g = prc::black_scholes_greeks(prc::Put, spot, strike, rate, dividend, maturity, vol);
  out.writeln("delta", g.delta.pull<double>());
  out.writeln("gamma", g.gamma.pull<double>());
  out.writeln("vega ", g.vega.pull<double>());
  double worst = 0.0;
  for (std::size_t i = 0; i < grid.size(); ++i) {
    const auto s = prc::black_scholes_greeks(prc::Put, grid[i], strike, rate, dividend, maturity, vol);
    worst = std::max({worst, std::abs(g.delta.pull<double>()[i] - s.delta), std::abs(g.gamma.pull<double>()[i] - s.gamma), std::abs(g.vega.pull<double>()[i] - s.vega),
                      std::abs(g.theta.pull<double>()[i] - s.theta), std::abs(g.rho.pull<double>()[i] - s.rho)});
  }
  out.writeln("largest difference from the scalar Greeks", worst);

  out.writeln("shapes: [5] gives", prc::black_scholes_price(prc::Call, spot, strike, rate, dividend, maturity, vol).sizes(), " [5, 1] gives",
              prc::black_scholes_price(prc::Call, spot.reshape({5, 1}), strike, rate, dividend, maturity, vol).sizes());
  auto attempt = [&](const char *label, auto &&call) {
    try {
      call();
      out.writeln(label, "-> no error");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      out.writeln(label, "->", message.substr(message.find("black_scholes_price: ") + 21, message.find('\n') - message.find("black_scholes_price: ") - 21));
    }
  };
  const auto two = pluss::make_series(std::vector<double>{0.5, 1.0}, {2}, as_f64());
  attempt("spot of 5, maturities of 2   ", [&] { prc::black_scholes_price(prc::Call, spot, two, strike, rate, dividend, vol); });
  attempt("a NaN spot                   ", [&] { prc::black_scholes_price(prc::Call, pluss::make_series(std::vector<double>{80, std::nan(""), 100}, {3}, as_f64()), strike, rate, dividend, maturity, vol); });
  attempt("a negative spot              ", [&] { prc::black_scholes_price(prc::Call, pluss::make_series(std::vector<double>{80, -1, 100}, {3}, as_f64()), strike, rate, dividend, maturity, vol); });
  attempt("a negative time to maturity  ", [&] { prc::black_scholes_price(prc::Call, pluss::make_series(std::vector<double>{80, 90, 100}, {3}, as_f64()), pluss::make_series(std::vector<double>{1, -1, 2}, {3}, as_f64()), strike, rate, dividend, vol); });
}
```

Output

```text
delta [-0.986415, -0.839010, -0.441555, -0.128659, -0.022819]
gamma [0.002357, 0.019873, 0.027789, 0.012894, 0.002942]
vega  [1.155153, 14.358585, 27.789495, 17.052947, 4.971805]
largest difference from the scalar Greeks 0.000000
shapes: [5] gives [5]  [5, 1] gives [5]
spot of 5, maturities of 2    -> spot and time_to_maturity size mismatch
a NaN spot                    -> spot contains non-finite values
a negative spot               -> spot values must be > 0
a negative time to maturity   -> time_to_maturity values must be >= 0
```

The delta of the put is near −1 deep in the money and near 0 deep out of the money. Gamma and vega peak near the strike (0.0278 and 27.8 at spot 100), which is where the price is most sensitive. Every element equals the scalar Greek, shapes `[5]` and `[5, 1]` both give `[5]`, and each invalid input is rejected with a message that names the argument.
