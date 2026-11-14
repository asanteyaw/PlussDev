<!-- generated from the_fft_pricer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The FFT pricer

[Back to Heston and Bates](../finance_03_heston_and_bates.md)

The Carr-Madan method prices a whole grid of strikes at once. It transforms the price, damped by a factor e^(αk) so that it is integrable in the log strike k, into a function of frequency whose values are the characteristic function divided by a simple polynomial; one fast Fourier transform then returns the damped prices at every strike on a log-spaced grid.

```cpp
module::Engine module::Controls::engine = module::Engine::Analytic;
pricer::Fft module::Controls::fft;
std::int64_t pricer::Fft::num_fft = 16384;
module::Field::Result module::Field::price(Config cfg = {}) const;
```

Choose the engine, set the FFT controls on `field.controls.fft`, and price:

```cpp
field.controls.engine = mod::Engine::Fft;
field.controls.fft.num_fft = 16384;     // and the other controls below
const auto result = field.price();      // result.price: one price per option, read from the grid by a cubic in the log strike
```

With `return_grid` set, `result.price` is instead the whole grid and `result.strike_grid` the strikes it belongs to. The controls are in `field.controls.fft`:

| Control | Meaning | Default |
|---|---|---|
| `num_fft` | the number of grid points *N* | 16384 |
| `du` | the step *Δu* of the frequency grid | 0.1 |
| `dk` | the step *Δk* of the log-strike grid; `NaN` means 2π / (*N* *Δu*), the spacing the transform gives | `NaN` |
| `alpha` | the damping exponent α, lowered automatically if the model has no finite moment of order α + 1 | 1.5 |
| `quadrature` | `Simpson` or `Trapezoidal` weights for the frequency sum | `Simpson` |
| `mode` | the form of the characteristic function ([The Heston model](the_heston_model.md)) | `LittleTrap` |
| `vol_risk_premium` | the market price of variance risk λ, which shifts κ to κ + λ and θ to κθ / (κ + λ) | 0 |
| `return_grid` | return the whole grid instead of one price per option | false |

## The grid

