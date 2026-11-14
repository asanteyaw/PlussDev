<!-- generated from variance_reduction.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Variance reduction

[Back to Monte Carlo](../finance_04_monte_carlo.md)

Three controls of `field.controls.mc` reduce the variance of the estimate without changing what it estimates: `antithetic`, `control_variate` and `importance_sampling`. Each should leave the mean of the price unchanged and lower its standard deviation across seeds, and both can be measured the same way: price repeatedly with different seeds, and compare means and standard deviations. The programs do that, with the COS price as the reference.

## Antithetic variates and a control variate

*Antithetic variates* pair each path with its mirror image, shocks Z and −Z, so the pair's errors partly cancel for payoffs that are monotone in the shocks. A *control variate* subtracts a correlated quantity whose mean is known. Here the control is the Black-Scholes discounted payoff driven by the same asset shocks, whose expectation is the Black-Scholes price at the volatility `control_vol` (zero means √θ, the long-run volatility of the Heston model). The estimator subtracts β times the control's deviation from its mean, with β the regression coefficient found from the paths themselves.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  const double reference = cos_reference(100.0);
  out.writeln("COS reference", reference, "  (16 runs of 8000 paths and 32 steps each)");
  auto show = [&](const char *label, auto edit) {
    std::vector<double> prices;
    for (int seed = 1; seed <= 16; ++seed) {
      mod::Field field = monte_carlo(fin::model::Kind::Heston, kHeston);
      field.controls.mc.paths = 8000;
      field.controls.mc.steps = 32;
      field.controls.mc.seed = 40 * seed;
      edit(field.controls.mc);
      prices.push_back(price1(field));
    }
    const auto [mean, sd] = mean_sd(prices);
    out.writeln(label, " mean", mean, " bias", mean - reference, " std dev of one run", sd);
  };
  show("plain                   ", [](auto &) {});
  show("antithetic              ", [](auto &mc) { mc.antithetic = true; });
  show("control variate         ", [](auto &mc) { mc.control_variate = true; });
  show("antithetic + control    ", [](auto &mc) { mc.antithetic = true; mc.control_variate = true; });
  for (double control_vol : {0.1, 0.2, 0.3, 0.6}) {
    show((std::string("control_vol ") + std::to_string(control_vol).substr(0, 3) + "         ").c_str(), [&](auto &mc) { mc.control_variate = true; mc.control_vol = control_vol; });
  }
}
```

Output

```text
COS reference 8.54226   (16 runs of 8000 paths and 32 steps each)
plain                     mean 8.55973  bias 0.01747  std dev of one run 0.14778
antithetic                mean 8.56129  bias 0.01903  std dev of one run 0.07109
control variate           mean 8.55212  bias 0.00987  std dev of one run 0.06418
antithetic + control      mean 8.53352  bias -0.00873  std dev of one run 0.05315
control_vol 0.1           mean 8.55107  bias 0.00881  std dev of one run 0.05477
control_vol 0.2           mean 8.55212  bias 0.00987  std dev of one run 0.06418
control_vol 0.3           mean 8.55362  bias 0.01136  std dev of one run 0.06996
control_vol 0.6           mean 8.55966  bias 0.01740  std dev of one run 0.08723
```

The spread of one run falls from 0.148 for plain simulation to 0.071 with antithetic pairs (a factor 2.1), 0.064 with the control variate (2.3) and 0.053 with both (2.8), and the means stay within the statistical error of the reference. The control's volatility matters: a value of 0.1 gives 0.055, the default (the long-run volatility, 0.2) 0.064, and 0.6 only 0.087, since a control volatility closer to the effective volatility of the option is better correlated with it.

## Importance sampling

*Importance sampling* simulates under a shifted distribution that visits the important region more often, and reweights by the likelihood ratio. For an option far out of the money almost every ordinary path pays nothing, so a shift of the asset shocks that pushes paths toward the strike helps. The shift `importance_shift` is added to every asset shock; the variance shock follows through its correlation. The estimate stays unbiased, since the likelihood ratio exactly undoes the shift. The program prices calls struck at 100, 140 and 160 with shifts from 0 to 0.08 per step.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  for (double strike : {100.0, 140.0, 160.0}) {
    out.writeln("strike", strike, " COS reference", cos_reference(strike));
    for (double shift : {0.0, 0.02, 0.04, 0.08}) {
      std::vector<double> prices;
      for (int seed = 1; seed <= 16; ++seed) {
        mod::Field field = monte_carlo(fin::model::Kind::Heston, kHeston, strike);
        field.controls.mc.paths = 8000;
        field.controls.mc.steps = 32;
        field.controls.mc.seed = 37 * seed;
        field.controls.mc.importance_sampling = shift > 0.0;
        field.controls.mc.importance_shift = shift;
        prices.push_back(price1(field));
      }
      const auto [mean, sd] = mean_sd(prices);
      out.writeln("   shift", shift, " mean", mean, " bias", mean - cos_reference(strike), " std dev of one run", sd, " (standard error of the mean", sd / 4.0, ")");
    }
  }
}
```

Output

```text
strike 100.00000  COS reference 8.54226
   shift 0.00000  mean 8.56294  bias 0.02069  std dev of one run 0.12777  (standard error of the mean 0.03194 )
   shift 0.02000  mean 8.55696  bias 0.01471  std dev of one run 0.11720  (standard error of the mean 0.02930 )
   shift 0.04000  mean 8.55237  bias 0.01012  std dev of one run 0.11035  (standard error of the mean 0.02759 )
   shift 0.08000  mean 8.54379  bias 0.00153  std dev of one run 0.09816  (standard error of the mean 0.02454 )
strike 140.00000  COS reference 0.09910
   shift 0.00000  mean 0.10167  bias 0.00257  std dev of one run 0.01192  (standard error of the mean 0.00298 )
   shift 0.02000  mean 0.10164  bias 0.00254  std dev of one run 0.01048  (standard error of the mean 0.00262 )
   shift 0.04000  mean 0.10191  bias 0.00281  std dev of one run 0.00938  (standard error of the mean 0.00235 )
   shift 0.08000  mean 0.10189  bias 0.00279  std dev of one run 0.00781  (standard error of the mean 0.00195 )
strike 160.00000  COS reference 0.00456
   shift 0.00000  mean 0.00535  bias 0.00078  std dev of one run 0.00253  (standard error of the mean 0.00063 )
   shift 0.02000  mean 0.00523  bias 0.00067  std dev of one run 0.00208  (standard error of the mean 0.00052 )
   shift 0.04000  mean 0.00508  bias 0.00052  std dev of one run 0.00180  (standard error of the mean 0.00045 )
   shift 0.08000  mean 0.00481  bias 0.00025  std dev of one run 0.00132  (standard error of the mean 0.00033 )
```

Importance sampling lowers the spread of one run most for the option furthest from the money: at a shift of 0.08 it falls from 0.128 to 0.098 at strike 100, from 0.0119 to 0.0078 at strike 140 and from 0.00253 to 0.00132 at strike 160, almost half. In every row the mean stays within about one standard error of the reference, so the estimator remains unbiased.

A shift in the wrong direction, or one that is too large, increases the variance: it moves the paths away from where the payoff is, and the likelihood ratios vary wildly. The shift is a per-step shift of the standard normal shocks, so it accumulates over the steps: a shift of 0.08 over 32 steps moves the terminal log price by about 0.08 √32 = 0.45 standard deviations.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="simulated_paths.html">
    <span class="label">Previous</span>
    <span class="title">Simulated paths</span>
  </a>
</div>
<!-- apiref:nav:end -->
