<!-- generated from the_fft_pricer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The FFT pricer

[Back to Heston and Bates](../finance_03_heston_and_bates.md)

The Carr-Madan method prices a whole grid of strikes at once. It transforms the price, damped by a factor e^(αk) so that it is integrable in the log strike k, into a function of frequency whose values are the characteristic function divided by a simple polynomial; one fast Fourier transform then returns the damped prices at every strike on a log-spaced grid.

```cpp
double prc::heston_price_fft(type, spot, strike, rate, dividend_yield, maturity, const vol::HestonParams &, const prc::HestonFftOptions & = {}, Config = {});
prc::HestonFftResult prc::heston_price_fft_grid(type, spot, rate, dividend_yield, maturity, const vol::HestonParams &, const prc::HestonFftOptions & = {}, Config = {});
struct prc::HestonFftResult { Series price; Series strike; };      // the whole grid, both of length num_fft
```

`heston_price_fft_grid` returns the grid. `heston_price_fft` evaluates it and interpolates to the requested strike with a cubic in the log strike, whose error falls as the fourth power of the grid spacing.

| `HestonFftOptions` | Meaning | Default |
|---|---|---|
| `num_fft` | the number of grid points *N* | 16384 |
| `characteristic_fcn_step` | the step *Δu* of the frequency grid | 0.1 |
| `log_strike_step` | the step *Δk* of the log-strike grid; `NaN` means 2π / (*N* *Δu*), the spacing the transform gives | `NaN` |
| `damping_factor` | the damping exponent α, lowered automatically if the model has no finite moment of order α + 1 | 1.5 |
| `quadrature` | `Simpson` or `Trapezoidal` weights for the frequency sum | `Simpson` |
| `mode` | the form of the characteristic function ([The Heston model](the_heston_model.md)) | `LittleTrap` |
| `vol_risk_premium` | the market price of variance risk λ, which shifts κ to κ + λ and θ to κθ / (κ + λ) | 0 |

## The grid

