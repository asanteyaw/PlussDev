<!-- generated from engines_and_greeks.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Engines and Greeks

[Back to The Pricing Field](../finance_06_the_pricing_field.md)

`controls.engine` decides how the expectation {math}`e^{-rT}\mathbb{E}^{\mathbb{Q}}[(S_T-K)^+]` is evaluated, and `controls.risk` decides whether the sensitivities of the price are computed with it. There are seven engines, in five families:

| Engine | How the price is obtained | Models |
|---|---|---|
| `Analytic` | a closed formula: the Black-Scholes price, or the SABR implied-volatility expansion priced through Black | `BlackScholes`, `Sabr` |
| `Fft` | Fourier inversion of the characteristic function on a grid, all strikes from one transform | `Heston`, `Bates` |
| `Cos` | the Fourier-cosine expansion of the density on a truncated interval; for all three also Bermudan and American exercise (page 7) | `Heston`, `Bates`, `BlackScholes` |
| `MonteCarlo` | the average of the discounted payoff over simulated paths | all six models |
| `Lattice` | a binomial tree, which also allows early exercise (page 7) | `BlackScholes` |
| `Spectral` | the integral equation of the American exercise boundary by Chebyshev collocation (page 7) | `BlackScholes` |
| `Pde` | a two-dimensional finite-difference solution with ADI time stepping, with early exercise (page 7) | `Heston` |

The transform engines need the characteristic function {math}`\varphi(u)=\mathbb{E}^{\mathbb{Q}}[e^{iu\ln S_T}]` of the model in closed form, which Heston and Bates have and the rough models do not; simulation needs nothing of the model but a way to draw paths, which is why it is the only engine that prices all six. Each engine has its own settings in `controls`: `controls.cos`, `controls.fft`, `controls.mc` and `controls.analytic`. They exist side by side, so switching the engine does not lose the settings of the others.

## The settings

| Member of `controls` | Meaning | Default |
|---|---|---|
| `engine` | `Analytic`, `Fft`, `Cos`, `MonteCarlo`, `Lattice`, `Spectral` or `Pde` | `Analytic` |
| `risk` | `All` returns the Greeks with the price, `PriceOnly` does not | `All` |
| `loss`, `huber_delta` | the loss of a fit and the threshold of `Huber` (page 4) | `MSE`, 0.05 |
| `cos.num_terms`, `cos.truncation`, `cos.cumulant_step` | number of cosine terms, width of the interval in standard deviations, step of the cumulant differences | 1024, 12, 1e-3 |
| `cos.exercise_terms`, `cos.exercise_truncation` | early exercise: cosine terms of the recursion, and the half-width of the log-asset range in standard deviations | 128, 9 |
| `cos.extrapolation` | early exercise: the depth {math}`d` of the Richardson extrapolation to the American price (-1 selects 3 for `BlackScholes`, 2 for `Heston` and `Bates`) | -1 |
| `cos.variance_nodes`, `cos.variance_tolerance` | `Heston` and `Bates` with early exercise: quadrature nodes in the log-variance, and the probability left outside its range | 128, 1e-8 |
| `fft.num_fft`, `fft.du`, `fft.dk` | points of the transform, step of the integration, step of the log-strike grid (`NaN` derives it as 2π/(num_fft·du)) | 16384, 0.1, `NaN` |
| `fft.alpha`, `fft.quadrature`, `fft.mode` | damping, `Simpson` or `Trapezoidal`, `LittleTrap` or `Original` form of the characteristic function | 1.5, `Simpson`, `LittleTrap` |
| `fft.vol_risk_premium`, `fft.return_grid` | market price of volatility risk; return the whole strike grid | 0, `false` |
| `mc.paths`, `mc.steps` | simulated paths, and steps to the longest maturity | 40000, 252 |
| `mc.scheme`, `mc.rand_method` | `Euler`, `Milstein`, `Milstein2`, `QuadExp`; `Pseudo`, `Sobol`, `Halton`, `Torus` | `Euler`, `Pseudo` |
| `mc.antithetic`, `mc.control_variate`, `mc.importance_sampling` | variance-reduction switches | `false` |
| `mc.importance_shift`, `mc.control_vol` | the shift of the importance sampling, the volatility of the control variate | 0, 0 |
| `mc.randomized_quasi`, `mc.sobol_scrambling`, `mc.offset` | randomisation and starting point of a quasi-random sequence | `false`, 0, 0 |
| `mc.seed` | seed of the random numbers; 0 is a fixed seed too | 0 |
| `mc.basis_degree` | American/Bermudan only: degree of the continuation-value regression | 3 |
| `lattice.steps` | steps of the binomial tree | 1000 |
| `pde.spot_nodes`, `pde.variance_nodes`, `pde.time_steps` | nodes of the asset grid and of the variance grid, and steps in time | 240, 60, 100 |
| `spectral.nodes`, `spectral.quadrature` | Chebyshev nodes of the exercise boundary, Gauss-Legendre points of each time integral | 24, 48 |
| `spectral.max_iterations`, `spectral.tolerance` | fixed-point sweeps of the boundary equation, and the relative change that ends them | 60, 1e-13 |
| `analytic.with_greeks` | closed-form Greeks, when `risk = All` | `true` |

