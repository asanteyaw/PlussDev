<!-- generated from the_greeks.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The Greeks

[Back to Black-Scholes](../finance_01_black_scholes.md)

The Greeks are the sensitivities of the price to its inputs. Pluss returns the five that Black-Scholes defines in closed form, with these conventions:

| Greek | Meaning | Definition |
|---|---|---|
| `delta` | the change in price per unit change in the spot | ∂V/∂S |
| `gamma` | the change in delta per unit change in the spot | ∂²V/∂S² |
| `vega` | the change in price per **unit** (100 percentage points) change in volatility | ∂V/∂σ |
| `theta` | the change in price per **year** of calendar time, so negative for a long option that loses time value | ∂V/∂t = −∂V/∂T |
| `rho` | the change in price per **unit** (100 percentage points) change in the rate | ∂V/∂r |

Divide `vega` and `rho` by 100 for the change per percentage point, and `theta` by 365 (or 252) for the change per day.

```cpp
prc::BlackScholesGreeks prc::black_scholes_greeks(prc::OptionType type, double spot, double strike, double rate,
                                                  double dividend_yield, double maturity, double volatility);
double prc::black_scholes_delta(type, ...);   double prc::black_scholes_gamma(spot, ...);   double prc::black_scholes_vega(spot, ...);
double prc::black_scholes_theta(type, ...);   double prc::black_scholes_rho(type, ...);
```

`gamma` and `vega` are the same for a call and a put, so their single functions take no option type.

## Closed forms, checked

With n the standard normal density, the call and put Greeks are

- delta: e^(−qT) N(d₁) for the call, and −e^(−qT) N(−d₁) for the put;
- gamma: e^(−qT) n(d₁) / (S σ √T);
- vega: S e^(−qT) n(d₁) √T;
- theta (call): −S e^(−qT) n(d₁) σ / (2√T) − r K e^(−rT) N(d₂) + q S e^(−qT) N(d₁), and for the put −S e^(−qT) n(d₁) σ / (2√T) + r K e^(−rT) N(−d₂) − q S e^(−qT) N(−d₁);
- rho: K T e^(−rT) N(d₂) for the call and −K T e^(−rT) N(−d₂) for the put.

The program computes all ten values from these formulas and prints them beside the library's, together with the single-Greek functions.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double S = 100.0, K = 105.0, r = 0.05, q = 0.02, T = 0.75, v = 0.25;
  const distr::Normal standard(0.0, 1.0);
  auto at = [](double x) { return pluss::make_series(std::vector<double>{x}, {1}, as_f64()); };
  auto N = [&](double x) { return standard.cdf(at(x)).pull<double>()[0]; };
  auto n = [&](double x) { return standard.pdf(at(x)).pull<double>()[0]; };

  const double sqrt_t = std::sqrt(T);
  const double d1 = (std::log(S / K) + (r - q + 0.5 * v * v) * T) / (v * sqrt_t);
  const double d2 = d1 - v * sqrt_t;
  const double eq = std::exp(-q * T), er = std::exp(-r * T);

  const double delta_c = eq * N(d1), delta_p = -eq * N(-d1);
  const double gamma = eq * n(d1) / (S * v * sqrt_t);
  const double vega = S * eq * n(d1) * sqrt_t;
  const double theta_c = -S * eq * n(d1) * v / (2.0 * sqrt_t) - r * K * er * N(d2) + q * S * eq * N(d1);
  const double theta_p = -S * eq * n(d1) * v / (2.0 * sqrt_t) + r * K * er * N(-d2) - q * S * eq * N(-d1);
  const double rho_c = K * T * er * N(d2), rho_p = -K * T * er * N(-d2);

  const auto call = prc::black_scholes_greeks(prc::Call, S, K, r, q, T, v);
  const auto put = prc::black_scholes_greeks(prc::Put, S, K, r, q, T, v);
  out.writeln("        library (call, put)            closed form (call, put)");
  out.writeln("delta ", call.delta, put.delta, "  ", delta_c, delta_p);
  out.writeln("gamma ", call.gamma, put.gamma, "  ", gamma, gamma);
  out.writeln("vega  ", call.vega, put.vega, "  ", vega, vega);
  out.writeln("theta ", call.theta, put.theta, "  ", theta_c, theta_p);
  out.writeln("rho   ", call.rho, put.rho, "  ", rho_c, rho_p);

  out.writeln("single functions:", prc::black_scholes_delta(prc::Call, S, K, r, q, T, v), prc::black_scholes_gamma(S, K, r, q, T, v), prc::black_scholes_vega(S, K, r, q, T, v),
              prc::black_scholes_theta(prc::Call, S, K, r, q, T, v), prc::black_scholes_rho(prc::Call, S, K, r, q, T, v));
  out.writeln("per percentage point of volatility:", call.vega / 100.0, " per day of time (365):", call.theta / 365.0);
}
```

Output

```text
        library (call, put)            closed form (call, put)
