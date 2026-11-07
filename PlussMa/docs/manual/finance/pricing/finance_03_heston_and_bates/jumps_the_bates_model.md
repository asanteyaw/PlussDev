<!-- generated from jumps_the_bates_model.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Jumps: the Bates model

[Back to Heston and Bates](../finance_03_heston_and_bates.md)

The Heston model produces a skew but, over short maturities, too little of it: a diffusion cannot make large moves quickly. Bates (1996) adds jumps to the asset price. Jumps arrive at the rate λ, and each multiplies the price by e^J with J normally distributed with mean m and standard deviation δ:

dS / S = (r − q − λk) dt + √v dW₁ + (e^J − 1) dN, with k = e^(m + δ²/2) − 1,

where the variance follows the Heston dynamics. The drift is compensated by λk so that the discounted price is still a martingale.

```cpp
struct vol::BatesParams {
  double kappa = 1.5, theta = 0.04, sigma = 0.3, rho = -0.7, v0 = 0.04, mu = 0.0, variance_floor = 1e-12;   // as for Heston
  double jump_intensity = 0.15;   // lambda
  double jump_mean = -0.08;       // m
  double jump_vol = 0.20;         // delta
};
double prc::bates_price_fft(type, spot, strike, rate, dividend_yield, maturity, const vol::BatesParams &, const prc::HestonFftOptions & = {}, Config = {});
double prc::bates_price_cos(type, spot, strike, rate, dividend_yield, maturity, const vol::BatesParams &, const prc::CosOptions & = {}, Config = {});
double prc::bates_price_mc(type, ..., const vol::BatesParams &, const prc::MonteCarloOptions & = {}, Config = {});     // Monte Carlo chapter
prc::HestonFftResult prc::bates_price_fft_grid(type, spot, rate, dividend_yield, maturity, const vol::BatesParams &, ...);
```

The Heston parameters have the same meaning and constraints, `jump_intensity` and `jump_vol` must be non-negative, and `jump_mean` finite. The controls are those of the pricers of the same name for Heston.

## What the jump parameters do

With no jumps (`jump_intensity = 0`) the price is the Heston price. More frequent jumps, larger jumps and more volatile jumps all raise the price of an at-the-money option, by adding variance. A negative mean jump makes the left tail heavier, so it raises the price of low strikes (puts) more than high ones. The program varies each parameter, and prices with both the COS and FFT pricers, which agree.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double spot = 100.0, rate = 0.03, dividend = 0.01, maturity = 1.0;
  const vol::BatesParams base;
  auto cos = [&](const vol::BatesParams &p, double strike = 100.0) { return prc::bates_price_cos(prc::Call, spot, strike, rate, dividend, maturity, p); };
  auto fft = [&](const vol::BatesParams &p) { return prc::bates_price_fft(prc::Call, spot, 100.0, rate, dividend, maturity, p); };

  vol::HestonParams heston;
  heston.kappa = base.kappa; heston.theta = base.theta; heston.sigma = base.sigma; heston.rho = base.rho; heston.v0 = base.v0;
  out.writeln("Heston (no jumps)", prc::heston_price_cos(prc::Call, spot, 100.0, rate, dividend, maturity, heston));
  out.writeln("default Bates: COS", cos(base), " FFT", fft(base));
  out.writeln("jump_intensity (lambda):");
  for (double lambda : {0.0, 0.05, 0.15, 0.5, 1.0}) {
    auto p = base;
    p.jump_intensity = lambda;
    out.writeln("  ", lambda, " COS", cos(p), " FFT", fft(p));
  }
  out.writeln("jump_mean (m):");
  for (double m : {-0.3, -0.08, 0.0, 0.1}) {
    auto p = base;
    p.jump_mean = m;
    out.writeln("  ", m, " COS", cos(p));
  }
  out.writeln("jump_vol (delta):");
  for (double d : {0.0, 0.1, 0.2, 0.4}) {
    auto p = base;
    p.jump_vol = d;
    out.writeln("  ", d, " COS", cos(p));
  }
  out.writeln("calls at strikes 80 and 120 with no jumps / default jumps:");
  auto no_jumps = base;
  no_jumps.jump_intensity = 0.0;
  out.writeln("  ", cos(no_jumps, 80.0), cos(base, 80.0), " |", cos(no_jumps, 120.0), cos(base, 120.0));
}
```

Output

```text
Heston (no jumps) 8.542257
default Bates: COS 9.095602  FFT 9.095602
jump_intensity (lambda):
   0.000000  COS 8.542257  FFT 8.542257
   0.050000  COS 8.729699  FFT 8.729699
   0.150000  COS 9.095602  FFT 9.095602
   0.500000  COS 10.291034  FFT 10.291034
   1.000000  COS 11.808685  FFT 11.808685
jump_mean (m):
   -0.300000  COS 9.852320
   -0.080000  COS 9.095602
   0.000000  COS 9.021930
   0.100000  COS 9.117524
