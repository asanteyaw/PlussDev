<!-- generated from glm.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Generalised Linear Models

A generalised linear model says that the mean of a response is a function of a linear predictor, g(μ) = Xβ, and that its variance is a known function of the mean. The family fixes the variance and the likelihood, the link fixes g, and `glm_fit` maximises the likelihood by iteratively reweighted least squares. The family has one entry point for fitting, one for prediction and two for the report, plus the options, the three choices and the result they use.

## Statistical Contract

`GLMFamily` is `Normal`, `Binomial` or `Poisson`, and `GLMLink` is the identity, log or logit link or `Canonical`, which resolves to the family's own link and is recorded as such on the result. The coefficients maximise the likelihood, including any case weights and offset, and the standard errors come from the inverse of the final weighted normal-equation matrix scaled by the dispersion: estimated for `Normal`, fixed at one for the others. Inference is Wald, against the standard normal distribution.

A fit that does not reach its tolerance in `max_iter` steps is returned with `converged` false and a warning, not as an error; check it after any fit with a non-canonical link.

## Practical Examples

The program fits a Poisson model to ten counts, predicts the expected count at a new dose and prints the report.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  options.predictor_names = {"dose"};
  const auto fit = reg::glm_fit(y, x, options);
  const auto dose = pluss::make_series(std::vector<double>{2.0}, {1}, as_f64());
  out.writeln("expected count at dose 2:", reg::glm_predict(fit, dose).items<double>()[0]);
  out.writeln(reg::glm_summary(fit));
}
```

Output

```text
expected count at dose 2: 2.04267
                            GLM Regression Results
==============================================================================
Dep. Variable:                         y   Family:                          poisson
Model:                               GLM   Link:                                log
Method:                             IRLS   Pseudo R-squared:                 0.8912
Rank:                                  2   No. Iterations:                        6
Alpha:                              0.05   Converged:                          true
No. Observations:                     10   Df Residuals:                          8
Df Model:                              1   Log-Likelihood:                   -16.65
Deviance:                          3.857   AIC:                                37.3
Null Deviance:                     35.45   BIC:                               37.91
==============================================================================
                        coef     std err           z       P>|z|      [0.025      0.975]
------------------------------------------------------------------------------
const                -0.5481      0.5064     -1.0822      0.2792     -1.5407      0.4445
dose                  0.6312      0.1253      5.0383  4.6963e-07      0.3856      0.8767
==============================================================================
Dispersion:                            1   Cond. No.                          7.261
==============================================================================
```

## Related Pages

The {guide}`regression guide <regress>` develops logistic and Poisson regression, deviance, separation and model comparison in longer form.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

glm/GLMFamily
glm/GLMLink
glm/GLMPredictionType
glm/GLMOptions
glm/GLMFitResult
glm/glm_fit
glm/glm_predict
glm/glm_summary
glm/print_glm_summary
```
<!-- apiref:toctree:end -->
