<!-- generated from logistic_regression.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Logistic regression

[Back to Generalised Linear Models](../regression_02_generalised_linear_models.md)

A generalised linear model (GLM) says that the mean μ of the response is a function of a linear predictor η = b₀ + b₁x₁ + ... through a *link*, g(μ) = η, and that the variance of the response is a known function of the mean. The coefficients are found by iteratively reweighted least squares (IRLS): each step is a weighted least-squares fit to a working response, repeated until the coefficients stop changing.

```cpp
reg::GLMFitResult reg::glm_fit(const Series &y, const Series &x, const reg::GLMOptions &options = {}, Config cfg = {});
Series            reg::glm_predict(const reg::GLMFitResult &fit, const Series &x_new,
                                   reg::GLMPredictionType type = reg::GLMPredictionType::Mean, Config cfg = {});
```

Inputs are read exactly as for least squares ([Shapes and inputs](../regression_01_least_squares/shapes_and_inputs.md)). *Logistic regression* is the binomial family with its canonical link, the logit: μ is the probability of success, and η = ln(μ / (1 − μ)) is the log-odds.

## An intercept-only model

With no predictors the maximum-likelihood estimate of a probability is the fraction of successes, so the fitted coefficient must be the log-odds of that fraction. With eight pass/fail results of which five passed, the fraction is 0.625 and the coefficient is exactly ln(0.625 / 0.375).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;
  const auto none = pluss::series::zeros({8, 0}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{0, 1, 1, 0, 1, 1, 0, 1}, {8}, as_f64());
  const auto fit = reg::glm_fit(y, none, options);
  out.writeln("coefficient", fit.coefficients.pull<double>(), " logit of the mean 0.625:", std::log(0.625 / 0.375));
  out.writeln("iterations", fit.iterations, " converged", fit.converged, " fitted probability", fit.fitted_values.pull<double>()[0]);
}
```

Output

```text
coefficient [0.510826]  logit of the mean 0.625: 0.510826
iterations 1  converged true  fitted probability 0.625000
```

The coefficient is exactly the log-odds of the fraction, 0.510826, and the fitted probability is 0.625 for every row. IRLS reports one iteration: the starting value of a model with an intercept is already this log-odds, and the first step only confirms it.

## Hours of study and passing

Ten students, the hours each studied and whether they passed. The program fits the logistic model and then checks it four ways. The *score equations*, the conditions that define the maximum-likelihood estimate, say that the residuals y − p sum to zero and are orthogonal to every column of the design. The *Wald* statistic z = b / se has a p-value from the standard normal distribution. The covariance of the coefficients is the inverse of the Fisher information X′WX, where the weights are W = p(1 − p). And for 0/1 data the deviance is −2 Σ [y ln p + (1 − y) ln(1 − p)].

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
namespace linalg = pluss::subsp::math::linalg;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto hours = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto passed = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1, 1, 1}, {10}, as_f64());

  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;
  options.response_name = "passed";
  options.predictor_names = {"hours"};
  const auto fit = reg::glm_fit(passed, hours, options);
  const auto b = fit.coefficients.pull<double>();
  out.writeln("coefficients", b, " names", fit.coefficient_names);
  out.writeln("iterations", fit.iterations, " converged", fit.converged, " link is the logit:", fit.link == reg::GLMLink::Logit ? "yes" : "no");

  // The score equations.
  const auto p = fit.fitted_values;
  const auto residual = passed.sub(p);
  out.writeln("sum(y - p)", residual.sum().item<double>(), " sum(hours (y - p))", hours.mul(residual).sum().item<double>());

  // Wald statistics and p-values from the standard normal distribution.
  const distr::Normal standard(0.0, 1.0);
  out.writeln("z values", fit.z_values.pull<double>(), " by hand", fit.coefficients.div(fit.std_errors).pull<double>());
  out.writeln("p values", fit.p_values.pull<double>(), " by hand", standard.cdf(fit.z_values.abs()).neg().add(1.0).mul(2.0).pull<double>());

  // The covariance is the inverse of the Fisher information X'WX.
  const auto design = pluss::series::ones({10, 1}, as_f64()).unite({hours.reshape({10, 1})}, 1);
  const auto weights = p.mul(p.neg().add(1.0)).reshape({10, 1});
  const auto information = design.transpose(0, 1).matmul(design.mul(weights));
  out.writeln("largest |covariance - (X'WX)^-1|", fit.covariance.sub(linalg::inverse(information)).abs().max().item<double>());

  // The deviance of 0/1 data, and the null deviance of the constant-probability model.
  const double deviance = -2.0 * passed.mul(p.log()).add(passed.neg().add(1.0).mul(p.neg().add(1.0).log())).sum().item<double>();
  const double mean = passed.mean().item<double>();
  const double null_deviance = -2.0 * 10.0 * (mean * std::log(mean) + (1.0 - mean) * std::log(1.0 - mean));
  out.writeln("deviance", fit.deviance, " by hand", deviance);
  out.writeln("null deviance", fit.null_deviance, " by hand", null_deviance, " pseudo R2", fit.pseudo_rsquared, " by hand", 1.0 - deviance / null_deviance);
  out.writeln("df_model", fit.df_model, " df_resid", fit.df_resid, " dispersion", fit.dispersion);
}
```

Output

```text
coefficients [-3.72188, 1.35341]  names [const, hours]
iterations 6  converged true  link is the logit: yes
sum(y - p) 0.00000  sum(hours (y - p)) 0.00000
z values [-1.58517, 1.70067]  by hand [-1.58517, 1.70067]
p values [0.11293, 0.08900]  by hand [0.11293, 0.08900]
largest |covariance - (X'WX)^-1| 0.00000
deviance 8.67022  by hand 8.67022
null deviance 13.86294  by hand 13.86294  pseudo R2 0.37458  by hand 0.37458
df_model 1  df_resid 8  dispersion 1.00000
```

