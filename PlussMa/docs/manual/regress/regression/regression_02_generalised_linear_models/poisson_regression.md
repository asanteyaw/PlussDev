<!-- generated from poisson_regression.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Poisson regression

[Back to Generalised Linear Models](../regression_02_generalised_linear_models.md)

Poisson regression models counts: non-negative whole numbers such as events per day. The family is `GLMFamily::Poisson`, whose canonical link is the logarithm, so the mean is μ = exp(b₀ + b₁x₁ + ...), always positive, and its variance equals its mean. A coefficient is a change in the log of the expected count, and exp(b) is a *rate ratio*: the factor by which the expected count is multiplied for a unit increase in the predictor.

The response must be non-negative. It need not be a whole number, but the likelihood is that of a count.

## An intercept-only model

With no predictors the maximum-likelihood estimate of the mean is the sample mean, so the coefficient is its logarithm.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto y = pluss::make_series(std::vector<double>{2, 0, 3, 1, 4, 2}, {6}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, pluss::series::zeros({6, 0}, as_f64()), options);
  out.writeln("coefficient", fit.coefficients.pull<double>(), " log of the mean 2:", std::log(2.0));
  out.writeln("link is the log:", fit.link == reg::GLMLink::Log ? "yes" : "no", " fitted mean", fit.fitted_values.pull<double>()[0]);
}
```

Output

```text
coefficient [0.693147]  log of the mean 2: 0.693147
link is the log: yes  fitted mean 2.000000
```

The coefficient is ln 2 = 0.693147 and every fitted mean is 2.

## Counts and a covariate

Twelve counts rise with a covariate x. The program fits the model and checks the pieces: the score equations Σ(y − μ) = 0 and Σ x(y − μ) = 0, the covariance (X′WX)⁻¹ with weights W = μ, and the deviance 2 Σ [y ln(y/μ) − (y − μ)], in which a zero count contributes 2μ. It then reports the rate ratio and its interval.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
namespace linalg = pluss::subsp::math::linalg;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const std::vector<double> counts{1, 0, 2, 1, 3, 2, 4, 3, 6, 5, 7, 8};
  std::vector<double> xs;
  for (int i = 0; i < 12; ++i) {
    xs.push_back(i);
  }
  const auto y = pluss::make_series(counts, {12}, as_f64());
  const auto x = pluss::make_series(xs, {12}, as_f64());

  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  const auto b = fit.coefficients.pull<double>();
  out.writeln("coefficients", b, " iterations", fit.iterations, " converged", fit.converged);

  const auto mu = fit.fitted_values;
  out.writeln("sum(y - mu)", y.sub(mu).sum().item<double>(), " sum(x (y - mu))", x.mul(y.sub(mu)).sum().item<double>());
  out.writeln("dispersion", fit.dispersion, " (fixed at 1 for the Poisson family)");

  const auto design = pluss::series::ones({12, 1}, as_f64()).unite({x.reshape({12, 1})}, 1);
  const auto information = design.transpose(0, 1).matmul(design.mul(mu.reshape({12, 1})));
  out.writeln("largest |covariance - (X'WX)^-1|", fit.covariance.sub(linalg::inverse(information)).abs().max().item<double>());

  // Deviance: 2 sum[y ln(y/mu) - (y - mu)], with the first term absent for a zero count.
  double deviance = 0.0;
  const auto fitted = mu.pull<double>();
  for (std::size_t i = 0; i < counts.size(); ++i) {
    deviance += 2.0 * ((counts[i] > 0 ? counts[i] * std::log(counts[i] / fitted[i]) : 0.0) - (counts[i] - fitted[i]));
  }
  out.writeln("deviance", fit.deviance, " by hand", deviance);

  const auto ci = fit.conf_int.host<double>().array().data;
  out.writeln("rate ratio per unit of x", std::exp(b[1]), " interval [", std::exp(ci[2]), ",", std::exp(ci[3]), "]");
  out.writeln("expected count at x = 0 and x = 11", std::exp(b[0]), std::exp(b[0] + 11.0 * b[1]));
}
```

Output

```text
coefficients [-0.12403, 0.20636]  iterations 6  converged true
sum(y - mu) 0.00000  sum(x (y - mu)) 0.00000
dispersion 1.00000  (fixed at 1 for the Poisson family)
largest |covariance - (X'WX)^-1| 0.00000
deviance 4.21304  by hand 4.21304
rate ratio per unit of x 1.22920  interval [ 1.11115 , 1.35978 ]
expected count at x = 0 and x = 11 0.88335 8.54995
```

The slope is 0.2064, a rate ratio of 1.2292 per unit of x with an interval of [1.1112, 1.3598] that excludes 1, so the expected count grows by about 23% per unit. The expected count rises from 0.88 at x = 0 to 8.55 at x = 11. The score equations are zero, the covariance is (X′WX)⁻¹ with weights equal to the fitted means, the deviance matches the formula, and `dispersion` is 1.

## Are the counts as variable as a Poisson distribution says?

The Poisson family fixes the variance at the mean, and `dispersion` is held at 1, so the standard errors are only as good as that assumption. Counts that vary more than that (*overdispersion*) make the reported standard errors too small. A check that needs no extra option is the Pearson statistic Σ (y − μ)² / μ divided by `df_resid`: near 1 when the variance matches the mean, well above 1 when it does not. When it is large, multiply the standard errors by its square root (the quasi-Poisson correction). The program computes the ratio for the smooth counts above and for counts of the same size that come in bursts.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<double> xs;
  for (int i = 0; i < 12; ++i) {
    xs.push_back(i);
  }
  const auto x = pluss::make_series(xs, {12}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;

  auto check = [&](const char *label, const std::vector<double> &counts) {
    const auto y = pluss::make_series(counts, {12}, as_f64());
    const auto fit = reg::glm_fit(y, x, options);
    const double pearson = y.sub(fit.fitted_values).pow(2.0).div(fit.fitted_values).sum().item<double>();
    const double ratio = pearson / static_cast<double>(fit.df_resid);
    out.writeln(label, "Pearson/df", ratio, " slope std error", fit.std_errors.pull<double>()[1], " corrected", fit.std_errors.pull<double>()[1] * std::sqrt(std::max(ratio, 1.0)));
  };
  check("smooth counts :", {1, 0, 2, 1, 3, 2, 4, 3, 6, 5, 7, 8});
  check("bursty counts :", {0, 0, 5, 0, 1, 9, 0, 2, 12, 0, 3, 15});
}
```

Output

```text
smooth counts : Pearson/df 0.3237  slope std error 0.0515  corrected 0.0515
bursty counts : Pearson/df 5.3030  slope std error 0.0487  corrected 0.1122
```

The smooth counts have a ratio of 0.32: they vary less than a Poisson distribution would, so the standard errors are, if anything, conservative, and no correction is applied (the program multiplies by the square root of the larger of the ratio and 1). The bursty counts have a ratio of 5.30: their variance is about five times their mean, and the reported standard error of the slope, 0.0487, is too small by a factor of √5.30 = 2.30, whose corrected value is 0.1122. The estimates themselves are unchanged; only the statements of precision are affected.

Counts observed over different exposures are modelled with the log exposure as an `offset` ([Weights and offsets](weights_and_offsets.md)).
