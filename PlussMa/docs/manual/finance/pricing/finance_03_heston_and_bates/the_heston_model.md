<!-- generated from the_heston_model.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The Heston model

[Back to Heston and Bates](../finance_03_heston_and_bates.md)

In the Black-Scholes model the volatility is a constant, which no option market supports: implied volatilities vary with strike and maturity. The Heston model lets the variance *v* of the asset move randomly:

dS = (r − q) S dt + √v S dW₁, dv = κ (θ − v) dt + σ √v dW₂, with corr(dW₁, dW₂) = ρ.

The variance reverts to its long-run level θ at speed κ, σ is the volatility of the variance, ρ the correlation between the asset and its variance (negative for equities: the variance rises when the price falls), and v₀ the starting variance. The characteristic function of ln S_T is known in closed form, so a European option price is a one-dimensional integral.

```cpp
struct vol::HestonParams {
  double kappa = 1.5, theta = 0.04, sigma = 0.3, rho = -0.7, v0 = 0.04;
  double mu = 0.0;                 // the drift of simulated paths; pricing uses r - q
  double variance_floor = 1e-12;   // the floor of simulated variances
};
double prc::heston_price(prc::OptionType type, double spot, double strike, double rate, double dividend_yield,
                         double maturity, const vol::HestonParams &params, const prc::HestonIntegrationOptions &options = {});
```

`vol` is `pluss::subsp::math::finance::volatility`. The parameters `kappa`, `theta`, `sigma` and `v0` must be positive and `rho` must lie strictly between −1 and 1. The Feller condition 2κθ ≥ σ², which keeps the variance away from zero, is not required: the formula is valid without it, and simulated paths are floored at `variance_floor`.

## Checks

Three properties pin the price down. Put-call parity holds exactly, as for any model. As σ → 0 with v₀ = θ the variance is constant and equal to θ, so the price must approach the Black-Scholes price at volatility √θ. And the price depends on the correlation: with ρ < 0 a call struck above the money is cheaper, and one struck below it dearer, than with ρ = 0.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double spot = 100.0, rate = 0.03, dividend = 0.01, maturity = 1.0;
  vol::HestonParams params;  // kappa 1.5, theta 0.04, sigma 0.3, rho -0.7, v0 0.04

  out.writeln("strike   call       put        call - put   S e^-qT - K e^-rT");
  for (double strike : {80.0, 100.0, 120.0}) {
    const double c = prc::heston_price(prc::Call, spot, strike, rate, dividend, maturity, params);
    const double p = prc::heston_price(prc::Put, spot, strike, rate, dividend, maturity, params);
    out.writeln(strike, c, p, c - p, spot * std::exp(-dividend * maturity) - strike * std::exp(-rate * maturity));
  }

  out.writeln("vol of vol sigma -> 0 (v0 = theta = 0.04), against Black-Scholes at volatility 0.2:");
  for (double sigma : {0.3, 0.1, 0.01, 0.001}) {
    vol::HestonParams p = params;
    p.sigma = sigma;
    out.writeln("  sigma", sigma, " Heston", prc::heston_price(prc::Call, spot, 100.0, rate, dividend, maturity, p), " Black-Scholes", prc::black_scholes_price(prc::Call, spot, 100.0, rate, dividend, maturity, 0.2));
  }

  out.writeln("correlation rho, calls struck at 80, 100, 120:");
  for (double rho : {-0.7, 0.0, 0.7}) {
    vol::HestonParams p = params;
    p.rho = rho;
    out.writeln("  rho", rho, prc::heston_price(prc::Call, spot, 80.0, rate, dividend, maturity, p), prc::heston_price(prc::Call, spot, 100.0, rate, dividend, maturity, p), prc::heston_price(prc::Call, spot, 120.0, rate, dividend, maturity, p));
  }
}
```

Output

```text
strike   call       put        call - put   S e^-qT - K e^-rT
80.000000 22.831101 1.461760 21.369341 21.369341
100.000000 8.542257 6.581827 1.960430 1.960430
120.000000 1.476924 18.925405 -17.448481 -17.448481
vol of vol sigma -> 0 (v0 = theta = 0.04), against Black-Scholes at volatility 0.2:
  sigma 0.300000  Heston 8.542257  Black-Scholes 8.827321
  sigma 0.100000  Heston 8.794675  Black-Scholes 8.827321
  sigma 0.010000  Heston 8.826992  Black-Scholes 8.827321
  sigma 0.001000  Heston 8.827318  Black-Scholes 8.827321
