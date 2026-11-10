<!-- generated from least_squares.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Least Squares

Least squares estimates a linear conditional mean: it chooses the coefficients β that minimise the weighted sum of squared residuals Σ wᵢ (yᵢ − xᵢβ)². It is the first model to fit for a continuous response, because its coefficients have a direct reading (the change in the mean of the response per unit of each predictor) and its inference is exact for Gaussian errors. The family has one function that fits, one that predicts, and two that report, plus the options, the covariance choice and the result they use.

## Statistical Contract

With an intercept the fitted design is `[1, x]`; without one it is exactly `x`. The coefficients come from a pseudoinverse of the weighted design, cut off at `OLSOptions::rcond`, so a rank-deficient design is still solved and the result records the rank, the smallest eigenvalue, the condition number and warnings; read those before trusting a coefficient. The covariance is mse × (X′WX)⁻¹ with mse = sse / df_resid, tests and intervals use the Student t distribution on the residual degrees of freedom, and the F statistic tests all slopes at once.

`OLSOptions::weights` gives case-weighted least squares: a weight is a repeat count, as if the row were listed that many times. It is not an inverse-variance weight with a separately estimated heteroskedastic covariance; for that, or for a quantile of the response, see the quantile family.

Beyond the coefficients the result carries the diagnostics that say whether a straight line is a fair description: R² and its adjusted form, the log-likelihood with AIC and BIC, the Durbin-Watson statistic for neighbouring residuals, and the omnibus and Jarque-Bera tests with the skewness and kurtosis of the residuals for normality.

## Practical Examples

The program fits eight points close to a line, reads the inference, predicts two new points and prints the report.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  reg::OLSOptions options;
  options.response_name = "load";
  options.predictor_names = {"hours"};
  const auto fit = reg::ols_fit(y, x, options);
  out.writeln(fit.coefficient_names, fit.coefficients.items<double>(), " R^2", fit.rsquared);
  out.writeln("std_errors", fit.std_errors.items<double>(), " p_values", fit.p_values.items<double>());
  const auto next = pluss::make_series(std::vector<double>{9.0, 10.0}, {2}, as_f64());
  out.writeln("fitted at 9 and 10", reg::ols_predict(fit, next).items<double>());
  out.writeln(reg::ols_summary(fit));
}
```

Output

```text
[const, hours] [0.21786, 1.97381]  R^2 0.99895
std_errors [0.13162, 0.02606]  p_values [0.14896, 0.00000]
fitted at 9 and 10 [17.98214, 19.95595]
                            OLS Regression Results
==============================================================================
Dep. Variable:                      load   R-squared:                         0.999
Model:                               OLS   Adj. R-squared:                   0.9988
Method:                    Least Squares   F-statistic:                        5735
Rank:                                  2   Prob (F-statistic):             3.569e-10
Alpha:                              0.05   Log-Likelihood:                    4.026
No. Observations:                      8   AIC:                              -4.052
Df Residuals:                          6   BIC:                              -3.893
Df Model:                              1   Covariance Type:               nonrobust
==============================================================================
                        coef     std err           t       P>|t|      [0.025      0.975]
------------------------------------------------------------------------------
const                 0.2179      0.1316      1.6552      0.1490     -0.1042      0.5399
hours                 1.9738      0.0261     75.7296  3.5688e-10      1.9100      2.0376
==============================================================================
Omnibus:                            1.16   Durbin-Watson:                      2.86
Prob(Omnibus):                    0.5598   Jarque-Bera (JB):                 0.7909
Skew:                            -0.5117   Prob(JB):                         0.6734
Kurtosis:                          1.849   Cond. No.                          11.48
==============================================================================
```

## Related Pages

The {guide}`regression guide <regress>` develops residual diagnostics, collinearity, missing observations and hand-checked examples in longer form.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

least_squares/OLSCovarianceType
least_squares/OLSOptions
least_squares/OLSFitResult
least_squares/ols_fit
least_squares/ols_predict
least_squares/ols_summary
least_squares/print_ols_summary
```
<!-- apiref:toctree:end -->
