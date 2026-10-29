<!-- generated from prediction_and_intervals.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Prediction and intervals

[Back to Least Squares](../regression_01_least_squares.md)

```cpp
Series reg::ols_predict(const reg::OLSFitResult &fit, const Series &x_new, Config cfg = {});
```

`ols_predict` evaluates the fitted line at new predictor values. It returns one `Float64` value per row of `x_new`. The new data may have the predictors only (`num_predictors` columns), in which case the intercept column is added for you, or the full design including the leading column of ones (`k_params` columns), which is useful when the design was built by hand. A model with no intercept takes the predictors only.

## Predictions

The program predicts at new points in each accepted form and confirms that the prediction is the design matrix times the coefficients. The final two attempts have the wrong number of columns.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 1, 4, 2, 5, 3, 6, 5, 8}, {8, 2}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{3.1, 5.2, 6.8, 9.1, 10.9, 13.2, 14.8, 17.1}, {8}, as_f64());
  const auto fit = reg::ols_fit(y, x);

  const auto new_points = pluss::make_series(std::vector<double>{9, 4, 10, 6}, {2, 2}, as_f64());
  out.writeln("predictors only          ", reg::ols_predict(fit, new_points).pull<double>());
  const auto with_ones = pluss::make_series(std::vector<double>{1, 9, 4, 1, 10, 6}, {2, 3}, as_f64());
  out.writeln("with the column of ones  ", reg::ols_predict(fit, with_ones).pull<double>());
  const auto b = fit.coefficients.pull<double>();
  out.writeln("by hand                  ", std::vector<double>{b[0] + b[1] * 9 + b[2] * 4, b[0] + b[1] * 10 + b[2] * 6});
  out.writeln("one observation as a row ", reg::ols_predict(fit, pluss::make_series(std::vector<double>{9, 4}, {1, 2}, as_f64())).pull<double>(), " and with the 1 in front", reg::ols_predict(fit, pluss::make_series(std::vector<double>{1, 9, 4}, {1, 3}, as_f64())).pull<double>());
  out.writeln("in-sample predictions equal the fitted values:", reg::ols_predict(fit, x).sub(fit.fitted_values).abs().max().item<double>());

  auto attempt = [&](const char *label, const pluss::Series &new_x) {
    try {
      reg::ols_predict(fit, new_x);
    } catch (const std::exception &e) {
      const std::string message = e.what();
      out.writeln(label, "->", message.substr(0, message.find('\n')));
    }
  };
  attempt("one column for a two-predictor fit ", pluss::make_series(std::vector<double>{9, 10}, {2, 1}, as_f64()));
  attempt("four columns for a two-predictor fit", pluss::make_series(std::vector<double>(8, 1.0), {2, 4}, as_f64()));

  // A model without an intercept takes the predictors only.
  reg::OLSOptions origin;
  origin.add_intercept = false;
  const auto through_origin = reg::ols_fit(y, x, origin);
  out.writeln("no intercept, predictors only", reg::ols_predict(through_origin, new_points).pull<double>());
}
```

Output

```text
predictors only           [-16.3739, -10.5450]
with the column of ones   [-16.3739, -10.5450]
by hand                   [-16.3739, -10.5450]
one observation as a row  [-16.3739]  and with the 1 in front [-16.3739]
in-sample predictions equal the fitted values: 0.0000
one column for a two-predictor fit  -> subsp::stats::regression::ols_predict: expected X with either num_predictors columns or k_params columns
four columns for a two-predictor fit -> subsp::stats::regression::ols_predict: expected X with either num_predictors columns or k_params columns
no intercept, predictors only [-12.4594, -7.8264]
```

The predictors-only form and the form with the leading column of ones give the same predictions, and a single new observation can be passed as one row of either width, which equal the hand calculation b₀ + b₁x₁ + b₂x₂, and predicting at the original rows returns the fitted values to rounding. A wrong number of columns is rejected with the same message in both directions. The no-intercept model is a different model, so it predicts differently.

## How uncertain is a prediction?

A point prediction ŷ₀ = x₀′ b has two kinds of uncertainty. The estimated *mean response* at x₀ has variance x₀′ Σ x₀, where Σ is `covariance`. A single *new observation* also carries its own noise, so its variance is `mse` more, x₀′ Σ x₀ + s². Both intervals use the same Student *t* multiplier as the coefficient intervals, which can be read back from the result as the half-width of an interval divided by its standard error.

The mean-response interval widens as x₀ moves away from the centre of the data, the shape of a confidence band, and the interval for a new observation is wider everywhere. The program tabulates both, then repeats the mean-response calculation another way: shifting the predictor so that x₀ becomes zero turns the intercept into the prediction at x₀, so its estimate and standard error must match.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::vector<double> xs{1, 2, 3, 4, 5, 6, 7, 8};
  const std::vector<double> ys{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1};
  const auto x = pluss::make_series(xs, {8}, as_f64());
  const auto y = pluss::make_series(ys, {8}, as_f64());
  const auto fit = reg::ols_fit(y, x);

  const auto cov = fit.covariance.host<double>().array().data;  // 2 x 2, row-major
  const auto b = fit.coefficients.pull<double>();
  const auto ci = fit.conf_int.host<double>().array().data;
  const double multiplier = (ci[3] - ci[2]) / 2.0 / fit.std_errors.pull<double>()[1];
  out.writeln("t multiplier", multiplier, " residual variance", fit.mse);

  out.writeln("  x0   prediction   mean-response interval        new-observation interval");
  for (double x0 : {1.0, 4.5, 8.0, 12.0, 16.0}) {
    const double prediction = b[0] + b[1] * x0;
    const double se_mean = std::sqrt(cov[0] + 2.0 * x0 * cov[1] + x0 * x0 * cov[3]);
    const double se_new = std::sqrt(se_mean * se_mean + fit.mse);
    out.writeln(x0, prediction, "  [", prediction - multiplier * se_mean, ",", prediction + multiplier * se_mean, "]   [", prediction - multiplier * se_new, ",", prediction + multiplier * se_new, "]");
  }

  // Shifting the predictor by x0 makes the intercept the prediction at x0.
  const double x0 = 12.0;
  const auto shifted = reg::ols_fit(y, x.sub(x0));
  out.writeln("re-centred at 12: intercept", shifted.coefficients.pull<double>()[0], " std error", shifted.std_errors.pull<double>()[0]);
  out.writeln("                  interval", shifted.conf_int.host<double>().array().data[0], shifted.conf_int.host<double>().array().data[1]);
}
```

