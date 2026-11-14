<!-- generated from jumps_the_bates_model.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Jumps: the Bates model

[Back to Heston and Bates](../finance_03_heston_and_bates.md)

The Heston model produces a skew but, over short maturities, too little of it: a diffusion cannot make large moves quickly. Bates (1996) adds jumps to the asset price. Jumps arrive at the rate λ, and each multiplies the price by e^J with J normally distributed with mean m and standard deviation δ:

dS / S = (r − q − λk) dt + √v dW₁ + (e^J − 1) dN, with k = e^(m + δ²/2) − 1,

where the variance follows the Heston dynamics. The drift is compensated by λk so that the discounted price is still a martingale.

```cpp
instrument::Vanilla instrument::make(pricing::OptionType type, double strike, double maturity, Config cfg = f64_default());
market::State market::make(double spot, double rate, double dividend_yield, Config cfg = f64_default());
module::Model model::make(model::Kind kind, std::initializer_list<double> params, Config cfg = f64_default());
module::Field::Result module::Field::price(Config cfg = {}) const;
module::Engine module::Controls::engine = module::Engine::Analytic;
```

Bates takes the five Heston parameters followed by `jump_intensity, jump_mean, jump_vol`:

```cpp
mod::Field field{.instrument = fin::instrument::make(prc::Call, strike, maturity),
                 .market = fin::market::make(spot, rate, dividend_yield),
                 .model = fin::model::make(fin::model::Kind::Bates, {kappa, theta, sigma, rho, v0, jump_intensity, jump_mean, jump_vol}),
                 .controls = {.engine = mod::Engine::Cos}};
```

The five Heston parameters have the same meaning and constraints, `jump_intensity` (λ) and `jump_vol` (δ) must be non-negative, and `jump_mean` (m) finite. The Fourier engines `Cos` and `Fft` price a Bates field with the controls of the previous pages, and `MonteCarlo` prices it by simulation ([Finance 4](../finance_04_monte_carlo.md)). The default jump parameters of the programs below are λ = 0.15, m = −0.08, δ = 0.20.

## What the jump parameters do

With no jumps (`jump_intensity = 0`) the price is the Heston price. More frequent jumps, larger jumps and more volatile jumps all raise the price of an at-the-money option, by adding variance. A negative mean jump makes the left tail heavier, so it raises the price of low strikes (puts) more than high ones. The program varies each parameter, and prices with both the COS and FFT engines, which agree.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace prc = fin::pricing;
using namespace pluss::tils;
using pluss::Series;

constexpr double kSpot = 100.0, kRate = 0.03, kDiv = 0.01;