## Program 1: which engine prices which model

`validate()` checks the engine against the model (and the model's parameters, so each model here is built with valid ones), so asking it is a way to discover the table. The program tries each of the seven engines on each of the six models and prints `yes` where the combination is accepted.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0), .market = fin::market::make(100.0, 0.02, 0.0)};
  using fin::model::Kind;
  const std::vector<mod::Model> models{fin::model::make(Kind::BlackScholes, {0.2}),
                                       fin::model::make(Kind::Sabr, {0.2, 1.0, 0.3, -0.3}),
                                       fin::model::make(Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                                       fin::model::make(Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.1, -0.1, 0.15}),
                                       fin::model::make(Kind::RoughHeston, {1.5, 0.04, 0.3, -0.2, -0.6, 0.04}),
                                       fin::model::make(Kind::RoughBergomi, {0.04, 0.8, -0.2, -0.7})};
  const std::vector<std::pair<const char *, mod::Engine>> engines{{"Analytic", mod::Engine::Analytic}, {"Fft", mod::Engine::Fft}, {"Cos", mod::Engine::Cos}, {"MonteCarlo", mod::Engine::MonteCarlo}, {"Lattice", mod::Engine::Lattice}, {"Spectral", mod::Engine::Spectral}, {"Pde", mod::Engine::Pde}};
  const auto cell = [](const std::string &text, std::size_t width) { return text + std::string(width - text.size(), ' '); };
  out.writeln(cell("model", 15) + cell("Analytic", 10) + cell("Fft", 6) + cell("Cos", 6) + cell("MonteCarlo", 12) + cell("Lattice", 9) + cell("Spectral", 10) + "Pde");
  for (const auto &model : models) {
    field.model = model;
    std::string row = cell(model.name(), 15);
    std::size_t column = 0;
    for (const auto &[name, engine] : engines) {
      field.controls.engine = engine;
      bool supported = true;
      try {
        field.validate();
      } catch (const std::exception &) {
        supported = false;
      }
      row += cell(supported ? "yes" : "-", std::array<std::size_t, 7>{10, 6, 6, 12, 9, 10, 5}[column++]);
    }
    out.writeln(row);
  }
}
```

Output

```text
model          Analytic  Fft   Cos   MonteCarlo  Lattice  Spectral  Pde
BlackScholes   yes       -     yes   yes         yes      yes       -    
Sabr           yes       -     -     yes         -        -         -    
Heston         -         yes   yes   yes         -        -         yes  
Bates          -         yes   yes   yes         -        -         -    
RoughHeston    -         -     -     yes         -        -         -    
RoughBergomi   -         -     -     yes         -        -         -    
```

A combination marked `-` is an error naming the model and listing the engines that do work (page 1, Program 5). The table is the contract of the `Field`: Heston and Bates have three engines and can be cross-checked against each other, Black-Scholes and SABR have a closed form and a simulation (Black-Scholes also a lattice), and the rough models can only be simulated, at a cost that grows as the square of the number of steps (Chapter 5).

## Program 2: the COS engine

The Fourier-cosine method expands the density of {math}`\ln(S_T/K)` in a cosine series on an interval {math}`[a,b]` chosen to hold nearly all the probability, and reads the series coefficients off the characteristic function:

```{math}
C \;\approx\; e^{-rT}\,{\sum_{n=0}^{N-1}}{}'\;\mathrm{Re}\!\Big\{\varphi\!\Big(\tfrac{n\pi}{b-a}\Big)\,e^{-in\pi\frac{a}{b-a}}\Big\}\,V_n,
\qquad
[a,b]=\Big[c_1-L\sqrt{c_2+\sqrt{c_4}},\;c_1+L\sqrt{c_2+\sqrt{c_4}}\Big],
```

where the prime halves the first term, {math}`V_n` are the cosine coefficients of the payoff, {math}`c_1,c_2,c_4` are the first, second and fourth cumulants of the log-price, and {math}`L` is `truncation`. `num_terms` is {math}`N`. `cumulant_step` is the step of the finite differences that obtain the cumulants from the characteristic function. Two errors compete: the truncation error from discarding the density outside {math}`[a,b]`, controlled by {math}`L`, and the series error from stopping at {math}`N` terms, which falls geometrically for a smooth density.

The program prices a call at strikes 80, 100 and 120 with a very fine setting (8192 terms, {math}`L=16`) as the reference, then reduces `num_terms` at the default truncation of 12 and reduces `truncation` at 256 terms. Entries are absolute errors in price.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({80.0, 100.0, 120.0}), .maturity = pluss::make_series({1.0})},
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos}};
  field.controls.risk = mod::Risk::PriceOnly;
  field.controls.cos.num_terms = 8192;
  field.controls.cos.truncation = 16.0;
  const auto reference = field.price().price.items<double>();
  out.writeln("error of num_terms at truncation 12 (strikes 80, 100, 120)");
  field.controls.cos.truncation = 12.0;
  for (const std::int64_t terms : {16, 32, 64, 128, 256}) {
    field.controls.cos.num_terms = terms;
    const auto p = field.price().price.items<double>();
    out.writeln(terms, std::abs(p[0] - reference[0]), std::abs(p[1] - reference[1]), std::abs(p[2] - reference[2]));
  }
  out.writeln("error of truncation at 256 terms");
  field.controls.cos.num_terms = 256;
  for (const double truncation : {2.0, 4.0, 8.0, 12.0}) {
    field.controls.cos.truncation = truncation;
    const auto p = field.price().price.items<double>();
    out.writeln(truncation, std::abs(p[0] - reference[0]), std::abs(p[1] - reference[1]), std::abs(p[2] - reference[2]));
  }
}
```

