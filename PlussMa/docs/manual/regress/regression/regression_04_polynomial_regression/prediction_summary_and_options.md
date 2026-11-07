<!-- generated from prediction_summary_and_options.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Prediction, the summary and options

[Back to Polynomial Regression](../regression_04_polynomial_regression.md)

```cpp
struct reg::PolyOptions {
  std::int64_t degree = 2;
  bool add_intercept = true;
  bool include_cross = false;
  bool drop_missing = true;
  double rcond = 1e-12;
  double alpha = 0.05;
  std::string response_name = "y";
  std::vector<std::string> predictor_names{};
};
```

`degree` and `include_cross` define the design ([The expanded design](the_expanded_design.md)). The other options are passed to the underlying least-squares fit and behave as there: `alpha` sets the interval coverage, `add_intercept` adds the constant, `drop_missing` removes incomplete rows or makes them an error, `rcond` is the rank cutoff, and the names label the output. `predictor_names` gives the names of the *original* predictors, one per column of `x`; the names of the expanded terms are built from them.

## Prediction

`poly_predict(fit, x_new)` expands `x_new` with the degree, the cross-term setting and the predictor names the fit used, and evaluates the fitted curve. `x_new` must have the same number of columns as the original `x`; a different number of columns is an error that names the number expected. Inside the range of the data the curve follows the data. Outside it, a polynomial runs away quickly and the higher the degree, the faster.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  std::vector<double> xs, ys;
  for (int i = 0; i < 24; ++i) {
    const double x = i * 0.25;
    xs.push_back(x);
    ys.push_back(1.0 - 2.0 * x + 0.5 * x * x + 0.6 * (2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0));
  }
  const auto x = pluss::make_series(xs, {24}, as_f64());
  const auto y = pluss::make_series(ys, {24}, as_f64());
  const auto new_x = pluss::make_series(std::vector<double>{0.5, 3.0, 5.0, 7.0, 10.0}, {5}, as_f64());

  out.writeln("data cover x from 0 to 5.75; new x    ", new_x.pull<double>());
  out.writeln("true curve                           ", new_x.pow(2.0).mul(0.5).sub(new_x.mul(2.0)).add(1.0).pull<double>());
  for (std::int64_t degree : {2, 3, 6}) {
    reg::PolyOptions options;
    options.degree = degree;
    const auto fit = reg::poly_fit(y, x, options);
    out.writeln("degree", degree, " prediction                   ", reg::poly_predict(fit, new_x).pull<double>());
  }

  reg::PolyOptions options;
  const auto fit = reg::poly_fit(y, x, options);
  const auto b = fit.ols.coefficients.pull<double>();
  out.writeln("by hand at x = 3 (degree 2)           ", b[0] + b[1] * 3.0 + b[2] * 9.0);
  out.writeln("in-sample predictions equal the fitted values:", reg::poly_predict(fit, x).sub(fit.ols.fitted_values).abs().max().item<double>());
}
```

Output

```text
data cover x from 0 to 5.75; new x     [0.500, 3.000, 5.000, 7.000, 10.000]
true curve                            [0.125, -0.500, 3.500, 11.500, 31.000]
degree 2  prediction                    [0.004, -0.475, 3.512, 11.384, 30.474]
degree 3  prediction                    [-0.003, -0.482, 3.494, 11.873, 33.770]
degree 6  prediction                    [0.150, -0.499, 3.707, -6.262, -729.510]
by hand at x = 3 (degree 2)            -0.475
in-sample predictions equal the fitted values: 0.000
```

Inside the data (x = 0.5 and 3) all three degrees follow the true curve closely. At x = 5, still inside, they are within 0.21 of 3.5. Beyond the data (x = 7 and 10) the quadratic stays near the true values (11.38 and 30.47 against 11.5 and 31.0), the cubic drifts (11.87 and 33.77), and the degree-6 curve is wrong in sign and size (−6.26 and −729.5). The hand calculation at x = 3 agrees, and predictions at the original rows return the fitted values. The data say nothing about a region they do not cover, and a high-degree polynomial is least reliable exactly there.

## The summary

`poly_summary` and `print_poly_summary` write a one-line heading with the degree and whether cross terms were used, then the least-squares report for the expanded fit. The coefficient names are the term names.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  std::vector<double> flat, ys;
  for (int i = 0; i < 25; ++i) {
    const double a = (i % 5) * 0.5, b = (i / 5) * 0.4;
    flat.push_back(a);
    flat.push_back(b);
    ys.push_back(1.0 + a + 2.0 * b + 0.5 * a * b + 0.05 * (2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0));
  }
  reg::PolyOptions options;
  options.degree = 2;
  options.include_cross = true;
  options.response_name = "yield";
  options.predictor_names = {"water", "light"};
  options.alpha = 0.10;
  const auto fit = reg::poly_fit(pluss::make_series(ys, {25}, as_f64()), pluss::make_series(flat, {25, 2}, as_f64()), options);
  reg::print_poly_summary(std::cout, fit);
}
```

Output