Output

```text
t multiplier 2.4469  residual variance 0.0325
  x0   prediction   mean-response interval        new-observation interval
1.0000 2.0333   [ 1.7488 , 2.3179 ]   [ 1.5086 , 2.5581 ]
4.5000 9.0250   [ 8.8691 , 9.1809 ]   [ 8.5574 , 9.4926 ]
8.0000 16.0167   [ 15.7321 , 16.3012 ]   [ 15.4919 , 16.5414 ]
12.0000 24.0071   [ 23.4737 , 24.5406 ]   [ 23.3151 , 24.6992 ]
16.0000 31.9976   [ 31.2000 , 32.7953 ]   [ 31.0862 , 32.9090 ]
re-centred at 12: intercept 24.0071  std error 0.2180
                  interval 23.4737 24.5406
```

The mean-response interval is narrowest at the centre of the data (x̄ = 4.5, width 0.31) and widens both ways: 0.57 at the ends of the data (x = 1 and 8), 1.07 at x = 12, four units beyond the data, and 1.60 at x = 16, five times the minimum. The interval for a new observation is wider everywhere (0.94 at x̄) because it adds the residual variance 0.0325. Far from the data the uncertainty of the line dominates, so the two intervals move closer in relative terms. Shifting the predictor by 12 makes the intercept the prediction at x = 12: 24.0071, with standard error 0.2180 and interval [23.4737, 24.5406], the row of the table. These intervals describe the sampling error of a straight line that is assumed to continue; they cannot warn that it stops, so a prediction well outside the observed range remains an extrapolation however narrow its band.