Output

```text
error of num_terms at truncation 12 (strikes 80, 100, 120)
16 1.02e+00 8.31e-01 8.27e-01
32 1.41e-02 8.08e-03 2.37e-02
64 6.34e-05 7.42e-05 4.18e-05
128 6.37e-10 8.63e-10 1.26e-10
256 1.03e-13 3.73e-14 3.86e-14
error of truncation at 256 terms
2.00e+00 1.32e-01 6.34e-03 2.47e-02
4.00e+00 5.07e-04 1.22e-04 3.66e-05
8.00e+00 1.82e-09 3.99e-10 1.15e-10
1.20e+01 1.03e-13 3.73e-14 3.86e-14
```

The series error is the first table. Every doubling of `num_terms` reduces the error by a growing factor: 8, 0.4, 0.007, 3·10⁻⁶, 5·10⁻¹². That is exponential convergence, characteristic of a smooth density, and it means that the default of 1024 terms is far more than the 256 needed for 12 correct digits; it is a safe default, not a necessary one. The second table is the truncation error: with {math}`L=2` the interval holds only part of the density and the error is 10⁻², with {math}`L=4` it is 10⁻⁶, and from {math}`L=8` it has vanished below the series error. The default {math}`L=12` is conservative; the cost of a larger {math}`L` is that the series needs more terms to resolve the wider interval.

Two practical rules follow. First, raise `num_terms` for short maturities and for models with a sharply peaked density (small variance): the density is then narrow relative to {math}`[a,b]` and needs more terms. Second, to check a COS price, change `num_terms` and `truncation` and see that the digits you care about do not move.

## Program 3: the FFT engine

The Carr-Madan method prices a whole grid of strikes with one Fourier transform. The call price, damped by {math}`e^{\alpha k}` with {math}`k=\ln K` so that it becomes integrable, has the transform

```{math}
C(k)=\frac{e^{-\alpha k}}{\pi}\int_0^\infty e^{-iuk}\,\psi(u)\,du,\qquad
\psi(u)=\frac{e^{-rT}\,\varphi\big(u-(\alpha+1)i\big)}{\alpha^2+\alpha-u^2+i(2\alpha+1)u},
```

which requires {math}`\mathbb{E}[S_T^{\alpha+1}]<\infty`. The integral is discretised with step `du` at `num_fft` points, and the FFT returns the prices on a log-strike grid of spacing {math}`\Delta k = 2\pi/(N\,\mathrm{du})`; the price at the strikes you asked for is interpolated from that grid. This coupling is the point to understand: a smaller `du` integrates more accurately, but it makes the strike grid coarser, so the interpolation error grows. `dk` can be set explicitly instead of derived. `quadrature` is the rule used for the integral, `Simpson` or `Trapezoidal`, `alpha` is the damping, `mode` selects the form of the Heston characteristic function, and `vol_risk_premium` is the market price of volatility risk used to change the measure (zero for risk-neutral pricing, as here).

