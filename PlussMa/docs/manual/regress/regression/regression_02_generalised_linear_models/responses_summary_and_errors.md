<!-- generated from responses_summary_and_errors.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Responses, the summary and errors

[Back to Generalised Linear Models](../regression_02_generalised_linear_models.md)

Shapes, names, dtypes, `add_intercept`, `drop_missing` and `rcond` work as they do for least squares ([Shapes and inputs](../regression_01_least_squares/shapes_and_inputs.md), [Missing data](../regression_01_least_squares/missing_data.md), [Collinear predictors](../regression_01_least_squares/collinear_predictors.md)), and `alpha` sets the coverage of the intervals. This page covers what is specific to a GLM: the accepted range of the response, the summary report and the error conditions.

## The response range

| Family | Accepted response |
|---|---|
| `Normal` | any finite value |
| `Binomial` | every value in [0, 1], including the ends |
| `Poisson` | every value at least 0 |

A binomial response need not be exactly 0 or 1: a proportion such as 0.4 is accepted, and each row counts as one observation of that proportion. The log-likelihood and deviance then use the same formula, so the fit is a quasi-likelihood estimate. The program fits proportions of defective items from batches, and shows a response outside the range and a negative count being refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto temperature = pluss::make_series(std::vector<double>{20, 25, 30, 35, 40, 45, 50, 55}, {8}, as_f64());
  const auto defective = pluss::make_series(std::vector<double>{0.02, 0.03, 0.06, 0.09, 0.20, 0.31, 0.52, 0.71}, {8}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;
  const auto fit = reg::glm_fit(defective, temperature, options);
  out.writeln("coefficients", fit.coefficients.pull<double>(), " converged", fit.converged, " iterations", fit.iterations);
  out.writeln("fitted proportions", fit.fitted_values.pull<double>());
  out.writeln("residuals (y - fitted)", fit.residuals.pull<double>());

  auto attempt = [&](const char *label, reg::GLMFamily family, const std::vector<double> &values) {
    reg::GLMOptions o;
    o.family = family;
    try {
      reg::glm_fit(pluss::make_series(values, {8}, as_f64()), temperature, o);
      out.writeln(label, "-> fitted");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      out.writeln(label, "->", message.substr(0, message.find('\n')));
    }
  };
  attempt("binomial with a response of 1.2 ", reg::GLMFamily::Binomial, {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 1.2});
  attempt("binomial with a response of -0.1", reg::GLMFamily::Binomial, {-0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8});
  attempt("poisson with a count of -1      ", reg::GLMFamily::Poisson, {1, 2, 3, 4, 5, 6, 7, -1});
  attempt("poisson with fractional values  ", reg::GLMFamily::Poisson, {1.5, 2, 3, 4, 5, 6, 7, 8.5});
}
```

Output

```text
coefficients [-7.2876, 0.1471]  converged true  iterations 6
fitted proportions [0.0128, 0.0263, 0.0534, 0.1052, 0.1970, 0.3386, 0.5164, 0.6902]
residuals (y - fitted) [0.0072, 0.0037, 0.0066, -0.0152, 0.0030, -0.0286, 0.0036, 0.0198]
binomial with a response of 1.2  -> subsp::stats::regression::glm_fit: binomial family expects y in [0,1]
binomial with a response of -0.1 -> subsp::stats::regression::glm_fit: binomial family expects y in [0,1]
poisson with a count of -1       -> subsp::stats::regression::glm_fit: poisson family expects y >= 0
poisson with fractional values   -> fitted
```

The proportion of defective items rises with temperature, with a coefficient of 0.1471 per degree, so the odds multiply by e^0.1471 = 1.158 per degree. The fit converges in 6 iterations. The residuals are y minus the fitted proportion. A binomial response above 1 or below 0 is refused, as is a negative Poisson count, while fractional Poisson values are accepted.

The `residuals` field holds the response residuals y − μ, not the Pearson or deviance residuals.

## The summary report

`glm_summary` and `print_glm_summary` write the same fixed layout as the least-squares report. The header gives the family, the link, the method (IRLS), the pseudo R², the iteration count, the convergence flag, both degrees of freedom, the log-likelihood, the deviance and null deviance, AIC and BIC. The table reports *z* statistics and normal p-values and intervals, and the footer the dispersion and the condition number. As in the other reports, the interval heading follows `alpha`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <sstream>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  const auto hours = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto passed = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1, 1, 1}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;
  options.response_name = "passed";
  options.predictor_names = {"hours"};
  options.alpha = 0.10;
  const auto fit = reg::glm_fit(passed, hours, options);
  reg::print_glm_summary(std::cout, fit);
  std::cout << "\nThe string form is identical: " << (reg::glm_summary(fit) == [&] {
    std::ostringstream stream;
    reg::print_glm_summary(stream, fit);
    return stream.str();
  }() ? "yes" : "no") << "\n";
}
```