The log strikes are centred on the spot, ln K_j = ln S + (j − *N*/2) Δk, so the strikes are spaced geometrically, the middle one is the spot, and `price` holds the call (or put) price at each. The program prices a strip of strikes from the grid and compares it with the COS engine.

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
  auto grid_field = strip(mod::Engine::Fft, {100.0}, {1.0});
  grid_field.controls.fft.return_grid = true;
  const auto call_grid = grid_field.price();
  const auto strikes = call_grid.strike_grid.items<double>();
  const auto grid_prices = call_grid.price.items<double>();
  const std::size_t middle = strikes.size() / 2;
  out.writeln("grid size", strikes.size(), " first strike", strikes.front(), " middle strike (index N/2)", strikes[middle], " ratio of neighbours", strikes[middle + 1] / strikes[middle]);
  std::size_t within = 0;
  for (double k : strikes) {
    within += (k > 50.0 && k < 200.0) ? 1 : 0;
  }
  out.writeln("strikes between 50 and 200:", within, " of", strikes.size());

  out.writeln("strike   FFT (interpolated)   COS            difference");
  const std::vector<double> test{70.0, 85.0, 100.0, 115.0, 130.0, 160.0};
  const auto fft = prices(strip(mod::Engine::Fft, test, {1.0}));
  const auto cos = prices(strip(mod::Engine::Cos, test, {1.0}));
  for (std::size_t i = 0; i < test.size(); ++i) {
    out.writeln(test[i], fft[i], cos[i], fft[i] - cos[i]);
  }
  auto put_field = strip(mod::Engine::Fft, {100.0}, {1.0}, {1.5, 0.04, 0.3, -0.7, 0.04}, prc::Put);
  put_field.controls.fft.return_grid = true;
  out.writeln("at the middle strike put =", put_field.price().price.items<double>()[middle],
              " call - (S e^-qT - K e^-rT) =", grid_prices[middle] - (kSpot * std::exp(-kDiv) - strikes[middle] * std::exp(-kRate)));
}
```

Output

```text
grid size 16384  first strike 0.000000  middle strike (index N/2) 100.000000  ratio of neighbours 1.003842
strikes between 50 and 200: 361  of 16384
strike   FFT (interpolated)   COS            difference
70.000000 31.633059 31.633059 -0.000000
85.000000 18.749590 18.749590 -0.000000
100.000000 8.542257 8.542257 -0.000000
115.000000 2.518489 2.518489 0.000000
130.000000 0.420271 0.420271 -0.000000
160.000000 0.004565 0.004565 -0.000000
at the middle strike put = 6.581827  call - (S e^-qT - K e^-rT) = 6.581827
```

The grid has 16384 points with neighbouring strikes 0.38% apart, the middle strike is the spot, and 361 of the strikes lie between 50 and 200. The interpolated prices agree with the COS prices to the six decimals shown at all six test strikes, on and off the grid. The put grid at the middle strike is 6.581827, which is the call price less S e^(−qT) − K e^(−rT), as parity requires.

## Accuracy: the frequency range

The transform integrates up to the frequency *N* Δu, and the integrand must have decayed there. The spacing of the log strikes is 2π / (*N* Δu), so a wider frequency range gives a finer strike grid as well. Making Δu small with a fixed *N* shortens the range, truncates the integral and coarsens the strike grid; making it too large samples the integrand too coarsely. The program varies *N* and Δu and prints the frequency range, the strike spacing, the error at the money, and the largest error over ten strikes between 60 and 190, most of them off the grid, against the COS price.

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
  out.print_defaults(PrintOptions{}.sci(2));
  const std::vector<double> strikes{60.0, 72.0, 85.0, 93.0, 100.0, 107.0, 118.0, 131.0, 155.0, 190.0};
  const auto reference = prices(strip(mod::Engine::Cos, strikes, {1.0}));

  out.writeln("   N       du      range N*du   strike spacing (log)   error at the money   largest error over ten strikes");
  for (std::int64_t n : {1024, 4096, 16384, 32768}) {
    for (double du : {0.001, 0.01, 0.05, 0.2}) {
      auto field = strip(mod::Engine::Fft, strikes, {1.0});
      field.controls.fft.num_fft = n;
      field.controls.fft.du = du;
      const double spacing = 2.0 * 3.14159265358979323846 / (n * du);
      try {
        const auto fft = prices(field);
        double worst = 0.0;
        for (std::size_t i = 0; i < strikes.size(); ++i) {
          worst = std::max(worst, std::abs(fft[i] - reference[i]));
        }
        out.writeln(n, du, n * du, spacing, std::abs(fft[4] - reference[4]), worst);
      } catch (const std::exception &) {
        out.writeln(n, du, n * du, spacing, "refused: the grid overflows");
      }
    }
  }
}
```

Output

```text
   N       du      range N*du   strike spacing (log)   error at the money   largest error over ten strikes
1024 1.00e-03 1.02e+00 6.14e+00 refused: the grid overflows
1024 1.00e-02 1.02e+01 6.14e-01 1.46e-01 2.28e+00
1024 5.00e-02 5.12e+01 1.23e-01 1.13e-06 4.67e-02
1024 2.00e-01 2.05e+02 3.07e-02 1.93e-09 2.75e-04
4096 1.00e-03 4.10e+00 1.53e+00 refused: the grid overflows
4096 1.00e-02 4.10e+01 1.53e-01 1.81e-05 9.12e-02
4096 5.00e-02 2.05e+02 3.07e-02 1.28e-13 2.75e-04
4096 2.00e-01 8.19e+02 7.67e-03 1.93e-09 1.13e-06
16384 1.00e-03 1.64e+01 3.83e-01 3.46e-01 1.15e+00
16384 1.00e-02 1.64e+02 3.83e-02 1.07e-12 5.86e-04
16384 5.00e-02 8.19e+02 7.67e-03 1.28e-13 1.13e-06
16384 2.00e-01 3.28e+03 1.92e-03 1.93e-09 2.69e-09
32768 1.00e-03 3.28e+01 1.92e-01 1.73e-04 2.08e-01
32768 1.00e-02 3.28e+02 1.92e-02 1.07e-12 4.18e-05
32768 5.00e-02 1.64e+03 3.83e-03 1.28e-13 3.68e-08
32768 2.00e-01 6.55e+03 9.59e-04 1.93e-09 2.00e-09
```

