<!-- generated from deviance_and_model_comparison.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Deviance and model comparison

[Back to Generalised Linear Models](../regression_02_generalised_linear_models.md)

The deviance of a GLM is twice the gap between the largest log-likelihood the response could attain and the log-likelihood of the fitted model, so it plays the part that the sum of squared residuals plays in least squares: smaller is better, and it is the quantity compared between models.

| Field | Meaning |
|---|---|
| `deviance` | the deviance of the fitted model |
| `null_deviance` | the deviance of the model with a constant mean (the sample mean of the response) |
| `pseudo_rsquared` | `1 − deviance / null_deviance`, the share of the null deviance removed |
| `log_likelihood` | the log-likelihood at the fit |
| `aic`, `bic` | `−2 log L + 2 r` and `−2 log L + r ln n`, with `r` the rank of the design |

For ungrouped 0/1 data the saturated model fits every observation perfectly, so the deviance is exactly −2 times the log-likelihood.

## Nested models

The program generates 40 observations of a binary outcome whose log-odds depend on two predictors, and fits three nested logistic models: a constant, x₁ alone, and x₁ with x₂. It confirms that the null deviance of any of them is the deviance of the constant-only model, that the deviance of 0/1 data is −2 log L, and that AIC and BIC follow from the log-likelihood and the rank.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<double> x1, x2, outcome, both;
  for (int i = 0; i < 40; ++i) {
    x1.push_back((i % 10) * 0.6);
    x2.push_back(((i * 7) % 13) * 0.4);
    const double eta = -2.0 + 0.8 * x1.back() - 0.5 * x2.back();
    const double u = std::fmod(i * 0.6180339887498949, 1.0);  // a fixed low-discrepancy sequence in place of random draws
    outcome.push_back(u < 1.0 / (1.0 + std::exp(-eta)) ? 1.0 : 0.0);
    both.push_back(x1.back());
    both.push_back(x2.back());
  }
  const auto y = pluss::make_series(outcome, {40}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;

  const auto constant = reg::glm_fit(y, pluss::series::zeros({40, 0}, as_f64()), options);
  const auto with_x1 = reg::glm_fit(y, pluss::make_series(x1, {40}, as_f64()), options);
  const auto with_both = reg::glm_fit(y, pluss::make_series(both, {40, 2}, as_f64()), options);

  out.writeln("model          rank  deviance  null dev  df_resid  pseudo R2  log-lik     AIC      BIC");
  auto row = [&](const char *label, const reg::GLMFitResult &fit) {
    out.writeln(label, fit.rank, fit.deviance, fit.null_deviance, fit.df_resid, fit.pseudo_rsquared, fit.log_likelihood, fit.aic, fit.bic);
  };
  row("constant     ", constant);
  row("x1           ", with_x1);
  row("x1 and x2    ", with_both);

  out.writeln("null deviance of the larger models equals the constant model's deviance:", with_x1.null_deviance, with_both.null_deviance, "vs", constant.deviance);
  out.writeln("deviance + 2 log L (zero for 0/1 data):", with_both.deviance + 2.0 * with_both.log_likelihood);
  out.writeln("AIC by hand", -2.0 * with_both.log_likelihood + 2.0 * 3.0, " BIC by hand", -2.0 * with_both.log_likelihood + 3.0 * std::log(40.0));
}
```

Output

```text
model          rank  deviance  null dev  df_resid  pseudo R2  log-lik     AIC      BIC
constant      1 51.7957 51.7957 39 0.0000 -25.8979 53.7957 55.4846
x1            2 44.9524 51.7957 38 0.1321 -22.4762 48.9524 52.3301
x1 and x2     3 34.8899 51.7957 37 0.3264 -17.4449 40.8899 45.9565
null deviance of the larger models equals the constant model's deviance: 51.7957 51.7957 vs 51.7957
deviance + 2 log L (zero for 0/1 data): -0.0000
AIC by hand 40.8899  BIC by hand 45.9565
```

The constant model's deviance, 51.80, is the null deviance of all three fits. Adding x₁ lowers the deviance to 44.95 (pseudo R² 0.132) and adding x₂ lowers it to 34.89 (pseudo R² 0.326), and AIC and BIC both fall at each step, so both prefer the largest model here. For 0/1 data the deviance plus twice the log-likelihood is zero, and AIC and BIC reproduce from the log-likelihood and the rank of 3.

## The likelihood-ratio test

The drop in deviance from a model to a larger model nested in it is, when the extra terms have no effect, approximately chi-squared with as many degrees of freedom as terms were added. That is the *likelihood-ratio test*. It compares two fitted models directly, and for a single added coefficient it is usually close to, but not identical with, the Wald test given by that coefficient's z statistic. The program tests x₁ against the constant, and x₂ given x₁.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<double> x1, x2, outcome, both;
  for (int i = 0; i < 40; ++i) {
    x1.push_back((i % 10) * 0.6);
    x2.push_back(((i * 7) % 13) * 0.4);
    const double eta = -2.0 + 0.8 * x1.back() - 0.5 * x2.back();
    const double u = std::fmod(i * 0.6180339887498949, 1.0);
    outcome.push_back(u < 1.0 / (1.0 + std::exp(-eta)) ? 1.0 : 0.0);
    both.push_back(x1.back());
    both.push_back(x2.back());
  }
  const auto y = pluss::make_series(outcome, {40}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;
  const auto constant = reg::glm_fit(y, pluss::series::zeros({40, 0}, as_f64()), options);
  const auto with_x1 = reg::glm_fit(y, pluss::make_series(x1, {40}, as_f64()), options);
  const auto with_both = reg::glm_fit(y, pluss::make_series(both, {40, 2}, as_f64()), options);

  auto tail = [&](double statistic, std::int64_t degrees) {
    return 1.0 - distr::ChiSquare(degrees).cdf(pluss::make_series(std::vector<double>{statistic}, {1}, as_f64())).pull<double>()[0];
  };
  const double lr_x1 = constant.deviance - with_x1.deviance;
  const double lr_x2 = with_x1.deviance - with_both.deviance;
  out.writeln("x1 against the constant : LR", lr_x1, " df 1  p", tail(lr_x1, 1), " | Wald z", with_x1.z_values.pull<double>()[1], " p", with_x1.p_values.pull<double>()[1]);
  out.writeln("x2 given x1             : LR", lr_x2, " df 1  p", tail(lr_x2, 1), " | Wald z", with_both.z_values.pull<double>()[2], " p", with_both.p_values.pull<double>()[2]);
  const double lr_both = constant.deviance - with_both.deviance;
  out.writeln("both against the constant: LR", lr_both, " df 2  p", tail(lr_both, 2));
  out.writeln("the LR statistic is also twice the log-likelihood gain:", 2.0 * (with_both.log_likelihood - constant.log_likelihood));
  out.writeln("AIC prefers (lower is better):", constant.aic, with_x1.aic, with_both.aic);
}
```

Output

```text
x1 against the constant : LR 6.8434  df 1  p 0.0089  | Wald z 2.3839  p 0.0171
x2 given x1             : LR 10.0625  df 1  p 0.0015  | Wald z -2.6522  p 0.0080
both against the constant: LR 16.9059  df 2  p 0.0002
the LR statistic is also twice the log-likelihood gain: 16.9059
AIC prefers (lower is better): 53.7957 48.9524 40.8899
```

x₁ against the constant gives a likelihood-ratio statistic of 6.84 (p = 0.0089), against a Wald p-value of 0.0171 for the same coefficient: close, not identical. x₂ given x₁ gives 10.06 (p = 0.0015) against a Wald p-value of 0.0080. Both terms against the constant give 16.91 on 2 degrees of freedom (p = 0.0002). The statistic is also twice the log-likelihood gain, and AIC is lowest for the largest model. Where the two tests disagree materially, the likelihood-ratio test is generally the more reliable in small samples.

The chi-squared reference needs the two models to be fitted to the same rows and the smaller to be a special case of the larger; AIC and BIC need only the same rows and the same family and response.
