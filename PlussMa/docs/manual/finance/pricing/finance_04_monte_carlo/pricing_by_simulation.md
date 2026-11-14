<!-- generated from pricing_by_simulation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Pricing by simulation

[Back to Monte Carlo](../finance_04_monte_carlo.md)

The Monte Carlo price of a European option is the average of the discounted payoff over simulated paths. The `MonteCarlo` engine of a `Field` does exactly that for whatever model the field holds, with the controls of `field.controls.mc`:

```cpp
module::Engine module::Controls::engine = module::Engine::Analytic;
pricer::MonteCarlo module::Controls::mc;
module::Field::Result module::Field::price(Config cfg = {}) const;
```

Choose the engine, set the controls on `field.controls.mc`, and price:

```cpp
field.controls.engine = mod::Engine::MonteCarlo;
field.controls.mc.paths = 40000;  field.controls.mc.steps = 252;  field.controls.mc.seed = 1;
const double price = field.price().price.items<double>()[0];
```

The simulation is risk neutral: the drift is `rate − dividend_yield` whatever the model. A maturity of zero returns the intrinsic value. The result is a single number per option, with no error bar; its uncertainty is found by repeating the price with different seeds. The programs of this chapter share three helpers: `monte_carlo` builds the field, `cos_reference` prices the same Heston option by the COS engine as the reference to be judged against, and `mean_sd` summarises repeated prices.

## The error falls as 1/√N

The statistical error of a Monte Carlo price is σ/√N, with σ the standard deviation of one discounted payoff. The program prices a Heston call 12 times with different seeds at each number of paths, and prints the mean, the standard deviation of the 12 prices (the standard error of one run) and that standard deviation times √N, which should be roughly constant.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace prc = fin::pricing;
using namespace pluss::tils;
using pluss::Series;

constexpr double kSpot = 100.0, kRate = 0.03, kDiv = 0.01;
const std::vector<double> kHeston{1.5, 0.04, 0.3, -0.7, 0.04};