The program uses COS at its defaults as the reference and prints the absolute error of the FFT price for various settings, then a long maturity.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({80.0, 100.0, 120.0}), .maturity = pluss::make_series({1.0})},
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos}};
  field.controls.risk = mod::Risk::PriceOnly;
  const auto reference = field.price().price.items<double>();  // COS defaults
  field.controls.engine = mod::Engine::Fft;
  const auto show = [&](const std::string &label) {
    const auto p = field.price().price.items<double>();
    out.writeln(label, std::abs(p[0] - reference[0]), std::abs(p[1] - reference[1]), std::abs(p[2] - reference[2]));
  };
  show("defaults (num_fft 16384, du 0.1, alpha 1.5)");
  for (const double du : {0.5, 0.25, 0.05}) {
    field.controls.fft.du = du;
    show("du " + std::to_string(du).substr(0, 4));
  }
  field.controls.fft.du = 0.5;
  field.controls.fft.quadrature = fin::pricing::HestonFftQuadrature::Trapezoidal;
  show("du 0.50, trapezoidal rule");
  field.controls.fft.quadrature = fin::pricing::HestonFftQuadrature::Simpson;
  field.controls.fft.du = 0.1;
  for (const double alpha : {0.5, 1.0, 2.0, 4.0}) {
    field.controls.fft.alpha = alpha;
    show("alpha " + std::to_string(alpha).substr(0, 3));
  }
  field.controls.fft.alpha = 1.5;
  for (const std::int64_t points : {512, 2048}) {
    field.controls.fft.num_fft = points;
    show("num_fft " + std::to_string(points));
  }
  field.controls.fft.num_fft = 16384;
  field.instrument.maturity = pluss::make_series({15.0});
  field.controls.engine = mod::Engine::Cos;
  const auto long_reference = field.price().price.items<double>();
  field.controls.engine = mod::Engine::Fft;
  out.writeln("15 years: cos", long_reference);
  out.writeln("          fft, little-trap form", field.price().price.items<double>());
  field.controls.fft.mode = fin::pricing::HestonCfMode::Original;
  try {
    out.writeln("          fft, original form   ", field.price().price.items<double>());
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln("          fft, original form    refused:", text.substr(text.find("the transform"), 56), "...");
  }
}
```

Output

```text
defaults (num_fft 16384, du 0.1, alpha 1.5) 6.88e-09 1.33e-13 6.65e-08
du 0.50 2.69e-03 2.68e-03 2.68e-03
du 0.25 2.17e-07 2.17e-07 2.16e-07
du 0.05 5.91e-08 1.26e-13 7.41e-07
du 0.50, trapezoidal rule 6.51e-07 6.51e-07 6.51e-07
alpha 0.5 5.03e-06 5.02e-06 4.96e-06
alpha 1.0 6.88e-09 9.02e-13 6.65e-08
alpha 2.0 6.88e-09 1.37e-13 6.65e-08
alpha 4.0 6.88e-09 1.08e-13 6.65e-08
num_fft 512 7.68e-03 1.14e-06 5.09e-02
num_fft 2048 3.69e-05 1.33e-13 5.67e-05
15 years: cos [4.95e+01, 4.08e+01, 3.35e+01]
          fft, little-trap form [4.95e+01, 4.08e+01, 3.35e+01]
          fft, original form    refused: the transform produced a non-finite price (the character ...
```

Reading the table, line by line:

- **Defaults.** `num_fft = 16384`, `du = 0.1`, `alpha = 1.5` give 10⁻⁷ or better.
- **`du`.** The error is 3·10⁻³ at 0.5, 2·10⁻⁷ at 0.25, and 7·10⁻⁷ at 0.05. It is not monotone: the integration error falls as `du` shrinks, but the strike grid spacing {math}`2\pi/(N\,\mathrm{du})` widens, and below about 0.1 the interpolation between grid strikes dominates. The default sits near the best compromise for 16384 points.
- **`quadrature`.** At the coarse step `du = 0.5`, the trapezoidal rule has error 7·10⁻⁷ against 3·10⁻³ for Simpson's. For a smooth integrand that decays to zero at both ends, the trapezoidal rule converges faster than any power of `du`, while Simpson's alternating weights add error at a coarse step. At the default step the two agree to the precision shown, so the choice matters only when you coarsen `du` to save time.
- **`alpha`.** Below 1 (0.5 here) the damping is too weak and the error is 5·10⁻⁶. From 1 to 4 the prices are the same to the digits shown, because the engine lowers `alpha` by halving until the moment {math}`\mathbb{E}[S_T^{\alpha+1}]` is genuinely finite, so a larger request is moderated and gives no gain.
- **`num_fft`.** With 512 points the error is 5·10⁻² at strike 120, with 2048 it is 6·10⁻⁵, and the default 16384 gives 7·10⁻⁸.
- **`mode`.** The two forms of the Heston characteristic function, `Original` and `LittleTrap`, are algebraically equal but the original one has a complex square root and logarithm whose branch cut is crossed at long maturities. At 15 years `LittleTrap` agrees with COS to the digits shown (49.5, 40.8, 33.5) and `Original` overflows: the engine detects the non-finite transform and refuses it rather than return `nan`. Keep the default.

## Program 4: the Monte Carlo engine

`controls.mc` carries the settings of Chapter 4: `paths`, `steps`, `scheme` (`Euler`, `Milstein`, `Milstein2`, `QuadExp`), `rand_method` (`Pseudo`, `Sobol`, `Halton`, `Torus`), `randomized_quasi`, `sobol_scrambling`, `antithetic`, `control_variate`, `importance_sampling`, `importance_shift`, `control_vol`, `offset` and `seed`. A Monte Carlo price is a random variable, and the `seed` makes it reproducible: the same seed gives the same price, the same price again on the next call, and a different seed gives an independent estimate. Repeating the pricing over seeds is how you get an error bar, since the estimator

```{math}
\hat C_N=\frac{e^{-rT}}{N}\sum_{j=1}^{N}(S^{(j)}_T-K)^+
```

has standard error {math}`s/\sqrt{N}` but returns no estimate of {math}`s`. The program prices a call 12 times at each setting, with seeds 1 to 12, and prints the mean and the standard deviation over the seeds. The exact price (COS) is 8.6281.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos}};
  field.controls.risk = mod::Risk::PriceOnly;
  const double exact = field.price().price.items<double>()[0];
  field.controls.engine = mod::Engine::MonteCarlo;
  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 64;

  const auto price = [&] { return field.price().price.items<double>()[0]; };
  field.controls.mc.seed = 1;
  const double a = price(), b = price();
  field.controls.mc.seed = 2;
  out.writeln("seed 1 twice", a, b, " seed 2", price(), " exact", exact);

  const auto spread = [&](const std::string &label) {
    double sum = 0.0, sum2 = 0.0;
    const int runs = 12;
    for (int seed = 1; seed <= runs; ++seed) {
      field.controls.mc.seed = seed;
      const double p = price();
      sum += p;
      sum2 += p * p;
    }
    const double mean = sum / runs;
    out.writeln(label, " mean", mean, " std over seeds", std::sqrt((sum2 - runs * mean * mean) / (runs - 1)));
  };
  spread("plain          ");
  field.controls.mc.antithetic = true;
  spread("antithetic     ");
  field.controls.mc.antithetic = false;
  field.controls.mc.rand_method = fin::volatility::RandMethod::Sobol;
  field.controls.mc.randomized_quasi = true;
  spread("sobol randomised");
  field.controls.mc.rand_method = fin::volatility::RandMethod::Pseudo;
  field.controls.mc.randomized_quasi = false;
  field.controls.mc.scheme = fin::volatility::SimScheme::QuadExp;
  spread("quadratic-exp  ");
}
```

Output

```text
seed 1 twice 8.7516 8.7516  seed 2 8.6213  exact 8.6281
plain            mean 8.6407  std over seeds 0.1033
antithetic       mean 8.6515  std over seeds 0.0539
sobol randomised  mean 8.6341  std over seeds 0.1026
quadratic-exp    mean 8.6393  std over seeds 0.1039
```

The first line shows the contract of the seed: seed 1 gives 8.7516 twice, seed 2 gives 8.6213, and the exact value is 8.6281. The four settings then compare estimators over 12 seeds of 20,000 paths:

- **Plain.** The standard deviation over seeds is 0.103, which is {math}`s/\sqrt{N}=13.9/\sqrt{20000}=0.098` to within the sampling error of 12 runs. The mean, 8.6407, is within one standard error of the mean (0.03) of the exact price.
- **Antithetic.** Each draw {math}`Z` is paired with {math}`-Z`. The payoff is monotone in {math}`Z`, so the two halves of each pair are negatively correlated and the average has lower variance: the standard deviation falls to 0.054, almost half, for no more simulation effort than the plain run.
- **Randomised Sobol.** The standard deviation is the same as plain, 0.103. A quasi-random sequence helps when the effective dimension is low; here 64 steps of two shocks each give 128 dimensions, and the gain is gone. Chapter 4 shows where it does help.
- **`QuadExp`.** This is a discretisation scheme, not a variance reduction. It changes the bias from taking steps of finite size, not the spread over seeds, and the spread (0.104) is that of plain sampling.

Variance reduction is cheaper than more paths: halving the standard error by adding paths costs four times the work, and antithetic sampling got there for free.

## Program 5: the Greeks and their definitions

`risk` is `All` or `PriceOnly`. With `All`, `price()` also returns the five sensitivities in `result.greeks`, each a `Series` of the shape of the price:

| Greek | Definition | Unit |
|---|---|---|
| `delta` | {math}`\Delta=\partial C/\partial S` | per unit of spot |
| `gamma` | {math}`\Gamma=\partial^2 C/\partial S^2` | per unit of spot squared |
| `vega` | the derivative with respect to the volatility level: {math}`\partial C/\partial\sigma` for `BlackScholes`, {math}`\partial C/\partial\alpha_0` for `Sabr`, {math}`\partial C/\partial v_0` for the Heston family, {math}`\partial C/\partial\xi` for `RoughBergomi` | per unit of that parameter |
| `theta` | {math}`\Theta=\partial C/\partial t=-\partial C/\partial T` | per year |
| `rho` | {math}`\partial C/\partial r` | per unit of rate, not per 1% |

For Black-Scholes without dividends they have closed forms, with {math}`d_1=\dfrac{\ln(S/K)+(r+\sigma^2/2)T}{\sigma\sqrt T}` and {math}`d_2=d_1-\sigma\sqrt T`:

```{math}
\Delta=N(d_1),\quad
\Gamma=\frac{\phi(d_1)}{S\sigma\sqrt T},\quad
\mathcal V=S\,\phi(d_1)\sqrt T,\quad
\Theta=-\frac{S\,\phi(d_1)\,\sigma}{2\sqrt T}-rKe^{-rT}N(d_2),\quad
\rho=KTe^{-rT}N(d_2),
```

where {math}`N` and {math}`\phi` are the standard normal distribution and density. With dividends the first three carry a factor {math}`e^{-qT}` and the rest gain terms in {math}`q`. The `Analytic` engine returns these closed forms. The other engines obtain the Greeks in the cheapest way their method allows, and only the last resort is to **bump and reprice**:

| Engine and model | Where the Greeks come from | Cost on top of the price |
|---|---|---|
| `Cos`, Heston and Bates (European) | the same pass over the characteristic function, with the factor of each Greek in the integrand | none |
| `Fft`, Heston and Bates | delta and gamma from the strike grid by homogeneity; vega, rho and theta from three transforms of the characteristic function times the factor of the Greek (the original form takes them from the little-trap form, the same analytic function) | three transforms |
| `MonteCarlo`, Heston, Bates and the rough models (European) | the same paths: delta and rho pathwise, gamma and theta as differences over the paths, vega pathwise by differentiating the paths in the initial variance | none |
| `MonteCarlo`, Black-Scholes (European) | pathwise formulas on the same normals (delta, vega, rho, theta), gamma by a second difference of the payoff | none |
| `MonteCarlo`, SABR (European) | the initial forward and the initial volatility are tracked through the simulation: delta and vega pathwise, gamma by a second difference along the pathwise Jacobian, rho and theta from the discount, the forward's growth and a backward difference over earlier dates | none |
| `Lattice`, Black-Scholes | delta, gamma and theta from the nodes of the first levels of the tree; vega and rho by carrying their derivatives through the backward recursion alongside the values | none |
| `Pde`, Heston | delta, gamma, vega and theta read off the solved surface (theta from the PDE itself); rho by differentiating every step of the scheme, the exercise constraint included | one extra field per step |
| `Analytic`, SABR | Black's (or Bachelier's) formula differentiated exactly, with the derivatives of the smile taken on the expansion itself | none |
| `Spectral`, American Black-Scholes | delta, gamma and theta from the solved boundary (it does not depend on the spot and is stationary in the time to maturity); vega and rho by differentiating the boundary equation | a few boundary iterations on dual numbers |
| `Cos`, Bermudan and American (Black-Scholes) | delta and gamma from the final cosine expansion; vega, rho and theta by differentiating the recursion itself | one tangent per Greek |
| `Cos`, Bermudan and American (Heston, Bates) | delta, gamma and vega (the kernel differentiated in the initial variance) from the final expansion; rho through the recursion; theta from the pricing equation applied to the expansion | one extra contraction per step |
| `MonteCarlo`, American exercise (least squares) | one regression fit; delta, rho and vega pathwise along the fitted exercise rule, gamma by applying that rule to the paths with the spot scaled (re-simulated from the scaled spot under SABR, whose paths are not proportional to it, with the spot, the drift and the volatility tracked), theta from the pricing equation under Black-Scholes and by a backward difference of the fit over two earlier horizons of the same paths otherwise | one extra simulation (the tracked one) |