The log strikes are centred on the spot, ln K_j = ln S + (j − *N*/2) Δk, so the strikes are spaced geometrically, the middle one is the spot, and `price` holds the call (or put) price at each. The program prices a strip of strikes from the grid and compares it with the integration pricer.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double spot = 100.0, rate = 0.03, dividend = 0.01, maturity = 1.0;
  const vol::HestonParams params;

  const auto grid = prc::heston_price_fft_grid(prc::Call, spot, rate, dividend, maturity, params);
  const auto strikes = grid.strike.pull<double>();
  const auto prices = grid.price.pull<double>();
  const std::size_t middle = strikes.size() / 2;
  out.writeln("grid size", strikes.size(), " first strike", strikes.front(), " middle strike (index N/2)", strikes[middle], " ratio of neighbours", strikes[middle + 1] / strikes[middle]);
  std::size_t within = 0;
  for (double k : strikes) {
    within += (k > 50.0 && k < 200.0) ? 1 : 0;
  }
  out.writeln("strikes between 50 and 200:", within, " of", strikes.size());

  out.writeln("strike   FFT (interpolated)   integration    difference");
  for (double strike : {70.0, 85.0, 100.0, 115.0, 130.0, 160.0}) {
    const double fft = prc::heston_price_fft(prc::Call, spot, strike, rate, dividend, maturity, params);
    const double exact = prc::heston_price(prc::Call, spot, strike, rate, dividend, maturity, params);
    out.writeln(strike, fft, exact, fft - exact);
  }
  out.writeln("the put grid is the call grid plus K e^-rT - S e^-qT; at the middle strike put =", prc::heston_price_fft_grid(prc::Put, spot, rate, dividend, maturity, params).price.pull<double>()[middle],
              " call - (S e^-qT - K e^-rT) =", prices[middle] - (spot * std::exp(-dividend * maturity) - strikes[middle] * std::exp(-rate * maturity)));
}
```

Output

```text
grid size 16384  first strike 0.000000  middle strike (index N/2) 100.000000  ratio of neighbours 1.003842
strikes between 50 and 200: 361  of 16384
strike   FFT (interpolated)   integration    difference
70.000000 31.633059 31.633059 0.000000
85.000000 18.749590 18.749590 0.000000
100.000000 8.542257 8.542257 0.000000
115.000000 2.518489 2.518489 0.000000
130.000000 0.420271 0.420271 0.000000
160.000000 0.004565 0.004565 0.000000
the put grid is the call grid plus K e^-rT - S e^-qT; at the middle strike put = 6.581827  call - (S e^-qT - K e^-rT) = 6.581827
```

The grid has 16384 points with neighbouring strikes 0.38% apart, the middle strike is the spot, and 361 of the strikes lie between 50 and 200. The interpolated prices agree with the integration pricer to the six decimals shown at all six test strikes, on and off the grid (the actual differences are of order 10⁻⁷). The put grid at the middle strike is 6.581827, which is the call price less S e^(−qT) − K e^(−rT), as parity requires.

## Accuracy: the frequency range

The transform integrates up to the frequency *N* Δu, and the integrand must have decayed there. The spacing of the log strikes is 2π / (*N* Δu), so a wider frequency range gives a finer strike grid as well. Making Δu small with a fixed *N* shortens the range, truncates the integral and coarsens the strike grid; making it too large samples the integrand too coarsely. The program varies *N* and Δu and prints the frequency range, the strike spacing, the error at the money, and the largest error over ten strikes between 60 and 190, most of them off the grid, against the COS price.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  const double spot = 100.0, rate = 0.03, dividend = 0.01, maturity = 1.0;
  const vol::HestonParams params;
  const double exact = prc::heston_price(prc::Call, spot, 100.0, rate, dividend, maturity, params);
  const std::vector<double> strikes{60.0, 72.0, 85.0, 93.0, 100.0, 107.0, 118.0, 131.0, 155.0, 190.0};

  out.writeln("   N       du      range N*du   strike spacing (log)   error at the money   largest error over ten strikes");
  for (std::int64_t n : {1024, 4096, 16384, 32768}) {
    for (double du : {0.001, 0.01, 0.05, 0.2}) {
      prc::HestonFftOptions options;
      options.num_fft = n;
      options.characteristic_fcn_step = du;
      const double error = prc::heston_price_fft(prc::Call, spot, 100.0, rate, dividend, maturity, params, options) - exact;
      double worst = 0.0;
      for (double strike : strikes) {
        worst = std::max(worst, std::abs(prc::heston_price_fft(prc::Call, spot, strike, rate, dividend, maturity, params, options) - prc::heston_price_cos(prc::Call, spot, strike, rate, dividend, maturity, params)));
      }
      out.writeln(n, du, n * du, 2.0 * 3.14159265358979323846 / (n * du), std::abs(error), worst);
    }
  }
}
```

Output

```text
   N       du      range N*du   strike spacing (log)   error at the money   largest error over ten strikes
1024 1.00e-03 1.02e+00 6.14e+00 1.09e+00 6.70e+05
1024 1.00e-02 1.02e+01 6.14e-01 1.46e-01 2.28e+00
1024 5.00e-02 5.12e+01 1.23e-01 1.14e-06 4.67e-02
1024 2.00e-01 2.05e+02 3.07e-02 1.03e-08 2.75e-04
4096 1.00e-03 4.10e+00 1.53e+00 5.69e-01 1.06e+01
4096 1.00e-02 4.10e+01 1.53e-01 1.81e-05 9.12e-02
4096 5.00e-02 2.05e+02 3.07e-02 1.22e-08 2.75e-04
4096 2.00e-01 8.19e+02 7.67e-03 1.03e-08 1.13e-06
16384 1.00e-03 1.64e+01 3.83e-01 3.46e-01 1.15e+00
16384 1.00e-02 1.64e+02 3.83e-02 1.22e-08 5.86e-04
16384 5.00e-02 8.19e+02 7.67e-03 1.22e-08 1.13e-06
16384 2.00e-01 3.28e+03 1.92e-03 1.03e-08 2.69e-09
32768 1.00e-03 3.28e+01 1.92e-01 1.73e-04 2.08e-01
32768 1.00e-02 3.28e+02 1.92e-02 1.22e-08 4.18e-05
32768 5.00e-02 1.64e+03 3.83e-03 1.22e-08 3.68e-08
32768 2.00e-01 6.55e+03 9.59e-04 1.03e-08 2.01e-09
```