Output

```text
                            GLM Regression Results                            
==============================================================================
Dep. Variable:                    passed   Family:                         binomial
Model:                               GLM   Link:                              logit
Method:                             IRLS   Pseudo R-squared:                 0.3746
Rank:                                  2   No. Iterations:                        6
Alpha:                               0.1   Converged:                          true
No. Observations:                     10   Df Residuals:                          8
Df Model:                              1   Log-Likelihood:                   -4.335
Deviance:                           8.67   AIC:                               12.67
Null Deviance:                     13.86   BIC:                               13.28
==============================================================================
                        coef     std err           z       P>|z|       [0.05       0.95]
------------------------------------------------------------------------------
const                -3.7219      2.3479     -1.5852      0.1129     -7.5839      0.1401
hours                 1.3534      0.7958      1.7007      0.0890      0.0444      2.6624
==============================================================================
Dispersion:                            1   Cond. No.                          7.261
==============================================================================

The string form is identical: yes
```

The interval columns are headed `[0.05` and `0.95]` because `alpha` is 0.10. The table uses *z* statistics and normal p-values, and the footer shows the dispersion (1) and the condition number. The string form is identical to the stream form.

A fit that did not converge is reported as such, with its warning listed at the end of the report:

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  const auto hours = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto separated = pluss::make_series(std::vector<double>{0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;
  options.max_iter = 25;
  reg::print_glm_summary(std::cout, reg::glm_fit(separated, hours, options));
}
```

Output

```text
                            GLM Regression Results                            
==============================================================================
Dep. Variable:                         y   Family:                         binomial
Model:                               GLM   Link:                              logit
Method:                             IRLS   Pseudo R-squared:                      1
Rank:                                  2   No. Iterations:                       25
Alpha:                              0.05   Converged:                         false
No. Observations:                     10   Df Residuals:                          8
Df Model:                              1   Log-Likelihood:                -2.98e-08
Deviance:                      1.155e-08   AIC:                                   4
Null Deviance:                     13.86   BIC:                               4.605
==============================================================================
                        coef     std err           z       P>|z|      [0.025      0.975]
------------------------------------------------------------------------------
const              -214.6932  56557.0726     -0.0038      0.9970 -1.1106e+05  1.1064e+05
x1                   78.0702  20131.7342      0.0039      0.9969 -39379.4037  39535.5442
==============================================================================
Dispersion:                            1   Cond. No.                          7.261
==============================================================================
Warnings:
[1] IRLS reached max_iter without satisfying tolerance.
```

`Converged: false` and the iteration count equal to `max_iter` mark the failure, and the warning ends the report. The other signs of separation are all here: a pseudo R² of 1, a deviance and log-likelihood of about 10⁻⁸, standard errors of order 10⁴, and *z* statistics near 0.004 with p-values near 1, so the Wald test is useless on such a fit.

## Errors

`glm_fit` and `glm_predict` throw `pluss::Error` with the message shown, preceded by the qualified function name. The conditions shared with least squares are `alpha`, `rcond`, the shape and row checks, the names, the empty model, too few rows, and missing data.

| Condition | Message |
|---|---|
| `alpha` outside (0, 1) | `alpha must be in (0,1)` |
| `rcond`, `tol` not positive; `max_iter` not positive | `rcond must be > 0`, `tol must be > 0`, `max_iter must be > 0` |
| binomial response outside [0, 1] | `binomial family expects y in [0,1]` |
| a logit link with a response outside [0, 1] | `the logit link expects y in [0,1]` |
| a negative weight | `weights must be non-negative` |
| weights or offset of the wrong length | `weights must have one value per row`, `offset must have one value per row` |
| Poisson response below 0 | `poisson family expects y >= 0` |
| shape and row problems, names, empty model, too few rows, missing data | as in [Errors](../regression_01_least_squares/errors.md) |
| `glm_predict` with the wrong number of columns | `expected X with either num_predictors columns or k_params columns` |
| `glm_predict` with an offset of the wrong length | `offset must have one value per row of X` |
| `glm_predict` on a default-constructed result | `fit.coefficients is undefined` |

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 1, 3, 2, 4, 5}, {8}, as_f64());
  reg::GLMOptions poisson;
  poisson.family = reg::GLMFamily::Poisson;
  const auto good = reg::glm_fit(y, x, poisson);

  auto attempt = [&](const char *label, auto &&call) {
    try {
      call();
      out.writeln(label, "-> no error");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      out.writeln(label, "->", message.substr(0, message.find('\n')));
    }
  };
  attempt("alpha = 1", [&] { auto o = poisson; o.alpha = 1.0; reg::glm_fit(y, x, o); });
  attempt("rcond = 0", [&] { auto o = poisson; o.rcond = 0.0; reg::glm_fit(y, x, o); });
  attempt("max_iter = 0", [&] { auto o = poisson; o.max_iter = 0; reg::glm_fit(y, x, o); });
  attempt("tol = 0", [&] { auto o = poisson; o.tol = 0.0; reg::glm_fit(y, x, o); });
  attempt("logit link with counts", [&] { auto o = poisson; o.link = reg::GLMLink::Logit; reg::glm_fit(y, x, o); });
  attempt("negative weight", [&] { auto o = poisson; o.weights = pluss::make_series(std::vector<double>{1, 1, 1, -1, 1, 1, 1, 1}, {8}, as_f64()); reg::glm_fit(y, x, o); });
  attempt("offset of length 3", [&] { auto o = poisson; o.offset = pluss::make_series(std::vector<double>{0, 0, 0}, {3}, as_f64()); reg::glm_fit(y, x, o); });
  attempt("7 rows against 8", [&] { reg::glm_fit(y, pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7}, {7}, as_f64()), poisson); });
  attempt("nothing to fit", [&] { auto o = poisson; o.add_intercept = false; reg::glm_fit(y, pluss::series::zeros({8, 0}, as_f64()), o); });
  attempt("one name too many", [&] { auto o = poisson; o.predictor_names = {"a", "b"}; reg::glm_fit(y, x, o); });
  attempt("predict, wrong columns", [&] { reg::glm_predict(good, pluss::make_series(std::vector<double>(8, 1.0), {2, 4}, as_f64())); });
  attempt("predict, offset of length 1", [&] { reg::glm_predict(good, pluss::make_series(std::vector<double>{1, 2}, {2}, as_f64()), pluss::make_series(std::vector<double>{0.0}, {1}, as_f64())); });
  attempt("predict, empty result", [&] { reg::glm_predict(reg::GLMFitResult{}, x); });
}
```

Output

```text
alpha = 1 -> subsp::stats::regression::glm_fit: alpha must be in (0,1)
rcond = 0 -> subsp::stats::regression::glm_fit: rcond must be > 0
max_iter = 0 -> subsp::stats::regression::glm_fit: max_iter must be > 0
tol = 0 -> subsp::stats::regression::glm_fit: tol must be > 0
logit link with counts -> subsp::stats::regression::glm_fit: the logit link expects y in [0,1]
negative weight -> subsp::stats::regression::glm_fit: weights must be non-negative
offset of length 3 -> subsp::stats::regression::glm_fit: offset must have one value per row
7 rows against 8 -> subsp::stats::regression::glm_fit: row mismatch between y and X
nothing to fit -> subsp::stats::regression::glm_fit: a model needs at least one predictor or an intercept
one name too many -> subsp::stats::regression::glm_fit: predictor_names size must equal number of design columns before intercept handling
predict, wrong columns -> subsp::stats::regression::glm_predict: expected X with either num_predictors columns or k_params columns
predict, offset of length 1 -> subsp::stats::regression::glm_predict: offset must have one value per row of X
predict, empty result -> subsp::stats::regression::glm_predict: fit.coefficients is undefined
```

Each of the thirteen conditions is rejected with the message shown, preceded by the qualified name of the function that raised it.