Every check holds. The score equations are zero, the *z* statistics and p-values agree with the normal distribution, the covariance is the inverse Fisher information, and the deviances, 8.67 for the model and 13.86 for the constant-probability model, match the formula, giving a pseudo R² of 0.3746. The model has `df_model` 1 and `df_resid` 8 (ten rows, rank 2), and `dispersion` is 1, fixed for the binomial family. The evidence for an effect of study time is modest: the slope has p = 0.089 and the intercept p = 0.113, so with ten students the data lean toward an effect of hours without establishing one at the 5% level.

## Odds ratios

A coefficient of a logistic model is a change in log-odds, so exp(b) is a multiplier of the odds, an *odds ratio*. The limits of an interval for it are the exponentials of the limits of the interval for b. The intervals of a GLM use the normal distribution, so their multiplier is the standard normal quantile (1.96 at `alpha = 0.05`), not a *t* quantile.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto hours = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto passed = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1, 1, 1}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;
  const auto fit = reg::glm_fit(passed, hours, options);

  const auto b = fit.coefficients.pull<double>();
  const auto se = fit.std_errors.pull<double>();
  const auto ci = fit.conf_int.host<double>().array().data;
  out.writeln("interval of the slope", ci[2], ci[3], " half-width / std err", (ci[3] - ci[2]) / 2.0 / se[1], "(the 97.5% normal quantile is 1.9600)");
  out.writeln("odds ratio per hour    ", std::exp(b[1]), " interval [", std::exp(ci[2]), ",", std::exp(ci[3]), "]");
  out.writeln("odds ratio per 2 hours ", std::exp(2.0 * b[1]), " interval [", std::exp(2.0 * ci[2]), ",", std::exp(2.0 * ci[3]), "]");
  out.writeln("the interval for the odds ratio excludes 1:", (std::exp(ci[2]) > 1.0 || std::exp(ci[3]) < 1.0) ? "yes" : "no", " (p value of the slope", fit.p_values.pull<double>()[1], ")");
}
```

Output

```text
interval of the slope -0.2063 2.9132  half-width / std err 1.9600 (the 97.5% normal quantile is 1.9600)
odds ratio per hour     3.8706  interval [ 0.8136 , 18.4151 ]
odds ratio per 2 hours  14.9816  interval [ 0.6619 , 339.1152 ]
the interval for the odds ratio excludes 1: no  (p value of the slope 0.0890 )
```

The interval for the slope, [−0.2063, 2.9132], extends 1.96 standard errors either side of the estimate. Exponentiated, the odds of passing are multiplied by 3.87 per extra hour, with an interval of [0.81, 18.42] that includes 1, which agrees with the p-value. Over two hours the odds ratio is 14.98 with an interval of [0.66, 339.1]: intervals for odds ratios are asymmetric about the estimate and widen quickly as the change in the predictor grows, because the exponential stretches the upper limit.

## Predicted probabilities

`glm_predict` returns either the mean (`GLMPredictionType::Mean`, the default: a probability for the binomial family) or the linear predictor (`GLMPredictionType::Linear`, the log-odds). The mean is the inverse link applied to the linear predictor, here 1 / (1 + e^−η). The half-way point, where the probability is 0.5, is where the linear predictor is zero, at −b₀ / b₁ hours.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto hours = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto passed = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1, 1, 1}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;
  const auto fit = reg::glm_fit(passed, hours, options);

  const auto study = pluss::make_series(std::vector<double>{1.0, 2.5, 4.0, 8.0, 0.0}, {5}, as_f64());
  const auto probability = reg::glm_predict(fit, study);
  const auto log_odds = reg::glm_predict(fit, study, reg::GLMPredictionType::Linear);
  out.writeln("hours       ", study.pull<double>());
  out.writeln("probability ", probability.pull<double>());
  out.writeln("log-odds    ", log_odds.pull<double>());
  out.writeln("1/(1+exp(-log-odds))", log_odds.neg().exp().add(1.0).reciprocal().pull<double>());

  const auto b = fit.coefficients.pull<double>();
  const double even = -b[0] / b[1];
  out.writeln("hours at which the probability is one half", even, " predicted there", reg::glm_predict(fit, pluss::make_series(std::vector<double>{even}, {1}, as_f64())).pull<double>());
  out.writeln("the fitted values are the in-sample means:", reg::glm_predict(fit, hours).sub(fit.fitted_values).abs().max().item<double>());
}
```

Output

```text
hours        [1.00000, 2.50000, 4.00000, 8.00000, 0.00000]
probability  [0.08561, 0.41621, 0.84446, 0.99918, 0.02362]
log-odds     [-2.36847, -0.33835, 1.69176, 7.10541, -3.72188]
1/(1+exp(-log-odds)) [0.08561, 0.41621, 0.84446, 0.99918, 0.02362]
hours at which the probability is one half 2.75000  predicted there [0.50000]
the fitted values are the in-sample means: 0.00000
```

The probability of passing is 0.0856 at 1 hour, 0.4162 at 2.5 hours and 0.8445 at 4 hours. At 0 hours it is 0.0236, the logistic function of the intercept alone, and at 8 hours, beyond the data, 0.9992. The mean is the logistic function of the log-odds, as the third line confirms. The probability is one half where the log-odds is zero, at 2.75 hours, and predicting there returns 0.5. Predicting at the original rows returns the fitted values.