The range *N* Δu must be large enough for the integrand to have decayed. With a range of 10 or less the integral is cut off and the price is wrong (error 0.15 at the money for *N* = 1024, Δu = 0.01; 0.35 for *N* = 16384, Δu = 0.001). When the log-strike grid is so coarse that its extreme strikes overflow (*N* = 1024 or 4096 with Δu = 0.001) the engine refuses rather than return a price, since a non-finite entry means the transform has failed. From a range of about 200 on, the error at the money is 10⁻⁹ or better, and the largest error over ten strikes, which is dominated by interpolation between grid points, falls with the strike spacing: 2.8 × 10⁻⁴ at a spacing of 3.1%, 1.1 × 10⁻⁶ at 0.77% and 2.7 × 10⁻⁹ at 0.19%. More points give a finer grid for the same Δu. The default, *N* = 16384 and Δu = 0.1, has a range of 1638 and a spacing of 0.38%, and holds the error over strikes from 60 to 190 below about 10⁻⁵ for maturities from a week to ten years.

## Models without a finite moment

The damped transform exists only if the model has a finite moment E[S_T^(α + 1)]. The Heston model loses that moment when the volatility of variance is large and the maturity long: for the parameters below the moment of order 2.5 is infinite at ten years. The characteristic function then continues analytically past the singularity to numbers that are not a moment, and a transform built on them gives a wrong price. The FFT engine tests for this: it checks that the moment is real, finite, at least the Jensen bound F^(α + 1) and no more than twice it (a moment far above the bound means the model is close to the explosion and the transform has lost its accuracy), and halves the damping exponent until it is. If no exponent works it refuses with a message that points to the COS engine, which has no such condition. The program prices a model with a large volatility of variance at three maturities with the FFT and COS engines.

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
  // 2 kappa theta = 0.096 against sigma^2 = 0.81: moments of the price explode at long maturities
  const std::vector<double> params{0.8, 0.06, 0.9, -0.3, 0.09};
  out.writeln("maturity  strike   FFT          COS");
  for (double maturity : {1.0, 5.0, 10.0}) {
    const std::vector<double> strikes{80.0, 100.0, 130.0};
    const auto fft = prices(strip(mod::Engine::Fft, strikes, {maturity}, params));
    const auto cos = prices(strip(mod::Engine::Cos, strikes, {maturity}, params));
    for (std::size_t i = 0; i < strikes.size(); ++i) {
      out.writeln(maturity, strikes[i], fft[i], cos[i]);
    }
  }
}
```

Output

```text
maturity  strike   FFT          COS
1.000000 80.000000 24.223549 24.223548
1.000000 100.000000 10.076517 10.076516
1.000000 130.000000 2.175132 2.175131
5.000000 80.000000 33.078415 33.078378
5.000000 100.000000 21.769960 21.769921
5.000000 130.000000 10.819907 10.819865
10.000000 80.000000 40.026473 40.026464
10.000000 100.000000 30.947474 30.947465
10.000000 130.000000 20.632598 20.632588
```

The two engines agree to the printed six decimals at all three maturities (24.223549 at the strike 80 for one year, 33.078415 for five, 40.026473 for ten). At ten years the damping exponent has been halved to keep the transform inside the region where the moment exists. The refusal is reserved for models where no damping exponent gives a genuine moment.

The moment can be lost for any characteristic function you supply, not only Heston's; the same check applies to the generic FFT pricer ([Generic characteristic functions](generic_characteristic_functions.md)).

## The other controls

`alpha` has almost no effect over a wide range; `quadrature` changes the sum's weights; `vol_risk_premium` changes the model. A `dk` set explicitly decouples the strike spacing from the frequency grid (the transform is then evaluated by a fractional FFT). The program shows each, and the settings that are rejected: a `num_fft` below 2, and a non-positive step or damping.

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
  out.print_defaults(PrintOptions{}.fixed(8));
  const double reference = price1(strip(mod::Engine::Cos, {100.0}, {1.0}));
  out.writeln("COS price", reference);
  for (double alpha : {0.5, 1.0, 1.5, 3.0, 5.0}) {
    auto field = strip(mod::Engine::Fft, {100.0}, {1.0});
    field.controls.fft.alpha = alpha;
    out.writeln("alpha", alpha, " error", price1(field) - reference);
  }
  for (auto quadrature : {prc::HestonFftQuadrature::Simpson, prc::HestonFftQuadrature::Trapezoidal}) {
    auto field = strip(mod::Engine::Fft, {100.0}, {1.0});
    field.controls.fft.quadrature = quadrature;
    out.writeln(quadrature == prc::HestonFftQuadrature::Simpson ? "Simpson    " : "Trapezoidal", " error", price1(field) - reference);
  }
  {
    auto field = strip(mod::Engine::Fft, {100.0}, {1.0});
    field.controls.fft.dk = 0.002;
    field.controls.fft.return_grid = true;
    const auto k = field.price().strike_grid.items<double>();
    field.controls.fft.return_grid = false;
    out.writeln("dk 0.002: strike ratio of neighbours", k[k.size() / 2 + 1] / k[k.size() / 2], " price at the money error", price1(field) - reference);
  }
  {
    auto field = strip(mod::Engine::Fft, {100.0}, {1.0});
    field.controls.fft.vol_risk_premium = 0.5;
    out.writeln("vol_risk_premium 0.5: price", price1(field), "(a different model; 0 gives", price1(strip(mod::Engine::Fft, {100.0}, {1.0})), ")");
  }

  auto attempt = [&](const char *label, auto edit) {
    auto field = strip(mod::Engine::Fft, {100.0}, {1.0});
    edit(field.controls.fft);
  try {
    (void)field.price();
    out.writeln(label, "-> no error");
  } catch (const std::exception &e) {
    const std::string message = e.what();
    out.writeln(label, "->", message.substr(0, message.find('\n')));
  }
  };
  attempt("num_fft = 1   ", [](auto &o) { o.num_fft = 1; });
  attempt("du = 0        ", [](auto &o) { o.du = 0.0; });
  attempt("alpha = 0     ", [](auto &o) { o.alpha = 0.0; });
  out.writeln("maturity 0 gives the intrinsic value:", price1(strip(mod::Engine::Fft, {90.0}, {0.0})));
}
```