The range *N* Δu must be large enough for the integrand to have decayed. With *N* = 4096 and Δu = 0.001 the range is 4.1, the integral is cut off, and the price is wrong (error 0.57 at the money and 10.6 over the strikes). From a range of about 200 on, the error at the money is 10⁻⁸ (the accuracy of the reference), and the largest error over ten strikes, which is dominated by interpolation between grid points, falls with the strike spacing: 2.8 × 10⁻⁴ at a spacing of 3.1% and 1.1 × 10⁻⁶ at 0.77%. More points (*N* = 16384) give a finer grid for the same Δu. The default, *N* = 16384 and Δu = 0.1, has a range of 1638 and a spacing of 0.38%, and holds the error over strikes from 60 to 190 below about 10⁻⁵ for maturities from a week to ten years (it was 4096 points before, with a spacing of 1.5% and errors a hundred times larger).

## Models without a finite moment

The damped transform exists only if the model has a finite moment E[S_T^(α + 1)]. The Heston model loses that moment when the volatility of variance is large and the maturity long: for the parameters below the moment of order 2.5 is infinite at ten years. The characteristic function then continues analytically past the singularity to numbers that are not a moment, and a transform built on them gives a wrong price (27.6 against the true 30.9 at the money, with α = 1.5). The FFT pricer tests for this: it checks that the moment is real, finite, at least the Jensen bound F^(α + 1) and no more than twice it (a moment far above the bound means the model is close to the explosion and the transform has lost its accuracy), and halves the damping exponent until it is. If no exponent works it refuses with a message that points to the COS pricer, which has no such condition. The program prices a model with a large volatility of variance at three maturities, with the FFT, integration and COS pricers.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  vol::HestonParams params;
  params.v0 = 0.09;
  params.theta = 0.06;
  params.kappa = 0.8;
  params.sigma = 0.9;  // 2 kappa theta = 0.096 against sigma^2 = 0.81: moments of the price explode at long maturities
  params.rho = -0.3;
  out.writeln("maturity  strike   FFT          integration   COS");
  for (double maturity : {1.0, 5.0, 10.0}) {
    for (double strike : {80.0, 100.0, 130.0}) {
      out.writeln(maturity, strike, prc::heston_price_fft(prc::Call, 100.0, strike, 0.03, 0.01, maturity, params), prc::heston_price(prc::Call, 100.0, strike, 0.03, 0.01, maturity, params),
                  prc::heston_price_cos(prc::Call, 100.0, strike, 0.03, 0.01, maturity, params));
    }
  }
}
```

Output

```text
maturity  strike   FFT          integration   COS
1.000000 80.000000 24.223549 24.223549 24.223549
1.000000 100.000000 10.076517 10.076517 10.076517
1.000000 130.000000 2.175132 2.175132 2.175132
5.000000 80.000000 33.078415 33.078415 33.078415
5.000000 100.000000 21.769960 21.769960 21.769960
5.000000 130.000000 10.819907 10.819907 10.819908
10.000000 80.000000 40.026473 40.026472 40.026491
10.000000 100.000000 30.947474 30.947474 30.947491
10.000000 130.000000 20.632598 20.632598 20.632615
```

The three pricers agree at one and five years to the printed six decimals (24.223549 at the strike 80 for one year, 33.078415 for five). At ten years, where the damping exponent has been halved, the FFT and integration prices still agree to 10⁻⁶ (40.026473 against 40.026472), and the COS price differs by about 2 × 10⁻⁵ (40.026491), which is its own truncation error for this heavy-tailed distribution. The refusal is reserved for models where no damping exponent gives a genuine moment.

The moment can be lost for any characteristic function you supply, not only Heston's; the same check applies to the generic FFT pricer ([Generic characteristic functions](generic_characteristic_functions.md)).

## The other controls

`damping_factor` has almost no effect over a wide range; `quadrature` changes the sum's weights; `vol_risk_premium` changes the model. A `log_strike_step` set explicitly decouples the strike spacing from the frequency grid (the transform is then evaluated by a fractional FFT). The program shows each, and the inputs that are rejected: a `num_fft` below 2, and a non-positive step or damping.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const double spot = 100.0, strike = 100.0, rate = 0.03, dividend = 0.01, maturity = 1.0;
  const vol::HestonParams params;
  const double exact = prc::heston_price(prc::Call, spot, strike, rate, dividend, maturity, params);
  auto fft = [&](prc::HestonFftOptions options) { return prc::heston_price_fft(prc::Call, spot, strike, rate, dividend, maturity, params, options); };

  out.writeln("integration price", exact);
  for (double alpha : {0.5, 1.0, 1.5, 3.0, 5.0}) {
    prc::HestonFftOptions o;
    o.damping_factor = alpha;
    out.writeln("damping_factor", alpha, " error", fft(o) - exact);
  }
  for (auto quadrature : {prc::HestonFftQuadrature::Simpson, prc::HestonFftQuadrature::Trapezoidal}) {
    prc::HestonFftOptions o;
    o.quadrature = quadrature;
    out.writeln(quadrature == prc::HestonFftQuadrature::Simpson ? "Simpson    " : "Trapezoidal", " error", fft(o) - exact);
  }
  {
    prc::HestonFftOptions o;
    o.log_strike_step = 0.002;
    const auto grid = prc::heston_price_fft_grid(prc::Call, spot, rate, dividend, maturity, params, o);
    const auto k = grid.strike.pull<double>();
    out.writeln("log_strike_step 0.002: strike ratio of neighbours", k[k.size() / 2 + 1] / k[k.size() / 2], " price at the money error", fft(o) - exact);
  }
  {
    prc::HestonFftOptions o;
    o.vol_risk_premium = 0.5;
    out.writeln("vol_risk_premium 0.5: price", fft(o), "(a different model; 0 gives", fft({}), ")");
  }

  auto attempt = [&](const char *label, auto edit) {
    prc::HestonFftOptions o;
    edit(o);
    try {
      fft(o);
      out.writeln(label, "-> no error");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      const auto start = message.find("heston_price_fft: ") + 18;
      out.writeln(label, "->", message.substr(start, message.find('\n') - start));
    }
  };
  attempt("num_fft = 1              ", [](prc::HestonFftOptions &o) { o.num_fft = 1; });
  attempt("characteristic_fcn_step 0", [](prc::HestonFftOptions &o) { o.characteristic_fcn_step = 0.0; });
  attempt("damping_factor 0         ", [](prc::HestonFftOptions &o) { o.damping_factor = 0.0; });
  out.writeln("maturity 0 gives the intrinsic value:", prc::heston_price_fft(prc::Call, 110.0, 100.0, rate, dividend, 0.0, params));
}
```

Output

```text
integration price 8.54225698
damping_factor 0.50000000  error -0.00000496
damping_factor 1.00000000  error 0.00000001
damping_factor 1.50000000  error 0.00000001
damping_factor 3.00000000  error 0.00000001
damping_factor 5.00000000  error 0.00000001
Simpson      error 0.00000001
Trapezoidal  error 0.00000001
log_strike_step 0.002: strike ratio of neighbours 1.00200200  price at the money error 0.00000001
vol_risk_premium 0.5: price 8.02014242 (a different model; 0 gives 8.54225699 )
num_fft = 1               -> num_fft must be >= 2
characteristic_fcn_step 0 -> characteristic_fcn_step must be finite and > 0
damping_factor 0          -> damping_factor must be finite and > 0
maturity 0 gives the intrinsic value: 10.00000000
```

The price is insensitive to `damping_factor` from 1 upward (the error is 10⁻⁸), and slightly worse at 0.5 (5 × 10⁻⁶). Simpson and trapezoidal weights agree to 10⁻⁸. A `log_strike_step` of 0.002 gives a grid 0.2% apart. A variance risk premium of 0.5 changes the model and the price, to 8.0201 from 8.5423. The invalid settings are rejected, and a zero maturity returns the intrinsic value.