delta  0.487378 -0.497734    0.487378 -0.497734
gamma  0.018150 0.018150    0.018150 0.018150
vega   34.032087 34.032087    34.032087 34.032087
theta  -6.766334 -3.679787    -6.766334 -3.679787
rho    31.036133 -44.815427    31.036133 -44.815427
single functions: 0.487378 0.018150 34.032087 -6.766334 31.036133
per percentage point of volatility: 0.340321  per day of time (365): -0.018538
```

All ten Greeks agree with the closed forms, and the single functions return the same values. For this option the delta of the call, 0.487, and of the put, −0.498, differ by e^(−qT) = 0.985, a relation checked below. A one-percentage-point rise in volatility adds 0.34 to the price, and a day of time costs 0.0185.

## Greeks as derivatives of the price

Each Greek can be recovered from prices alone, by bumping one input. The program prices the option at inputs moved by a small amount and compares central differences with the Greeks: delta from the spot, gamma from three spots, vega from the volatility, rho from the rate, and theta from the maturity (theta is minus the derivative with respect to the time to maturity).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const double S = 100.0, K = 105.0, r = 0.05, q = 0.02, T = 0.75, v = 0.25;
  auto price = [&](double s, double rate, double t, double vol) { return prc::black_scholes_price(prc::Call, s, K, rate, q, t, vol); };
  const auto g = prc::black_scholes_greeks(prc::Call, S, K, r, q, T, v);

  const double h = 1e-3;
  const double delta = (price(S + h, r, T, v) - price(S - h, r, T, v)) / (2.0 * h);
  const double gamma = (price(S + h, r, T, v) - 2.0 * price(S, r, T, v) + price(S - h, r, T, v)) / (h * h);
  const double vega = (price(S, r, T, v + h * 1e-3) - price(S, r, T, v - h * 1e-3)) / (2.0 * h * 1e-3);
  const double rho = (price(S, r + h * 1e-3, T, v) - price(S, r - h * 1e-3, T, v)) / (2.0 * h * 1e-3);
  const double theta = -(price(S, r, T + h * 1e-3, v) - price(S, r, T - h * 1e-3, v)) / (2.0 * h * 1e-3);

  out.writeln("             Greek       central difference     difference");
  out.writeln("delta ", g.delta, delta, g.delta - delta);
  out.writeln("gamma ", g.gamma, gamma, g.gamma - gamma);
  out.writeln("vega  ", g.vega, vega, g.vega - vega);
  out.writeln("rho   ", g.rho, rho, g.rho - rho);
  out.writeln("theta ", g.theta, theta, g.theta - theta);
}
```

Output

```text
             Greek       central difference     difference
delta  0.48737796 0.48737796 0.00000000
gamma  0.01815045 0.01815047 -0.00000003
vega   34.03208699 34.03208699 -0.00000000
rho    31.03613346 31.03613346 -0.00000000
theta  -6.76633415 -6.76633415 0.00000000
```

Each Greek equals the central difference of the price. The gamma difference is of order 10⁻⁸, the truncation error of the second difference, and the others agree to the digits shown.

## Relations between the Greeks

