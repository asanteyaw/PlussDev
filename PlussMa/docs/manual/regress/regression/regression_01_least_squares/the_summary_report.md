<!-- generated from the_summary_report.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The summary report

[Back to Least Squares](../regression_01_least_squares.md)

Every family writes its fit as a fixed-layout text report: a header of model statistics, a coefficient table, diagnostics, and a list of warnings if there are any.

```cpp
std::ostream &reg::print_ols_summary(std::ostream &os, const reg::OLSFitResult &fit);
std::string    reg::ols_summary(const reg::OLSFitResult &fit);
```

`print_ols_summary` writes to any stream and returns it. `ols_summary` returns the same text as a string. The names in the report come from `OLSOptions::response_name` and `predictor_names`.

## The report

The header lists the model (`Dep. Variable`, `Rank`, `Alpha`, the observation count and both degrees of freedom) beside the fit statistics. The table lists, for each coefficient, the estimate, its standard error, its *t* statistic, the two-sided p-value, and the confidence interval. The last block holds the residual diagnostics and the condition number.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  const std::vector<double> age{1, 2, 3, 4, 5, 6, 7, 8};
  const std::vector<double> height{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1};
  reg::OLSOptions options;
  options.response_name = "height";
  options.predictor_names = {"age"};
  const auto fit = reg::ols_fit(pluss::make_series(height, {8}, as_f64()), pluss::make_series(age, {8}, as_f64()), options);
  reg::print_ols_summary(std::cout, fit);
}
```

Output

```text
                            OLS Regression Results                            
==============================================================================
Dep. Variable:                    height   R-squared:                        0.9988
Model:                               OLS   Adj. R-squared:                   0.9986
Method:                    Least Squares   F-statistic:                        5163
Rank:                                  2   Prob (F-statistic):             4.889e-10
Alpha:                              0.05   Log-Likelihood:                     3.51
No. Observations:                      8   AIC:                               -3.02
Df Residuals:                          6   BIC:                              -2.861
Df Model:                              1   Covariance Type:               nonrobust
==============================================================================
                        coef     std err           t       P>|t|      [0.025      0.975]
------------------------------------------------------------------------------
const                 0.0357      0.1404      0.2544      0.8077     -0.3078      0.3792
age                   1.9976      0.0278     71.8557  4.8889e-10      1.9296      2.0656
==============================================================================
Omnibus:                            2.34   Durbin-Watson:                     3.286
Prob(Omnibus):                    0.3104   Jarque-Bera (JB):                 0.9501
Skew:                            -0.4041   Prob(JB):                         0.6218
Kurtosis:                          1.518   Cond. No.                          11.48
==============================================================================
```

The header values are the fit fields printed to four significant digits. For a single predictor the F-statistic equals the square of the slope's *t* statistic: 71.8557² = 5163. The p-values of the table switch to scientific notation below 10⁻³ (4.8889e-10), and the residual diagnostics repeat the values read in the previous pages.

## Strings, streams and the interval heading

`ols_summary` and `print_ols_summary` produce identical text, so the string form can be logged, compared or stored. The interval columns are headed with the limits actually used: the lower is `alpha / 2` and the upper `1 − alpha / 2`. With `alpha = 0.10` the heading is `[0.05` and `0.95]`, and the intervals are the 90% intervals.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <sstream>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::vector<double> age{1, 2, 3, 4, 5, 6, 7, 8};
  const std::vector<double> height{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1};
  const auto y = pluss::make_series(height, {8}, as_f64());
  const auto x = pluss::make_series(age, {8}, as_f64());

  const auto fit = reg::ols_fit(y, x);
  std::ostringstream stream;
  reg::print_ols_summary(stream, fit);
  out.writeln("the string and the stream agree:", stream.str() == reg::ols_summary(fit) ? "yes" : "no");

  reg::OLSOptions ninety;
  ninety.alpha = 0.10;
  const auto text = reg::ols_summary(reg::ols_fit(y, x, ninety));
  const auto heading = text.substr(text.find("coef"), text.find('\n', text.find("coef")) - text.find("coef"));
  out.writeln("heading at alpha = 0.10:", heading);
  const auto fit90 = reg::ols_fit(y, x, ninety);
  out.writeln("slope interval at 90%:", fit90.conf_int.host<double>().array().data[2], fit90.conf_int.host<double>().array().data[3]);
  out.writeln("slope interval at 95%:", fit.conf_int.host<double>().array().data[2], fit.conf_int.host<double>().array().data[3]);
}
```

