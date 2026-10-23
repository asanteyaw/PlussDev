<!-- generated from the_summary_and_prediction.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The summary, and prediction

[Back to Least Squares](../regression_01_least_squares.md)


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
  const std::vector<double> xs{1, 2, 3, 4, 5, 6, 7, 8}, ys{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1};
  reg::OLSOptions options;
  options.response_name = "height";
  options.predictor_names = {"age"};
  const auto fit = reg::ols_fit(vec(ys), column(xs), options);
  reg::print_ols_summary(std::cout, fit);
  const auto forecast = reg::ols_predict(fit, column({9.0, 10.0})).to_vec<double>();
  std::cout << std::fixed << std::setprecision(4) << "\nprediction at 9 and 10: " << forecast[0] << ", " << forecast[1] << "\n";
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

prediction at 9 and 10: 18.0143, 20.0119
```
The summary collects the headline numbers: R² and its adjusted form, the F test of the whole model, the information criteria, one row per coefficient, and the residual diagnostics (omnibus and Jarque–Bera tests of normality, skewness, kurtosis, the Durbin–Watson statistic for serial correlation, and the condition number). `ols_predict` adds the intercept itself, so pass only the predictor values. A prediction outside the range of the data (9 and 10 here, past 8) extrapolates the line and is less reliable the further it goes.