// A call under the given model, priced (or simulated) by Monte Carlo; the controls in field.controls.mc are the caller's to set.
mod::Field monte_carlo(fin::model::Kind kind, const std::vector<double> &params, double strike = 100.0, double maturity = 1.0) {
  return mod::Field{.instrument = fin::instrument::make(prc::Call, strike, maturity),
                    .market = fin::market::make(kSpot, kRate, kDiv),
                    .model = fin::model::make(kind, params),
                    .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
}
// The COS price of the same Heston call: the reference the simulation is judged against.
double cos_reference(double strike, const std::vector<double> &params = kHeston, double maturity = 1.0) {
  mod::Field f = monte_carlo(fin::model::Kind::Heston, params, strike, maturity);
  f.controls.engine = mod::Engine::Cos;
  return f.price().price.items<double>()[0];
}
double price1(const mod::Field &f) { return f.price().price.items<double>()[0]; }
// Mean and standard deviation of a set of repeated prices.
std::pair<double, double> mean_sd(const std::vector<double> &x) {
  double mean = 0.0;
  for (double v : x) mean += v / x.size();
  double variance = 0.0;
  for (double v : x) variance += (v - mean) * (v - mean) / (x.size() - 1);
  return {mean, std::sqrt(variance)};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("COS reference", cos_reference(100.0));
  out.writeln("   paths     mean of 12     std dev of one run    std dev x sqrt(paths)");
  for (std::int64_t paths : {1000, 4000, 16000, 64000}) {
    std::vector<double> prices;
    for (int seed = 1; seed <= 12; ++seed) {
      mod::Field field = monte_carlo(fin::model::Kind::Heston, kHeston);
      field.controls.mc.paths = paths;
      field.controls.mc.steps = 32;
      field.controls.mc.seed = 100 + seed;
      prices.push_back(price1(field));
    }
    const auto [mean, sd] = mean_sd(prices);
    out.writeln(paths, mean, sd, sd * std::sqrt(static_cast<double>(paths)));
  }
}
```

Output

```text
COS reference 8.5423
   paths     mean of 12     std dev of one run    std dev x sqrt(paths)
1000 8.5427 0.4135 13.0768
4000 8.4464 0.1361 8.6058
16000 8.5430 0.0609 7.7094
64000 8.5694 0.0355 8.9810
```

The standard deviation of one run falls from 0.41 at 1000 paths to 0.036 at 64000, and its product with √paths stays in the same range (13.1, 8.6, 7.7 and 9.0; each standard deviation is itself estimated from 12 runs, so it carries an error of about 20%). The means of 12 runs are within 0.1 of the reference at every size. Quadrupling the number of paths roughly halves the error, as it should.

## The discretisation bias

Besides the statistical error there is a bias from replacing the continuous process by `steps` time steps. It falls with the number of steps but, for the same effort, takes paths away. The program prices the same call with 4 to 128 steps, with enough paths and seeds that the statistical error (printed) is small against the bias.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace prc = fin::pricing;
using namespace pluss::tils;
using pluss::Series;

constexpr double kSpot = 100.0, kRate = 0.03, kDiv = 0.01;
const std::vector<double> kHeston{1.5, 0.04, 0.3, -0.7, 0.04};

// A call under the given model, priced (or simulated) by Monte Carlo; the controls in field.controls.mc are the caller's to set.
mod::Field monte_carlo(fin::model::Kind kind, const std::vector<double> &params, double strike = 100.0, double maturity = 1.0) {
  return mod::Field{.instrument = fin::instrument::make(prc::Call, strike, maturity),
                    .market = fin::market::make(kSpot, kRate, kDiv),
                    .model = fin::model::make(kind, params),
                    .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
}
// The COS price of the same Heston call: the reference the simulation is judged against.
double cos_reference(double strike, const std::vector<double> &params = kHeston, double maturity = 1.0) {
  mod::Field f = monte_carlo(fin::model::Kind::Heston, params, strike, maturity);
  f.controls.engine = mod::Engine::Cos;
  return f.price().price.items<double>()[0];
}
double price1(const mod::Field &f) { return f.price().price.items<double>()[0]; }
// Mean and standard deviation of a set of repeated prices.
std::pair<double, double> mean_sd(const std::vector<double> &x) {
  double mean = 0.0;
  for (double v : x) mean += v / x.size();
  double variance = 0.0;
  for (double v : x) variance += (v - mean) * (v - mean) / (x.size() - 1);
  return {mean, std::sqrt(variance)};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double reference = cos_reference(100.0);
  out.writeln("COS reference", reference);
  out.writeln("   steps     mean of 8 runs     bias      standard error of the mean");
  for (std::int64_t steps : {4, 8, 16, 32, 64, 128}) {
    std::vector<double> prices;
    for (int seed = 1; seed <= 8; ++seed) {
      mod::Field field = monte_carlo(fin::model::Kind::Heston, kHeston);
      field.controls.mc.paths = 30000;
      field.controls.mc.steps = steps;
      field.controls.mc.seed = 500 + seed;
      prices.push_back(price1(field));
    }
    const auto [mean, sd] = mean_sd(prices);
    out.writeln(steps, mean, mean - reference, sd / std::sqrt(8.0));
  }
}
```

Output

```text
COS reference 8.5423
   steps     mean of 8 runs     bias      standard error of the mean
4 8.7362 0.1939 0.0211
8 8.6700 0.1278 0.0230
16 8.6101 0.0679 0.0308
32 8.5528 0.0106 0.0173
64 8.5531 0.0108 0.0283
128 8.5418 -0.0004 0.0201
```

The bias falls with the number of steps, from +0.194 at 4 steps through +0.128, +0.068 and +0.011 at 8, 16 and 32. At 32 steps and above the bias is no longer distinguishable from the statistical error of the mean (0.017 to 0.030), which is the point of diminishing return: more steps cost proportionally more time and give nothing measurable.

## Schemes

`scheme` selects how the variance is stepped. `Euler` is the simplest; `Milstein` and `Milstein2` add a correction term; `QuadExp` is Andersen's quadratic-exponential scheme, which matches the first two moments of the variance exactly and handles a variance near zero; `Transition` samples the exact non-central chi-squared transition of the square-root process (this needs the degree 4κθ/σ² of that distribution to exceed 1, so that the sampler can be driven by the shock that is correlated with the asset; below it, `Transition` uses the quadratic-exponential step instead). With 32 steps and parameters that satisfy the Feller condition they price alike; they differ when the variance is often near zero, which the next program arranges with a small mean reversion and a large volatility of variance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace prc = fin::pricing;
using namespace pluss::tils;
using pluss::Series;

constexpr double kSpot = 100.0, kRate = 0.03, kDiv = 0.01;
const std::vector<double> kHeston{1.5, 0.04, 0.3, -0.7, 0.04};

// A call under the given model, priced (or simulated) by Monte Carlo; the controls in field.controls.mc are the caller's to set.
mod::Field monte_carlo(fin::model::Kind kind, const std::vector<double> &params, double strike = 100.0, double maturity = 1.0) {
  return mod::Field{.instrument = fin::instrument::make(prc::Call, strike, maturity),
                    .market = fin::market::make(kSpot, kRate, kDiv),
                    .model = fin::model::make(kind, params),
                    .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
}
// The COS price of the same Heston call: the reference the simulation is judged against.
double cos_reference(double strike, const std::vector<double> &params = kHeston, double maturity = 1.0) {
  mod::Field f = monte_carlo(fin::model::Kind::Heston, params, strike, maturity);
  f.controls.engine = mod::Engine::Cos;
  return f.price().price.items<double>()[0];
}
double price1(const mod::Field &f) { return f.price().price.items<double>()[0]; }
// Mean and standard deviation of a set of repeated prices.
std::pair<double, double> mean_sd(const std::vector<double> &x) {
  double mean = 0.0;
  for (double v : x) mean += v / x.size();
  double variance = 0.0;
  for (double v : x) variance += (v - mean) * (v - mean) / (x.size() - 1);
  return {mean, std::sqrt(variance)};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // 2 kappa theta = 0.04 is far below sigma^2 = 1: the variance is often near zero
  const std::vector<double> params{0.5, 0.04, 1.0, -0.7, 0.04};
  const double reference = cos_reference(100.0, params);
  out.writeln("COS reference", reference);
  const std::vector<std::pair<const char *, fin::volatility::SimScheme>> schemes{{"Euler    ", fin::volatility::SimScheme::Euler}, {"Milstein ", fin::volatility::SimScheme::Milstein},
                                                                                {"Milstein2", fin::volatility::SimScheme::Milstein2}, {"QuadExp  ", fin::volatility::SimScheme::QuadExp},
                                                                                {"Transition", fin::volatility::SimScheme::Transition}};
  for (const auto &[name, scheme] : schemes) {
    std::vector<double> prices;
    for (int seed = 1; seed <= 8; ++seed) {
      mod::Field field = monte_carlo(fin::model::Kind::Heston, params);
      field.controls.mc.paths = 30000;
      field.controls.mc.steps = 32;
      field.controls.mc.scheme = scheme;
      field.controls.mc.seed = 900 + seed;
      prices.push_back(price1(field));
    }
    const auto [mean, sd] = mean_sd(prices);
    out.writeln(name, " mean of 8 runs", mean, " bias", mean - reference);
  }
}
```

Output

```text
COS reference 6.1563
Euler      mean of 8 runs 8.2822  bias 2.1259
Milstein   mean of 8 runs 8.6197  bias 2.4633
Milstein2  mean of 8 runs 8.6276  bias 2.4713
QuadExp    mean of 8 runs 6.2332  bias 0.0769
Transition  mean of 8 runs 6.2332  bias 0.0769
```

With a variance process that violates the Feller condition badly, the schemes differ widely against the COS price of 6.1563. Euler and the two Milstein schemes are biased by more than 2 (35% or more) because they cannot represent a variance that sits near zero. The quadratic-exponential scheme is far closer (bias 0.077, about 1% of the price, from the finite step and the sampling error of eight runs). `Transition` gives the same result, because for these parameters its exact sampler cannot be driven by the correlated shock, so it uses the quadratic-exponential step.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="options_and_errors.html">
    <span class="label">Previous</span>
    <span class="title">Options and errors</span>
  </a>
  <a class="next-link" href="quasi_random_sequences.html">
    <span class="label">Next</span>
    <span class="title">Quasi-random sequences</span>
  </a>
</div>
<!-- apiref:nav:end -->
