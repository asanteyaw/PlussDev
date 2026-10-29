<!-- generated from comparing_two_event_rates.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Comparing two event rates

[Back to Worked Problems](../regression_05_worked_problems.md)

A process is observed for twelve equal periods under a control condition and twelve under a treatment, and the number of events in each period is counted. Are events more frequent under the treatment, and by how much? With equal observation periods, the counts can be compared directly, and a Poisson regression on a group indicator answers both questions: the rate ratio is exp(b₁), and the coefficient's test is the test of equal rates.

The counts are generated as Poisson draws with means 4 (control) and 6.5 (treatment), by inverting the Poisson distribution function at a fixed low-discrepancy sequence.

## The rate ratio and its tests

With a single indicator the maximum-likelihood fitted mean of each group is its sample mean, so exp(b₁) must be the ratio of the group means exactly. The program checks that, and then compares three tests of the hypothesis of no difference: the Wald test from the coefficient, the likelihood-ratio test from the drop in deviance against the intercept-only model, and the quasi-likelihood version of the Wald test that scales the standard error by the dispersion estimated from the Pearson statistic.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // Poisson draws by inverting the distribution function at a fixed low-discrepancy sequence.
  auto poisson_draw = [](double mean, double u) {
    double term = std::exp(-mean), cumulative = term;
    int k = 0;
    while (cumulative < u && k < 100) {
      ++k;
      term *= mean / k;
      cumulative += term;
    }
    return static_cast<double>(k);
  };
  std::vector<double> group, events;
  for (int i = 0; i < 24; ++i) {
    const bool treated = i >= 12;
    group.push_back(treated ? 1.0 : 0.0);
    events.push_back(poisson_draw(treated ? 6.5 : 4.0, std::fmod((i + 1) * 0.6180339887498949, 1.0)));
  }
  const auto x = pluss::make_series(group, {24}, as_f64());
  const auto y = pluss::make_series(events, {24}, as_f64());

  double control = 0.0, treated = 0.0;
  for (int i = 0; i < 24; ++i) {
    (i < 12 ? control : treated) += events[i] / 12.0;
  }
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  options.predictor_names = {"treated"};
  const auto fit = reg::glm_fit(y, x, options);
  const auto b = fit.coefficients.pull<double>();
  const auto ci = fit.conf_int.host<double>().array().data;

  out.writeln("mean events per period: control", control, " treated", treated, " ratio", treated / control);
  out.writeln("rate ratio exp(b1)    :", std::exp(b[1]), " interval [", std::exp(ci[2]), ",", std::exp(ci[3]), "]   fitted means", std::exp(b[0]), std::exp(b[0] + b[1]));

  // Three tests of "no difference".
  auto null_options = options;
  null_options.predictor_names.clear();  // the null model has no predictor to name
  const auto null_fit = reg::glm_fit(y, pluss::series::zeros({24, 0}, as_f64()), null_options);
  const double lr = null_fit.deviance - fit.deviance;
  const distr::ChiSquare chi(1);
  const double lr_p = 1.0 - chi.cdf(pluss::make_series(std::vector<double>{lr}, {1}, as_f64())).pull<double>()[0];
  const double pearson = y.sub(fit.fitted_values).pow(2.0).div(fit.fitted_values).sum().item<double>();
  const double dispersion = pearson / static_cast<double>(fit.df_resid);
  const distr::Normal standard(0.0, 1.0);
  const double z_quasi = fit.z_values.pull<double>()[1] / std::sqrt(std::max(dispersion, 1.0));
  const double quasi_p = 2.0 * (1.0 - standard.cdf(pluss::make_series(std::vector<double>{std::abs(z_quasi)}, {1}, as_f64())).pull<double>()[0]);
  out.writeln("Wald             z =", fit.z_values.pull<double>()[1], " p =", fit.p_values.pull<double>()[1]);
  out.writeln("likelihood ratio chi2 =", lr, " p =", lr_p);
  out.writeln("Pearson dispersion", dispersion, " quasi-Wald z =", z_quasi, " p =", quasi_p);
}
```

Output

```text
mean events per period: control 3.9167  treated 6.5833  ratio 1.6809
rate ratio exp(b1)    : 1.6809  interval [ 1.1715 , 2.4118 ]   fitted means 3.9167 6.5833
Wald             z = 2.8190  p = 0.0048
likelihood ratio chi2 = 8.2167  p = 0.0042
Pearson dispersion 1.0424  quasi-Wald z = 2.7611  p = 0.0058
```

The rate ratio, 1.6809, is the ratio of the group means exactly (6.5833 over 3.9167), and the fitted means of the two groups are those sample means. Its interval, [1.17, 2.41], excludes 1. The three tests of no difference agree: Wald z = 2.82 (p = 0.0048), likelihood ratio χ² = 8.22 (p = 0.0042) and the quasi-likelihood Wald test (z = 2.76, p = 0.0058). The Pearson dispersion is 1.04, close to 1, so the counts vary about as much as a Poisson distribution says and the plain Poisson tests can be trusted here.

## Reading the answer

Whether the dispersion matters is read from the Pearson ratio: counts that vary more than a Poisson distribution allows make the plain Wald and likelihood-ratio tests too eager, and the corrected test is the honest one ([Poisson regression](../regression_02_generalised_linear_models/poisson_regression.md)).

The summary report of the fit puts these quantities in one place:

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  auto poisson_draw = [](double mean, double u) {
    double term = std::exp(-mean), cumulative = term;
    int k = 0;
    while (cumulative < u && k < 100) {
      ++k;
      term *= mean / k;
      cumulative += term;
    }
    return static_cast<double>(k);
  };
  std::vector<double> group, events;
  for (int i = 0; i < 24; ++i) {
    const bool treated = i >= 12;
    group.push_back(treated ? 1.0 : 0.0);
    events.push_back(poisson_draw(treated ? 6.5 : 4.0, std::fmod((i + 1) * 0.6180339887498949, 1.0)));
  }
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  options.response_name = "events";
  options.predictor_names = {"treated"};
  reg::print_glm_summary(std::cout, reg::glm_fit(pluss::make_series(events, {24}, as_f64()), pluss::make_series(group, {24}, as_f64()), options));
}
```

Output

```text
                            GLM Regression Results                            
==============================================================================
Dep. Variable:                    events   Family:                          poisson
Model:                               GLM   Link:                                log
Method:                             IRLS   Pseudo R-squared:                 0.2559
Rank:                                  2   No. Iterations:                        5
Alpha:                              0.05   Converged:                          true
No. Observations:                     24   Df Residuals:                         22
Df Model:                              1   Log-Likelihood:                   -52.68
Deviance:                          23.89   AIC:                               109.4
Null Deviance:                      32.1   BIC:                               111.7
==============================================================================
                        coef     std err           z       P>|z|      [0.025      0.975]
------------------------------------------------------------------------------
const                 1.3652      0.1459      9.3596  8.0024e-21      1.0794      1.6511
treated               0.5193      0.1842      2.8190      0.0048      0.1582      0.8804
==============================================================================
Dispersion:                            1   Cond. No.                          2.618
==============================================================================
```

The report holds the same numbers: the `treated` coefficient is 0.5193 (standard error 0.1842, z = 2.82, p = 0.0048), the null deviance of 32.1 less the deviance of 23.89 is the likelihood-ratio statistic of 8.22, and the pseudo R² is 0.2559.