Two relations hold for every Black-Scholes price and make useful checks. First, put-call parity differentiates into relations between the call and put Greeks: delta_C − delta_P = e^(−qT), while gamma, and vega are equal, and rho_C − rho_P = K T e^(−rT). Second, the price satisfies the Black-Scholes equation, which ties theta, delta and gamma together:

theta + (r − q) S delta + ½ σ² S² gamma − r V = 0.

The program checks both for several spots and for a call and a put.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const double K = 100.0, r = 0.04, q = 0.015, T = 1.5, v = 0.3;
  out.writeln("spot    call-put delta - e^-qT   gamma difference   vega difference   rho difference - K T e^-rT");
  for (double S : {60.0, 90.0, 100.0, 120.0, 180.0}) {
    const auto c = prc::black_scholes_greeks(prc::Call, S, K, r, q, T, v);
    const auto p = prc::black_scholes_greeks(prc::Put, S, K, r, q, T, v);
    out.writeln(S, c.delta - p.delta - std::exp(-q * T), c.gamma - p.gamma, c.vega - p.vega, c.rho - p.rho - K * T * std::exp(-r * T));
  }
  out.writeln("residual of the Black-Scholes equation, theta + (r - q) S delta + v^2 S^2 gamma / 2 - r V:");
  for (auto type : {prc::Call, prc::Put}) {
    for (double S : {60.0, 100.0, 180.0}) {
      const auto g = prc::black_scholes_greeks(type, S, K, r, q, T, v);
      const double V = prc::black_scholes_price(type, S, K, r, q, T, v);
      out.writeln(type == prc::Call ? "call" : "put ", S, g.theta + (r - q) * S * g.delta + 0.5 * v * v * S * S * g.gamma - r * V);
    }
  }
}
```

Output

```text
spot    call-put delta - e^-qT   gamma difference   vega difference   rho difference - K T e^-rT
60.0000000000 -0.0000000000 0.0000000000 0.0000000000 0.0000000000
90.0000000000 0.0000000000 0.0000000000 0.0000000000 0.0000000000
100.0000000000 0.0000000000 0.0000000000 0.0000000000 0.0000000000
120.0000000000 0.0000000000 0.0000000000 0.0000000000 0.0000000000
180.0000000000 0.0000000000 0.0000000000 0.0000000000 0.0000000000
residual of the Black-Scholes equation, theta + (r - q) S delta + v^2 S^2 gamma / 2 - r V:
call 60.0000000000 -0.0000000000
call 100.0000000000 -0.0000000000
call 180.0000000000 -0.0000000000
put  60.0000000000 0.0000000000
put  100.0000000000 0.0000000000
put  180.0000000000 -0.0000000000
```

The parity relations hold to ten decimals at every spot: the call and put deltas differ by e^(−qT), the gammas and vegas are equal, and the rhos differ by K T e^(−rT). The Black-Scholes equation holds for the call and the put at every spot, with residuals of zero to ten decimals.

## Behaviour at maturity

At zero maturity the price is the intrinsic value, delta is the indicator that the option is in the money (one half exactly at the money), and the other Greeks are zero.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("spot   call price   delta   gamma   vega   theta   rho    (maturity 0, strike 100)");
  for (double spot : {90.0, 100.0, 110.0}) {
    const auto g = prc::black_scholes_greeks(prc::Call, spot, 100.0, 0.05, 0.02, 0.0, 0.2);
    out.writeln(spot, prc::black_scholes_price(prc::Call, spot, 100.0, 0.05, 0.02, 0.0, 0.2), g.delta, g.gamma, g.vega, g.theta, g.rho);
  }
}
```

Output

```text
spot   call price   delta   gamma   vega   theta   rho    (maturity 0, strike 100)
90.0000 0.0000 0.0000 0.0000 0.0000 0.0000 0.0000
100.0000 0.0000 0.5000 0.0000 0.0000 0.0000 0.0000
110.0000 10.0000 1.0000 0.0000 0.0000 0.0000 0.0000
```

At maturity the call is worth its intrinsic value (0 at 90, 0 at 100, 10 at 110), delta is 0, 0.5 and 1, and every other Greek is zero.