Output

```text
COS price 8.54225699
alpha 0.50000000  error -0.00000497
alpha 1.00000000  error -0.00000000
alpha 1.50000000  error -0.00000000
alpha 3.00000000  error -0.00000000
alpha 5.00000000  error -0.00000000
Simpson      error -0.00000000
Trapezoidal  error -0.00000000
dk 0.002: strike ratio of neighbours 1.00200200  price at the money error -0.00000000
vol_risk_premium 0.5: price 8.02014242 (a different model; 0 gives 8.54225699 )
num_fft = 1    -> subsp::math::finance::module::Field: controls.fft.num_fft must be >= 2
du = 0         -> subsp::math::finance::module::Field: controls.fft.du must be finite and > 0
alpha = 0      -> subsp::math::finance::module::Field: controls.fft.alpha must be finite and > 0
maturity 0 gives the intrinsic value: 10.00000000
```

The price is insensitive to `alpha` from 1 upward (the error is below 10⁻⁸), and slightly worse at 0.5 (5 × 10⁻⁶). Simpson and trapezoidal weights agree to 10⁻⁸. A `dk` of 0.002 gives a grid 0.2% apart. A variance risk premium of 0.5 changes the model and the price, to 8.0201 from 8.5423. The invalid settings are rejected, each with a message that names the control, and a zero maturity returns the intrinsic value.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="the_cos_pricer.html">
    <span class="label">Previous</span>
    <span class="title">The COS pricer</span>
  </a>
  <a class="next-link" href="the_heston_model.html">
    <span class="label">Next</span>
    <span class="title">The Heston model</span>
  </a>
</div>
<!-- apiref:nav:end -->