```text
Polynomial Regression (degree=2, cross=true)
                            OLS Regression Results                            
==============================================================================
Dep. Variable:                     yield   R-squared:                        0.9998
Model:                               OLS   Adj. R-squared:                   0.9997
Method:                    Least Squares   F-statistic:                   1.591e+04
Rank:                                  6   Prob (F-statistic):             1.032e-33
Alpha:                               0.1   Log-Likelihood:                    54.85
No. Observations:                     25   AIC:                               -97.7
Df Residuals:                         19   BIC:                              -90.39
Df Model:                              5   Covariance Type:               nonrobust
==============================================================================
                        coef     std err           t       P>|t|       [0.05       0.95]
------------------------------------------------------------------------------
const                 0.9671      0.0213     45.3923  7.6963e-21      0.9303      1.0040
water                 1.0465      0.0332     31.4832  7.3603e-18      0.9890      1.1039
water^2              -0.0114      0.0148     -0.7727      0.4492     -0.0370      0.0141
light                 2.0083      0.0415     48.3353  2.3559e-21      1.9364      2.0801
light^2               0.0089      0.0231      0.3863      0.7035     -0.0310      0.0489
water*light           0.4850      0.0155     31.3533  7.9506e-18      0.4583      0.5117
==============================================================================
Omnibus:                           2.803   Durbin-Watson:                      2.86
Prob(Omnibus):                    0.2463   Jarque-Bera (JB):                  1.287
Skew:                           -0.07845   Prob(JB):                         0.5253
Kurtosis:                          1.899   Cond. No.                          23.94
==============================================================================
```

The heading gives the degree and whether cross terms are used. The cross term `water*light` has an estimate of 0.4850 against the generating value 0.5 and a *t* statistic of 31.4, and the squared terms, which the generating curve does not contain, are small and not significant (p = 0.45 and 0.70). The two predictors keep their names in the terms, and the intervals are headed `[0.05` and `0.95]` because `alpha` is 0.10.

## Options and data handling

The remaining options in action: no intercept (the curve is forced through the origin, the term names lose `const`), incomplete rows, and the effect of `include_cross` when the degree is 1.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double nan = std::numeric_limits<double>::quiet_NaN();
  std::vector<double> xs, ys;
  for (int i = 0; i < 12; ++i) {
    xs.push_back(i * 0.5);
    ys.push_back(0.4 * xs.back() + 0.3 * xs.back() * xs.back() + 0.05 * (2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0));
  }
  const auto x = pluss::make_series(xs, {12}, as_f64());
  const auto y = pluss::make_series(ys, {12}, as_f64());

  reg::PolyOptions origin;
  origin.add_intercept = false;
  const auto through_origin = reg::poly_fit(y, x, origin);
  out.writeln("through the origin: names", through_origin.ols.coefficient_names, " coefficients", through_origin.ols.coefficients.pull<double>(), " k_params", through_origin.ols.k_params);

  auto with_gaps = ys;
  with_gaps[4] = nan;
  auto x_gaps = xs;
  x_gaps[9] = nan;
  const auto gaps = reg::poly_fit(pluss::make_series(with_gaps, {12}, as_f64()), pluss::make_series(x_gaps, {12}, as_f64()));
  out.writeln("two incomplete rows: nobs", gaps.ols.nobs, " dropped", gaps.ols.dropped_observations);

  reg::PolyOptions crossed;
  crossed.degree = 1;
  crossed.include_cross = true;
  const auto linear = reg::poly_fit(y, x, crossed);
  out.writeln("degree 1 with include_cross: names", linear.ols.coefficient_names, " fit.include_cross", linear.include_cross);
}
```

Output

```text
through the origin: names [x1, x1^2]  coefficients [0.3978, 0.3008]  k_params 2
two incomplete rows: nobs 10  dropped 2
degree 1 with include_cross: names [const, x1]  fit.include_cross true
```

Without an intercept the terms are `x1` and `x1^2` and there are two parameters. Two incomplete rows leave 10 observations. At degree 1, `include_cross` is recorded in the result but adds no terms.

## Errors

`poly_fit` and `poly_expand` reject a `degree` below 1 (`degree must be >= 1`), and `poly_fit` raises every least-squares error ([Errors](../regression_01_least_squares/errors.md)) under its own name, including too few rows for the number of expanded columns. `poly_predict` rejects an `x_new` whose number of columns differs from the original, and a default-constructed result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1}, {8}, as_f64());
  const auto fit = reg::poly_fit(y, x);

  auto attempt = [&](const char *label, auto &&call) {
    try {
      call();
      out.writeln(label, "-> no error");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      out.writeln(label, "->", message.substr(0, message.find('\n')));
    }
  };
  attempt("degree 0 in poly_fit    ", [&] { reg::PolyOptions o; o.degree = 0; reg::poly_fit(y, x, o); });
  attempt("degree 0 in poly_expand ", [&] { reg::poly_expand(x, 0); });
  attempt("degree 8 on 8 rows      ", [&] { reg::PolyOptions o; o.degree = 8; reg::poly_fit(y, x, o); });
  attempt("two names, one predictor", [&] { reg::PolyOptions o; o.predictor_names = {"a", "b"}; reg::poly_fit(y, x, o); });
  attempt("predict, two columns    ", [&] { reg::poly_predict(fit, pluss::make_series(std::vector<double>{1, 2, 3, 4}, {2, 2}, as_f64())); });
  attempt("predict, empty result   ", [&] { reg::poly_predict(reg::PolyFitResult{}, x); });
}
```

Output

```text
degree 0 in poly_fit     -> subsp::stats::regression::poly_fit: degree must be >= 1
degree 0 in poly_expand  -> subsp::stats::regression::poly_expand: degree must be >= 1
degree 8 on 8 rows       -> subsp::stats::regression::ols_fit: requires n >= number of parameters
two names, one predictor -> subsp::stats::regression::poly_fit: predictor_names size must match input columns
predict, two columns     -> subsp::stats::regression::poly_predict: expected X with 1 columns, the predictors the fit used
predict, empty result    -> subsp::stats::regression::poly_predict: fit.ols.coefficients is undefined
```

A degree below 1 is rejected by both `poly_fit` and `poly_expand`. A degree of 8 on 8 rows needs 9 parameters and is rejected by the underlying fit. The two prediction failures name the number of columns and the empty result.