A Greek that is a derivative of the simulation or of the solver is computed by differentiating the computation itself, so it carries no step size and no repricing. The remaining finite differences are stencils on quantities that are already there: the second difference of a payoff in the spot (a payoff has no second derivative to differentiate), and the differences over earlier dates of the same paths. Control variates leave the Greeks to the plain estimator (they sharpen the price only), and importance sampling reweights each path by its likelihood ratio, so neither needs a reprice. The one case that still bumps and reprices is a lattice with fewer than two steps, which has no second level of nodes.

The program prices an at-the-money call under Black-Scholes at 20% volatility with the closed form, computes the five formulas above by hand, and then prices again by Monte Carlo with `risk = All`. It finishes with `PriceOnly`, which leaves the Greeks undefined.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double spot = 100.0, strike = 100.0, rate = 0.02, maturity = 1.0, sigma = 0.2;
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, strike, maturity), .market = fin::market::make(spot, rate, 0.0),
                   .model = fin::model::BlackScholes{.volatility = pluss::make_series({sigma})}, .controls = {.engine = mod::Engine::Analytic}};
  const auto row = [&](const std::string &label, const fin::module::Field::Result &r) {
    out.writeln(label, r.greeks.delta.items<double>(), r.greeks.gamma.items<double>(), r.greeks.vega.items<double>(), r.greeks.theta.items<double>(), r.greeks.rho.items<double>());
  };
  out.writeln("                delta     gamma     vega      theta     rho");
  row("closed form    ", field.price());

  const double d1 = (std::log(spot / strike) + (rate + 0.5 * sigma * sigma) * maturity) / (sigma * std::sqrt(maturity));
  const double d2 = d1 - sigma * std::sqrt(maturity);
  const double pdf = std::exp(-0.5 * d1 * d1) / std::sqrt(2.0 * M_PI), cdf1 = 0.5 * std::erfc(-d1 / std::sqrt(2.0)), cdf2 = 0.5 * std::erfc(-d2 / std::sqrt(2.0));
  out.writeln("by hand        ", cdf1, pdf / (spot * sigma * std::sqrt(maturity)), spot * pdf * std::sqrt(maturity),
              -spot * pdf * sigma / (2.0 * std::sqrt(maturity)) - rate * strike * std::exp(-rate * maturity) * cdf2, strike * maturity * std::exp(-rate * maturity) * cdf2);

  field.controls.engine = mod::Engine::MonteCarlo;
  field.controls.mc.paths = 40000;
  field.controls.mc.steps = 8;
  field.controls.mc.seed = 3;
  row("monte carlo    ", field.price());
  field.controls.risk = mod::Risk::PriceOnly;
  out.writeln("price only: delta defined?", field.price().greeks.delta.defined());
}
```

Output

```text
                delta     gamma     vega      theta     rho