correlation rho, calls struck at 80, 100, 120:
  rho -0.700000 22.831101 8.542257 1.476924
  rho 0.000000 22.366997 8.576689 2.459869
  rho 0.700000 21.755116 8.528151 3.233592
```

Parity holds to the digits shown at each strike (21.3693, 1.9604 and −17.4485). As σ falls, the Heston price approaches the Black-Scholes price at volatility 0.2: 8.5423 at σ = 0.3, 8.7947 at 0.1, 8.826992 at 0.01 and 8.827318 at 0.001, against 8.827321. The correlation reshapes the price across strikes: with ρ = −0.7 the call at strike 120 is worth 1.477 against 2.460 at ρ = 0 and 3.234 at ρ = 0.7, and the call at strike 80 is worth more (22.83, 22.37, 21.76), which is the skew.

## The integration controls

`HestonIntegrationOptions` has three controls: `integration_steps`, the number of nodes of Simpson's rule; `integration_u_max`, the upper limit of the integral over the frequency u (the integrand decays, so a finite limit suffices); and `mode`, the form of the characteristic function. The defaults, 2048 steps and a limit of 150, are converged for ordinary parameters. The program shows the price as each control is relaxed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const double spot = 100.0, strike = 100.0, rate = 0.03, dividend = 0.01, maturity = 1.0;
  const vol::HestonParams params;
  const double reference = prc::heston_price(prc::Call, spot, strike, rate, dividend, maturity, params);
  out.writeln("default price", reference);

  out.writeln("integration_steps:");
  for (std::int64_t steps : {16, 32, 64, 128, 256, 2048}) {
    prc::HestonIntegrationOptions options;
    options.integration_steps = steps;
    out.writeln("  ", steps, " error", prc::heston_price(prc::Call, spot, strike, rate, dividend, maturity, params, options) - reference);
  }
  out.writeln("integration_u_max:");
  for (double u_max : {5.0, 10.0, 20.0, 50.0, 100.0, 150.0}) {
    prc::HestonIntegrationOptions options;
    options.integration_u_max = u_max;
    out.writeln("  ", u_max, " error", prc::heston_price(prc::Call, spot, strike, rate, dividend, maturity, params, options) - reference);
  }
}
```

Output

```text
default price 8.54225698
integration_steps:
   16  error -0.66042791
   32  error -0.14096568
   64  error -0.00196859
   128  error -0.00000011
   256  error -0.00000000
   2048  error 0.00000000
integration_u_max:
   5.00000000  error -2.55738579
   10.00000000  error -0.49007428
   20.00000000  error 0.04763762
   50.00000000  error 0.00002031
   100.00000000  error 0.00000000
   150.00000000  error 0.00000000
```

Simpson's rule needs about 64 nodes to be accurate to 0.002 and 128 to be accurate to 10⁻⁷; the default of 2048 is far beyond convergence. The frequency limit matters more: a limit of 5 gives an error of −2.56, 20 an error of +0.048, 50 an error of 2 × 10⁻⁵, and 100 and above are converged to eight decimals, so the default of 150 has a wide margin.

## The two forms of the characteristic function

