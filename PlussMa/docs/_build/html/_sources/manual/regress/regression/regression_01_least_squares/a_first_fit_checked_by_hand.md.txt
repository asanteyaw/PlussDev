<!-- generated from a_first_fit_checked_by_hand.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A first fit, checked by hand

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
  const auto fit = reg::ols_fit(vec(ys), column(xs));
  const auto b = fit.coefficients.to_vec<double>();
  double mx = 0, my = 0;
  for (std::size_t i = 0; i < xs.size(); ++i) { mx += xs[i] / 8; my += ys[i] / 8; }
  double sxx = 0, sxy = 0;
  for (std::size_t i = 0; i < xs.size(); ++i) { sxx += (xs[i] - mx) * (xs[i] - mx); sxy += (xs[i] - mx) * (ys[i] - my); }
  std::cout << std::fixed << std::setprecision(6);
  std::cout << "coefficients       : " << b[0] << " (intercept), " << b[1] << " (slope)\n";
  std::cout << "by hand            : " << my - sxy / sxx * mx << ", " << sxy / sxx << "\n";
  const auto se = fit.std_errors.to_vec<double>(), t = fit.t_values.to_vec<double>(), p = fit.p_values.to_vec<double>();
  std::cout << "std errors         : " << se[0] << ", " << se[1] << "\n";
  std::cout << "t values           : " << t[0] << ", " << t[1] << "\n";
  std::cout << std::scientific << std::setprecision(3) << "p values           : " << p[0] << ", " << p[1] << "\n";
  const auto ci = fit.conf_int.to_vec<double>();
  std::cout << std::fixed << std::setprecision(6) << "95% intervals      : [" << ci[0] << ", " << ci[1] << "], [" << ci[2] << ", " << ci[3] << "]\n";
  std::cout << "R-squared          : " << fit.rsquared << "  (adjusted " << fit.adj_rsquared << ")\n";
  std::cout << "residual std dev   : " << fit.rmse << "  on " << fit.df_resid << " degrees of freedom\n";
  std::cout << "observations/rank  : " << fit.nobs << " / " << fit.rank << "\n";
}
```

Output

```text
coefficients       : 0.035714 (intercept), 1.997619 (slope)
by hand            : 0.035714, 1.997619
std errors         : 0.140385, 0.027800
t values           : 0.254402, 71.855652
p values           : 8.077e-01, 4.889e-10
95% intervals      : [-0.307796, 0.379225], [1.929594, 2.065644]
R-squared          : 0.998839  (adjusted 0.998646)
residual std dev   : 0.180167  on 6 degrees of freedom
observations/rank  : 8 / 2
```
The coefficients match the textbook formulas (slope = S<sub>xy</sub> / S<sub>xx</sub>, intercept = ȳ − slope · x̄) to the printed digits. The slope is 71.9 standard errors from zero (p ≈ 5e-10); the intercept, at 0.25 standard errors, is not distinguishable from zero. The 95% interval for the slope, [1.930, 2.066], is the estimate plus or minus the 97.5% point of Student's t distribution on 6 degrees of freedom (2.447) times its standard error. Note that the degrees of freedom are n minus the parameters, and the residual standard deviation `rmse` uses them.