closed form     [0.57926] [0.01955] [39.10427] [-4.89063] [49.00993]
by hand         0.57926 0.01955 39.10427 -4.89063 49.00993
monte carlo     [0.58138] [0.01936] [39.36801] [-4.92014] [49.16677]
price only: delta defined? false
```

The closed form and the hand formulas agree to every digit shown: {math}`\Delta=0.57926`, {math}`\Gamma=0.01955`, {math}`\mathcal V=39.10427`, {math}`\Theta=-4.89063`, {math}`\rho=49.00993`. The Monte Carlo row, with 40,000 paths, takes its Greeks from the pathwise formulas on the same normals as the price (no bumping): delta 0.58138, vega 39.368, theta −4.920 and rho 49.167 are each within a percent of the exact values, and gamma, 0.01936 against 0.01955, is within 1%. Gamma is the weakest from simulation, since it is a second difference of a payoff with a kink. With `PriceOnly` the Greeks are undefined; check `defined()` before reading them.

Because the Greeks cost extra on most engines (nothing on the closed forms and the COS engine), never leave `risk = All` in a calibration loop. It is the default of a new `Field`, so set `PriceOnly` before iterating.

## Program 6: what the Heston vega is

For the Heston family the vega is the derivative with respect to the **initial variance** {math}`v_0`, not the volatility {math}`\sigma`. They are related by {math}`v=\sigma^2`: if the whole variance path moves with {math}`v_0`, then {math}`\partial C/\partial\sigma=2\sigma\,\partial C/\partial v_0`. The program checks this with a Heston model that is almost a Black-Scholes model: very slow mean reversion, a vol of vol of 10⁻⁴ and {math}`v_0=0.04`, so the variance stays near 0.04 and the volatility is 20%.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0), .market = fin::market::make(100.0, 0.02, 0.0)};
  const auto row = [&](const std::string &label, const fin::module::Field::Result &r) {
    out.writeln(label, r.price.items<double>(), r.greeks.delta.items<double>(), r.greeks.vega.items<double>(), r.greeks.theta.items<double>());
  };
  field.model = fin::model::make(fin::model::Kind::Heston, {0.01, 0.04, 0.0001, 0.0, 0.04});
  field.controls.engine = mod::Engine::Cos;
  const auto heston = field.price();
  row("heston, almost constant variance", heston);
  field.model = fin::model::BlackScholes{.volatility = pluss::make_series({0.2})};
  field.controls.engine = mod::Engine::Analytic;
  row("black-scholes at 20%            ", field.price());
  out.writeln("heston vega * 2 sigma =", heston.greeks.vega.items<double>()[0] * 2.0 * 0.2);
}
```