// A strip of options on one underlying priced under Heston (or Bates, with eight parameters) by the chosen engine.
mod::Field strip(mod::Engine engine, const std::vector<double> &strikes, const std::vector<double> &maturities, std::vector<double> params = {1.5, 0.04, 0.3, -0.7, 0.04},
                 prc::OptionType type = prc::Call, fin::model::Kind kind = fin::model::Kind::Heston) {
  return mod::Field{.instrument = {.type = type, .strike = pluss::make_series(strikes), .maturity = pluss::make_series(maturities)},
                    .market = fin::market::make(kSpot, kRate, kDiv),
                    .model = fin::model::make(kind, params),
                    .controls = {.engine = engine, .risk = mod::Risk::PriceOnly}};
}
std::vector<double> prices(const mod::Field &f) { return f.price().price.items<double>(); }
double price1(const mod::Field &f) { return prices(f)[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const std::vector<double> base{1.5, 0.04, 0.3, -0.7, 0.04, 0.15, -0.08, 0.2};
  auto bates = [&](mod::Engine engine, std::vector<double> p, double strike = 100.0) {
    return price1(strip(engine, {strike}, {1.0}, p, prc::Call, fin::model::Kind::Bates));
  };
  auto with = [&](std::size_t index, double value) {
    auto p = base;
    p[index] = value;
    return p;
  };

  out.writeln("Heston (no jumps)", price1(strip(mod::Engine::Cos, {100.0}, {1.0})));
  out.writeln("default Bates: COS", bates(mod::Engine::Cos, base), " FFT", bates(mod::Engine::Fft, base));
  out.writeln("jump_intensity (lambda):");
  for (double lambda : {0.0, 0.05, 0.15, 0.5, 1.0}) {
    out.writeln("  ", lambda, " COS", bates(mod::Engine::Cos, with(5, lambda)), " FFT", bates(mod::Engine::Fft, with(5, lambda)));
  }
  out.writeln("jump_mean (m):");
  for (double m : {-0.3, -0.08, 0.0, 0.1}) {
    out.writeln("  ", m, " COS", bates(mod::Engine::Cos, with(6, m)));
  }
  out.writeln("jump_vol (delta):");
  for (double d : {0.0, 0.1, 0.2, 0.4}) {
    out.writeln("  ", d, " COS", bates(mod::Engine::Cos, with(7, d)));
  }
  out.writeln("calls at strikes 80 and 120 with no jumps / default jumps:");
  out.writeln("  ", bates(mod::Engine::Cos, with(5, 0.0), 80.0), bates(mod::Engine::Cos, base, 80.0), " |", bates(mod::Engine::Cos, with(5, 0.0), 120.0), bates(mod::Engine::Cos, base, 120.0));
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
   0.400000  COS 9.956164
calls at strikes 80 and 120 with no jumps / default jumps:
   22.831101 23.110574  | 1.476924 1.913956
```

With no jumps the Bates price is the Heston price, 8.5423. COS and FFT agree to six decimals everywhere. The price rises with the jump intensity, from 8.54 through 9.10 at the default 0.15 to 11.81 at an intensity of 1, and with the jump volatility (8.64 with jumps of fixed size, 9.96 at a volatility of 0.4). The mean jump size has a smaller effect and is U-shaped, with a minimum near zero (9.85 at −0.3, 9.02 at 0 and 9.12 at 0.1), because the compensator removes the effect of the mean on the forward. The jumps raise both wings: the call at strike 80 goes from 22.83 to 23.11 and the call at strike 120 from 1.477 to 1.914, a rise of 30%.

## Parity and the martingale property

Put-call parity holds for the Bates prices, which is the statement that the compensated jump process leaves the forward price unchanged: C − P = S e^(−qT) − K e^(−rT) however large the jump parameters. The program checks it for several settings, with both engines.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace prc = fin::pricing;
using namespace pluss::tils;
using pluss::Series;

constexpr double kSpot = 100.0, kRate = 0.03, kDiv = 0.01;

// A strip of options on one underlying priced under Heston (or Bates, with eight parameters) by the chosen engine.
mod::Field strip(mod::Engine engine, const std::vector<double> &strikes, const std::vector<double> &maturities, std::vector<double> params = {1.5, 0.04, 0.3, -0.7, 0.04},
                 prc::OptionType type = prc::Call, fin::model::Kind kind = fin::model::Kind::Heston) {
  return mod::Field{.instrument = {.type = type, .strike = pluss::make_series(strikes), .maturity = pluss::make_series(maturities)},
                    .market = fin::market::make(kSpot, kRate, kDiv),
                    .model = fin::model::make(kind, params),
                    .controls = {.engine = engine, .risk = mod::Risk::PriceOnly}};
}
std::vector<double> prices(const mod::Field &f) { return f.price().price.items<double>(); }
double price1(const mod::Field &f) { return prices(f)[0]; }

int main() {
  OutputEngine out;
  const double strike = 105.0, maturity = 1.5;
  const double forward = kSpot * std::exp(-kDiv * maturity) - strike * std::exp(-kRate * maturity);
  out.print_defaults(PrintOptions{}.sci(2));
  out.writeln("lambda   m      delta   COS: C - P - forward    FFT: C - P - forward");
  for (double lambda : {0.1, 0.5, 2.0}) {
    for (double m : {-0.2, 0.1}) {
      for (double delta : {0.1, 0.3}) {
        const std::vector<double> p{1.5, 0.04, 0.3, -0.7, 0.04, lambda, m, delta};
        auto parity = [&](mod::Engine engine) {
          return price1(strip(engine, {strike}, {maturity}, p, prc::Call, fin::model::Kind::Bates)) - price1(strip(engine, {strike}, {maturity}, p, prc::Put, fin::model::Kind::Bates)) - forward;
        };
        out.writeln(lambda, m, delta, parity(mod::Engine::Cos), parity(mod::Engine::Fft));
      }
    }
  }
}
```

Output

```text
lambda   m      delta   COS: C - P - forward    FFT: C - P - forward
1.00e-01 -2.00e-01 1.00e-01 2.18e-14 5.77e-15
1.00e-01 -2.00e-01 3.00e-01 2.18e-14 4.44e-16
1.00e-01 1.00e-01 1.00e-01 2.18e-14 -3.11e-15
1.00e-01 1.00e-01 3.00e-01 2.18e-14 7.55e-15
5.00e-01 -2.00e-01 1.00e-01 2.18e-14 9.33e-15
5.00e-01 -2.00e-01 3.00e-01 2.18e-14 7.55e-15
5.00e-01 1.00e-01 1.00e-01 2.18e-14 4.00e-15
5.00e-01 1.00e-01 3.00e-01 2.18e-14 9.33e-15
2.00e+00 -2.00e-01 1.00e-01 2.18e-14 4.00e-15
2.00e+00 -2.00e-01 3.00e-01 2.18e-14 4.44e-16
2.00e+00 1.00e-01 1.00e-01 2.18e-14 7.55e-15
2.00e+00 1.00e-01 3.00e-01 2.18e-14 4.00e-15
```

Parity holds to rounding (10⁻¹⁴) for every combination of jump parameters, with both engines, which is the statement that the compensated jumps leave the forward price unchanged.

## Invalid parameters

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <limits>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace prc = fin::pricing;
using namespace pluss::tils;
using pluss::Series;

constexpr double kSpot = 100.0, kRate = 0.03, kDiv = 0.01;

// A strip of options on one underlying priced under Heston (or Bates, with eight parameters) by the chosen engine.
mod::Field strip(mod::Engine engine, const std::vector<double> &strikes, const std::vector<double> &maturities, std::vector<double> params = {1.5, 0.04, 0.3, -0.7, 0.04},
                 prc::OptionType type = prc::Call, fin::model::Kind kind = fin::model::Kind::Heston) {
  return mod::Field{.instrument = {.type = type, .strike = pluss::make_series(strikes), .maturity = pluss::make_series(maturities)},
                    .market = fin::market::make(kSpot, kRate, kDiv),
                    .model = fin::model::make(kind, params),
                    .controls = {.engine = engine, .risk = mod::Risk::PriceOnly}};
}
std::vector<double> prices(const mod::Field &f) { return f.price().price.items<double>(); }
double price1(const mod::Field &f) { return prices(f)[0]; }

int main() {
  OutputEngine out;
  const double nan = std::numeric_limits<double>::quiet_NaN();
  auto attempt = [&](const char *label, std::vector<double> p) {
  try {
    (void)strip(mod::Engine::Cos, {100.0}, {1.0}, p, prc::Call, fin::model::Kind::Bates).price();
    out.writeln(label, "-> no error");
  } catch (const std::exception &e) {
    const std::string message = e.what();
    out.writeln(label, "->", message.substr(0, message.find('\n')));
  }
  };
  attempt("jump_intensity = -1  ", {1.5, 0.04, 0.3, -0.7, 0.04, -1.0, -0.08, 0.2});
  attempt("jump_vol = -0.1      ", {1.5, 0.04, 0.3, -0.7, 0.04, 0.15, -0.08, -0.1});
  attempt("jump_mean = NaN      ", {1.5, 0.04, 0.3, -0.7, 0.04, 0.15, nan, 0.2});
  attempt("kappa = 0            ", {0.0, 0.04, 0.3, -0.7, 0.04, 0.15, -0.08, 0.2});
  attempt("rho = -1             ", {1.5, 0.04, 0.3, -1.0, 0.04, 0.15, -0.08, 0.2});
}
```

Output

```text
jump_intensity = -1   -> subsp::math::finance::module::Model: model.Bates.jump_intensity must be >= 0
jump_vol = -0.1       -> subsp::math::finance::module::Model: model.Bates.jump_vol must be >= 0
jump_mean = NaN       -> subsp::math::finance::module::Model: model.Bates.jump_mean must be finite
kappa = 0             -> subsp::math::finance::module::Model: model.Bates.kappa must be > 0
rho = -1              -> subsp::math::finance::module::Model: model.Bates.rho must be in (-1, 1)
```

Each invalid parameter is rejected with a message that names it.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="generic_characteristic_functions.html">
    <span class="label">Previous</span>
    <span class="title">Generic characteristic functions</span>
  </a>
  <a class="next-link" href="smiles_from_heston.html">
    <span class="label">Next</span>
    <span class="title">Smiles from Heston</span>
  </a>
</div>
<!-- apiref:nav:end -->
