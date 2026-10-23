// Generalised linear models. `reg::glm_fit(y, x, options)` fits a model whose mean is a function of the linear predictor, by iteratively
// reweighted least squares (IRLS). `GLMOptions::family` is Normal, Binomial (y in [0, 1], logistic regression) or Poisson (counts); `link` is
// Canonical (the natural one for the family: identity, logit, log) or a named link; `max_iter` and `tol` control IRLS. The result adds the
// deviance, null deviance, pseudo R-squared, `converged`, and the dispersion. Predictions come as the mean or as the linear predictor
// (`GLMPredictionType`). Here: a logistic regression of pass/fail on study hours, and a Poisson regression of counts on a covariate;
// with only an intercept, both have closed-form maximum-likelihood estimates (logit of the mean, log of the mean) to check against.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

namespace reg = pluss::subsp::stats::regression;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series column(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size()), 1}).to(f64); }
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
Series table(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }
int main() {
  std::cout << std::fixed << std::setprecision(5);
  {
    const std::vector<double> y{0, 1, 1, 0, 1, 1, 0, 1};
    reg::GLMOptions o;
    o.family = reg::GLMFamily::Binomial;
    const auto fit = reg::glm_fit(vec(y), Series::zeros({8, 0}, f64), o);
    std::cout << "binomial, intercept only: " << fit.coefficients.item<double>() << "  (logit of mean 0.625 = " << std::log(0.625 / 0.375) << "), iterations " << fit.iterations
              << ", converged " << fit.converged << "\n";
  }
  {
    const std::vector<double> y{2, 0, 3, 1, 4, 2};
    reg::GLMOptions o;
    o.family = reg::GLMFamily::Poisson;
    const auto fit = reg::glm_fit(vec(y), Series::zeros({6, 0}, f64), o);
    std::cout << "poisson, intercept only : " << fit.coefficients.item<double>() << "  (log of mean 2 = " << std::log(2.0) << ")\n";
  }
  const std::vector<double> hours{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0};
  const std::vector<double> passed{0, 0, 0, 1, 0, 1, 0, 1, 1, 1};
  reg::GLMOptions o;
  o.family = reg::GLMFamily::Binomial;
  o.response_name = "passed";
  o.predictor_names = {"hours"};
  const auto fit = reg::glm_fit(vec(passed), column(hours), o);
  const auto b = fit.coefficients.to_vec<double>();
  std::cout << "\nlogistic regression of pass on hours\n";
  std::cout << "intercept " << b[0] << ", slope " << b[1] << " (odds multiply by " << std::exp(b[1]) << " per hour)\n";
  std::cout << "deviance " << fit.deviance << " against null deviance " << fit.null_deviance << ", pseudo R-squared " << fit.pseudo_rsquared << ", iterations " << fit.iterations << "\n";
  const auto prob = reg::glm_predict(fit, column({1.0, 2.5, 4.0})).to_vec<double>();
  const auto lin = reg::glm_predict(fit, column({1.0, 2.5, 4.0}), reg::GLMPredictionType::Linear).to_vec<double>();
  // the maximum-likelihood conditions of logistic regression: the residuals y - p are orthogonal to the design columns
  const auto fitted = fit.fitted_values.to_vec<double>();
  double s0 = 0.0, s1 = 0.0;
  for (std::size_t i = 0; i < passed.size(); ++i) { s0 += passed[i] - fitted[i]; s1 += hours[i] * (passed[i] - fitted[i]); }
  std::cout << std::scientific << std::setprecision(1) << "score equations (should be 0): sum(y - p) = " << s0 << ", sum(x (y - p)) = " << s1 << std::fixed << std::setprecision(5) << "\n";
  std::cout << "P(pass) at 1, 2.5, 4 hours: " << prob[0] << ", " << prob[1] << ", " << prob[2] << "\n";
  std::cout << "linear predictor          : " << lin[0] << ", " << lin[1] << ", " << lin[2] << "\n";
}