Output

```text
heston, almost constant variance [8.91604] [0.57926] [97.27350] [-4.89063]
black-scholes at 20%             [8.91604] [0.57926] [39.10427] [-4.89063]
heston vega * 2 sigma = 38.90940
```

The prices, delta and theta coincide with Black-Scholes at 20% to five decimals, which confirms the model is, to that accuracy, a constant-volatility one. The Heston vega is 97.27 per unit of variance. Multiplying by {math}`2\sigma=0.4` gives 38.909 against the Black-Scholes vega of 39.104. The 0.5% difference is exact and explicable: with {math}`\kappa=0.01` the variance moves away from {math}`v_0` as {math}`v_t=\theta+(v_0-\theta)e^{-\kappa t}`, so a change in {math}`v_0` reaches the average variance over the year only with weight {math}`\frac{1}{T}\int_0^T e^{-\kappa t}dt=0.995`. When you compare a model vega with a Black-Scholes vega, convert first.

## Program 7: the whole FFT grid

The FFT computes prices on a full grid of strikes, and normally only the requested strikes are kept. With `controls.fft.return_grid = true`, `result.price` and `result.strike_grid` hold the whole grid, 16384 prices at the strike of each grid node, from which you can read any strike by interpolation. The inputs must be scalar (one strike, one maturity, one spot), since the grid depends on them. The program takes the grid for a one-year call, interpolates it linearly at five strikes, and then asks for `risk = All`, which is not available on the grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0), .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::Fft}};
  field.controls.risk = mod::Risk::PriceOnly;
  field.controls.fft.return_grid = true;
  const auto grid = field.price();
  const auto k = grid.strike_grid.items<double>(), c = grid.price.items<double>();
  out.writeln("points", k.size(), " first strike", k.front(), " last strike", k.back());
  out.writeln("strike  price");
  for (const double target : {80.0, 90.0, 100.0, 110.0, 120.0}) {
    std::size_t i = 0;
    while (i + 2 < k.size() && k[i + 1] < target) ++i;
    const double w = (target - k[i]) / (k[i + 1] - k[i]);
    out.writeln(target, c[i] + w * (c[i + 1] - c[i]));
  }
  field.controls.risk = mod::Risk::All;
  try {
    field.price();
  } catch (const std::exception &e) {
    std::string message = e.what();
    out.writeln(message.substr(0, message.find('\n')));
  }
}
```

Output

```text
points 16384  first strike 0.0000  last strike 4386297051952354.0000
strike  price
80.0000 23.0606
90.0000 15.1139
100.0000 8.6281
110.0000 4.0598
120.0000 1.4922
subsp::math::finance::pricer::Fft::price: RiskSpec::All is not supported with return_grid=true
```

The grid has 16384 nodes and spans from near zero to an absurd upper strike (4·10¹⁵): the grid is uniform in log-strike and wide, and only the part near the spot is meaningful. Interpolating at 80, 90, 100, 110 and 120 gives prices that match the earlier engines to four digits (23.0606, 8.6281, 1.4922 against 23.0606, 8.6281, 1.4918 from the COS engine), with differences at 110 and 120 from linear interpolation on the log-strike grid. For accurate prices use the engine at the strikes you need; use the grid to see the whole curve in one call, for plotting or to feed a fitting routine. `Risk::All` with `return_grid` is rejected because the bumped prices would each need their own grid.

## Program 8: calling a pricer without a Field

A `Field` calls a pricer for you. The pricers are also values you can use on their own: `pricer::Analytic`, `pricer::Fft`, `pricer::COS` and `pricer::MonteCarlo` each have a `price(instrument, model, market, risk, cfg)` member that takes the same objects as the `Field` and returns the same `module::Field::Result`. Use them when one engine is all you need, or to keep several differently configured engines side by side. `risk` is a `pricer::RiskSpec`. The settings are the fields of the struct, which you give in the braces. `Analytic` has `with_greeks`, which turns off the closed-form Greeks even when `RiskSpec::All` is asked.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto heston = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  const auto bs = fin::model::BlackScholes{.volatility = pluss::make_series({0.2})};

  const fin::pricer::COS cos{.num_terms = 512, .truncation = 10.0};
  const fin::pricer::Fft fft{.num_fft = 4096, .du = 0.05};
  const fin::pricer::Analytic analytic{};
  out.writeln("cos     ", cos.price(inst, heston, mkt).price.items<double>());
  out.writeln("fft     ", fft.price(inst, heston, mkt, fin::pricer::RiskSpec::All).greeks.delta.items<double>());
  out.writeln("analytic", analytic.price(inst, bs, mkt, fin::pricer::RiskSpec::All).greeks.gamma.items<double>());
  const fin::pricer::Analytic no_greeks{.with_greeks = false};
  out.writeln("with_greeks = false, RiskSpec::All -> delta defined?", no_greeks.price(inst, bs, mkt, fin::pricer::RiskSpec::All).greeks.delta.defined());
}
```