Output

```text
the string and the stream agree: yes
heading at alpha = 0.10: coef     std err           t       P>|t|       [0.05       0.95]
slope interval at 90%: 1.9436 2.0516
slope interval at 95%: 1.9296 2.0656
```

The heading follows `alpha`: `[0.05` and `0.95]`, and the intervals below it are the 90% intervals, [1.9436, 2.0516], inside the 95% interval [1.9296, 2.0656].

## Warnings

A fit that succeeds numerically can still be unreliable, and the report ends with a numbered `Warnings` list when it is. The result holds the same messages in `warnings`, so a program can test for them. Three conditions raise one:

- the design matrix is rank deficient (`rank < k_params`), so the coefficients are not unique and the pseudoinverse solution was returned;
- the design is nearly singular: the smallest eigenvalue of X′X is below 10⁻¹⁰, or else the condition number exceeds 1000;
- the model has no intercept, where R² and F are measured about zero.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const std::vector<double> x1{1, 2, 3, 4, 5, 6, 7, 8};
  const std::vector<double> y{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1};
  std::vector<double> doubled, pair;
  for (double v : x1) {
    pair.push_back(v);
    pair.push_back(2.0 * v);
  }
  const auto response = pluss::make_series(y, {8}, as_f64());

  const auto singular = reg::ols_fit(response, pluss::make_series(pair, {8, 2}, as_f64()));
  out.writeln("duplicate predictor: rank", singular.rank, "of", singular.k_params, " warnings", singular.warnings.size());
  reg::print_ols_summary(std::cout, singular);
  reg::OLSOptions origin;
  origin.add_intercept = false;
  const auto no_intercept = reg::ols_fit(response, pluss::make_series(x1, {8}, as_f64()), origin);
  out.writeln("\nno intercept: warnings", no_intercept.warnings);
}
```

Output

```text
duplicate predictor: rank 2 of 3  warnings 2
                            OLS Regression Results                            
==============================================================================
Dep. Variable:                         y   R-squared:                        0.9988
Model:                               OLS   Adj. R-squared:                   0.9986
Method:                    Least Squares   F-statistic:                        5163
Rank:                                  2   Prob (F-statistic):             4.889e-10
Alpha:                              0.05   Log-Likelihood:                     3.51
No. Observations:                      8   AIC:                               -3.02
Df Residuals:                          6   BIC:                              -2.861
Df Model:                              1   Covariance Type:               nonrobust
==============================================================================
                        coef     std err           t       P>|t|      [0.025      0.975]
------------------------------------------------------------------------------
const                 0.0357      0.1404      0.2544      0.8077     -0.3078      0.3792
x1                    0.3995      0.0056     71.8557  4.8889e-10      0.3859      0.4131
x2                    0.7990      0.0111     71.8557  4.8889e-10      0.7718      0.8263
==============================================================================
Omnibus:                            2.34   Durbin-Watson:                     3.286
Prob(Omnibus):                    0.3104   Jarque-Bera (JB):                 0.9501
Skew:                            -0.4041   Prob(JB):                         0.6218
Kurtosis:                          1.518   Cond. No.                            inf
==============================================================================
Warnings:
[1] The design matrix is singular or rank deficient; coefficients were estimated using a pseudoinverse.
[2] The smallest eigenvalue is 4.933e-31. This might indicate strong multicollinearity or a singular design matrix.

no intercept: warnings [Model has no explicit intercept; R-squared and F-statistic interpretations differ from intercept models.]
```

The duplicated predictor (x₂ = 2x₁) has rank 2 of 3 and raises both rank warnings. The condition number prints as `inf` because the smallest singular value is numerically zero. The two slopes, 0.3995 and 0.7990, are the minimum-norm split of the single estimable slope (0.3995 + 2 × 0.7990 = 1.9976) and have the same *t* statistic, 71.86. The last line shows the single warning of a model without an intercept.

The same three reports exist for the other families: `glm_summary`, `quantile_summary` and `poly_summary`, with the matching `print_*` functions. Each is shown in its own chapter.