jump_vol (delta):
   0.000000  COS 8.637799
   0.100000  COS 8.775644
   0.200000  COS 9.095602
   0.400000  COS 9.956165
calls at strikes 80 and 120 with no jumps / default jumps:
   22.831101 23.110574  | 1.476924 1.913956
```

With no jumps the Bates price is the Heston price, 8.5423. COS and FFT agree to six decimals everywhere. The price rises with the jump intensity, from 8.54 through 9.10 at the default 0.15 to 11.81 at an intensity of 1, and with the jump volatility (8.64 with jumps of fixed size, 9.96 at a volatility of 0.4). The mean jump size has a smaller effect and is U-shaped, with a minimum near zero (9.85 at −0.3, 9.02 at 0 and 9.12 at 0.1), because the compensator removes the effect of the mean on the forward. The jumps raise both wings: the call at strike 80 goes from 22.83 to 23.11 and the call at strike 120 from 1.477 to 1.914, a rise of 30%.

## Parity and the martingale property

Put-call parity holds for the Bates prices, which is the statement that the compensated jump process leaves the forward price unchanged: C − P = S e^(−qT) − K e^(−rT) however large the jump parameters. The program checks it for several settings, with both pricers.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  const double spot = 100.0, strike = 105.0, rate = 0.03, dividend = 0.01, maturity = 1.5;
  const double forward = spot * std::exp(-dividend * maturity) - strike * std::exp(-rate * maturity);
  out.writeln("lambda   m      delta   COS: C - P - forward    FFT: C - P - forward");
  for (double lambda : {0.1, 0.5, 2.0}) {
    for (double m : {-0.2, 0.1}) {
      for (double delta : {0.1, 0.3}) {
        vol::BatesParams p;
        p.jump_intensity = lambda;
        p.jump_mean = m;
        p.jump_vol = delta;
        const double cos = prc::bates_price_cos(prc::Call, spot, strike, rate, dividend, maturity, p) - prc::bates_price_cos(prc::Put, spot, strike, rate, dividend, maturity, p);
        const double fft = prc::bates_price_fft(prc::Call, spot, strike, rate, dividend, maturity, p) - prc::bates_price_fft(prc::Put, spot, strike, rate, dividend, maturity, p);
        out.writeln(lambda, m, delta, cos - forward, fft - forward);
      }
    }
  }
}
```

Output

```text
lambda   m      delta   COS: C - P - forward    FFT: C - P - forward
1.00e-01 -2.00e-01 1.00e-01 4.44e-16 9.33e-15
1.00e-01 -2.00e-01 3.00e-01 7.55e-15 4.44e-16
1.00e-01 1.00e-01 1.00e-01 7.55e-15 -1.33e-15
1.00e-01 1.00e-01 3.00e-01 9.33e-15 9.33e-15
5.00e-01 -2.00e-01 1.00e-01 9.33e-15 9.33e-15
5.00e-01 -2.00e-01 3.00e-01 2.22e-15 9.33e-15
5.00e-01 1.00e-01 1.00e-01 7.55e-15 7.55e-15
5.00e-01 1.00e-01 3.00e-01 5.77e-15 7.55e-15
2.00e+00 -2.00e-01 1.00e-01 4.00e-15 4.00e-15
2.00e+00 -2.00e-01 3.00e-01 4.00e-15 7.55e-15
2.00e+00 1.00e-01 1.00e-01 7.55e-15 7.55e-15
2.00e+00 1.00e-01 3.00e-01 4.44e-16 4.00e-15
```

Parity holds to rounding (10⁻¹⁴) for every combination of jump parameters, with both pricers, which is the statement that the compensated jumps leave the forward price unchanged.

## Invalid parameters

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  auto attempt = [&](const char *label, auto edit) {
    vol::BatesParams p;
    edit(p);
    try {
      prc::bates_price_cos(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, p);
      out.writeln(label, "-> no error");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      const auto start = message.find("bates_price_cos: ") + 17;
      out.writeln(label, "->", message.substr(start, message.find('\n') - start));
    }
  };
  attempt("jump_intensity = -1  ", [](vol::BatesParams &p) { p.jump_intensity = -1.0; });
  attempt("jump_vol = -0.1      ", [](vol::BatesParams &p) { p.jump_vol = -0.1; });
  attempt("jump_mean = NaN      ", [](vol::BatesParams &p) { p.jump_mean = std::numeric_limits<double>::quiet_NaN(); });
  attempt("kappa = 0            ", [](vol::BatesParams &p) { p.kappa = 0.0; });
  attempt("rho = -1             ", [](vol::BatesParams &p) { p.rho = -1.0; });
}
```

Output

```text
jump_intensity = -1   -> jump_intensity must be finite and >= 0
jump_vol = -0.1       -> jump_vol must be finite and >= 0
jump_mean = NaN       -> jump_mean must be finite
kappa = 0             -> kappa must be finite and > 0
rho = -1              -> rho must be in (-1,1)
```

Each invalid parameter is rejected with its message.