Output

```text
cos      [8.628108]
fft      [0.644680]
analytic [0.019552]
with_greeks = false, RiskSpec::All -> delta defined? false
```

The COS price, 8.628108, is the same as through the `Field`; the FFT delta is 0.644671 against 0.644691 from COS, the two differing in the fifth decimal because each is a difference quotient of its own prices; and the Black-Scholes gamma is 0.019552. With `with_greeks = false` the Greeks are undefined even though `RiskSpec::All` was asked.

## Checklist

1. Choose the engine to suit the model: `Analytic` for Black-Scholes and SABR, `Cos` or `Fft` for Heston and Bates, `MonteCarlo` for the rough models.
2. Check a transform price by changing its settings: `num_terms` and `truncation` for COS, `num_fft` and `du` for FFT. The digits you rely on must not move.
3. Keep `mode = LittleTrap` for the FFT, and do not raise `alpha` to gain accuracy.
4. A Monte Carlo price needs a seed to be reproducible and several seeds to have an error bar. Use antithetic sampling before more paths.
5. Set `risk = PriceOnly` in loops; `All` is free on the closed forms and COS, a few transforms or simulations on FFT, Monte Carlo, lattice and PDE, and nine pricings elsewhere.
6. The Heston vega is per unit of initial variance; multiply by {math}`2\sigma` to compare with a volatility vega.
7. Use `return_grid` to see a whole curve, and the engine itself for the prices you will quote.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="building_a_field.html">
    <span class="label">Previous</span>
    <span class="title">Building a Field</span>
  </a>
  <a class="next-link" href="models_and_parameters.html">
    <span class="label">Next</span>
    <span class="title">Models and parameters</span>
  </a>
</div>
<!-- apiref:nav:end -->