The Heston characteristic function contains a complex square root and logarithm. In the original formulation of Heston (1993) the logarithm is evaluated on its principal branch, which jumps as the frequency or the maturity grows, so the integrand is discontinuous and the price is wrong for long maturities. The reformulation `HestonCfMode::LittleTrap` (Albrecher et al.) is algebraically equal but continuous, and is the default. `HestonCfMode::Original` is kept so that results computed with the original formulation can be reproduced. The program prices a one-year-to-fifteen-year call both ways, and with the COS pricer (which uses the stable form) for reference.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const vol::HestonParams params;
  out.writeln("maturity   Original    LittleTrap   COS (reference)");
  for (double maturity : {0.25, 1.0, 2.0, 5.0, 10.0, 15.0}) {
    prc::HestonIntegrationOptions original;
    original.mode = prc::HestonCfMode::Original;
    prc::HestonIntegrationOptions stable;
    stable.mode = prc::HestonCfMode::LittleTrap;
    out.writeln(maturity, prc::heston_price(prc::Call, 100.0, 100.0, 0.03, 0.01, maturity, params, original), prc::heston_price(prc::Call, 100.0, 100.0, 0.03, 0.01, maturity, params, stable),
                prc::heston_price_cos(prc::Call, 100.0, 100.0, 0.03, 0.01, maturity, params));
  }
}
```

Output

```text
maturity   Original    LittleTrap   COS (reference)
0.250000 4.155291 4.155291 4.155291
1.000000 8.542257 8.542257 8.542257
2.000000 12.427442 12.438526 12.438526
5.000000 17.193110 20.564342 20.564342
10.000000 16.925942 29.317629 29.317629
15.000000 18.202548 35.107281 35.107281
```

Up to one year the two forms agree to six decimals, and at two years they differ in the third decimal (12.4274 against 12.4385). The original form then fails badly: 17.19 against 20.56 at five years, 16.93 against 29.32 at ten years and 18.20 against 35.11 at fifteen, while `LittleTrap` equals the COS price at every maturity.

## Maturity zero and invalid inputs

At zero maturity the price is the intrinsic value. Every invalid input is an error naming the argument; `mu` is not used for pricing, only for simulation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const vol::HestonParams params;
  out.writeln("maturity 0: call at spot 110 and 90 (strike 100):", prc::heston_price(prc::Call, 110.0, 100.0, 0.03, 0.01, 0.0, params), prc::heston_price(prc::Call, 90.0, 100.0, 0.03, 0.01, 0.0, params));
  out.writeln("mu does not enter the price:", prc::heston_price(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, params), prc::heston_price(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, [] { vol::HestonParams p; p.mu = 0.5; return p; }()));
  out.writeln("a variance process that violates the Feller condition is allowed (2 kappa theta =", 2 * 0.2 * 0.02, "< sigma^2 =", 0.8 * 0.8, "):",
              prc::heston_price(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, [] { vol::HestonParams p; p.kappa = 0.2; p.theta = 0.02; p.sigma = 0.8; return p; }()));

  auto attempt = [&](const char *label, auto edit) {
    vol::HestonParams p;
    edit(p);
    try {
      prc::heston_price(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, p);
      out.writeln(label, "-> no error");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      const auto start = message.find("heston_price: ") + 14;
      out.writeln(label, "->", message.substr(start, message.find('\n') - start));
    }
  };
  attempt("kappa = -1  ", [](vol::HestonParams &p) { p.kappa = -1.0; });
  attempt("theta = 0   ", [](vol::HestonParams &p) { p.theta = 0.0; });
  attempt("sigma = 0   ", [](vol::HestonParams &p) { p.sigma = 0.0; });
  attempt("rho = 1     ", [](vol::HestonParams &p) { p.rho = 1.0; });
  attempt("v0 = -0.1   ", [](vol::HestonParams &p) { p.v0 = -0.1; });
  for (std::int64_t steps : {0, 1}) {
    try {
      prc::HestonIntegrationOptions options;
      options.integration_steps = steps;
      prc::heston_price(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, vol::HestonParams{}, options);
    } catch (const std::exception &e) {
      const std::string message = e.what();
      const auto start = message.find("heston_price: ") + 14;
      out.writeln("integration_steps =", steps, "->", message.substr(start, message.find('\n') - start));
    }
  }
  try {
    prc::HestonIntegrationOptions options;
    options.integration_u_max = 0.0;
    prc::heston_price(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, vol::HestonParams{}, options);
  } catch (const std::exception &e) {
    const std::string message = e.what();
    const auto start = message.find("heston_price: ") + 14;
    out.writeln("integration_u_max = 0 ->", message.substr(start, message.find('\n') - start));
  }
}
```

Output

```text
maturity 0: call at spot 110 and 90 (strike 100): 10.0000 0.0000
mu does not enter the price: 8.5423 8.5423
a variance process that violates the Feller condition is allowed (2 kappa theta = 0.0080 < sigma^2 = 0.6400 ): 6.1130
kappa = -1   -> kappa must be finite and > 0
theta = 0    -> theta must be finite and > 0
sigma = 0    -> sigma must be finite and > 0
rho = 1      -> rho must be in (-1,1)
v0 = -0.1    -> v0 must be finite and > 0
integration_steps = 0 -> integration_steps must be >= 2
integration_steps = 1 -> integration_steps must be >= 2
integration_u_max = 0 -> integration_u_max must be finite and > 0
```

At zero maturity the call is worth its intrinsic value (10 at spot 110, 0 at spot 90). Setting `mu` to 0.5 does not change the price, since pricing uses r − q. A variance process with 2κθ = 0.008 far below σ² = 0.64 prices without complaint. Each invalid parameter and each invalid control is rejected with the message shown.
