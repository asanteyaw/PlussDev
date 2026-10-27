<!-- generated from logistic_and_poisson_regression.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Logistic and Poisson regression

[Back to Generalised Linear, Quantile and Polynomial Regression](../regression_02_glm_quantile_polynomial.md)


```cpp
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
  const auto b = fit.coefficients.host<double>();
  std::cout << "\nlogistic regression of pass on hours\n";
  std::cout << "intercept " << b[0] << ", slope " << b[1] << " (odds multiply by " << std::exp(b[1]) << " per hour)\n";
  std::cout << "deviance " << fit.deviance << " against null deviance " << fit.null_deviance << ", pseudo R-squared " << fit.pseudo_rsquared << ", iterations " << fit.iterations << "\n";
  const auto prob = reg::glm_predict(fit, column({1.0, 2.5, 4.0})).host<double>();
  const auto lin = reg::glm_predict(fit, column({1.0, 2.5, 4.0}), reg::GLMPredictionType::Linear).host<double>();
  // the maximum-likelihood conditions of logistic regression: the residuals y - p are orthogonal to the design columns
  const auto fitted = fit.fitted_values.host<double>();
  double s0 = 0.0, s1 = 0.0;
  for (std::size_t i = 0; i < passed.size(); ++i) { s0 += passed[i] - fitted[i]; s1 += hours[i] * (passed[i] - fitted[i]); }
  std::cout << std::scientific << std::setprecision(1) << "score equations (should be 0): sum(y - p) = " << s0 << ", sum(x (y - p)) = " << s1 << std::fixed << std::setprecision(5) << "\n";
  std::cout << "P(pass) at 1, 2.5, 4 hours: " << prob[0] << ", " << prob[1] << ", " << prob[2] << "\n";
  std::cout << "linear predictor          : " << lin[0] << ", " << lin[1] << ", " << lin[2] << "\n";
}
```

Output

```text
binomial, intercept only: 0.51083  (logit of mean 0.625 = 0.51083), iterations 1, converged 1
poisson, intercept only : 0.69315  (log of mean 2 = 0.69315)

logistic regression of pass on hours
intercept -3.72188, slope 1.35341 (odds multiply by 3.87061 per hour)
deviance 8.67022 against null deviance 13.86294, pseudo R-squared 0.37458, iterations 6
score equations (should be 0): sum(y - p) = 1.6e-15, sum(x (y - p)) = 2.6e-15
P(pass) at 1, 2.5, 4 hours: 0.08561, 0.41621, 0.84446
linear predictor          : -2.36847, -0.33835, 1.69176
```
With only an intercept the maximum-likelihood estimate has a closed form (the logit of the mean for the binomial, the log of the mean for the Poisson), and the fit reproduces it. For the pass/fail data the coefficient on hours, 1.353, is on the log-odds scale: each extra hour multiplies the odds of passing by e^1.353 = 3.87. The *score equations*, the conditions that define the maximum-likelihood estimate (the residuals `y − p` sum to zero and are orthogonal to each predictor), hold to rounding, which is an independent check that IRLS converged to the right point. The deviance has dropped from 13.86 to 8.67, a pseudo R² of 0.37.
