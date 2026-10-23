<!-- generated from robust_and_conditional_quantile_fits.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Robust and conditional-quantile fits

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
  std::vector<double> xs{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, ys{3.0, 5.1, 6.9, 9.0, 11.1, 13.0, 15.0, 16.9, 19.1, 21.0};
  ys[9] = 60.0;  // an outlier in the last observation
  std::cout << std::fixed << std::setprecision(4);
  const auto ols_fit = reg::ols_fit(vec(ys), column(xs));
  const auto ols = ols_fit.coefficients.to_vec<double>();
  reg::QuantileOptions median;
  median.tau = 0.5;
  const auto q = reg::quantile_fit(vec(ys), column(xs), median);
  const auto qb = q.coefficients.to_vec<double>();
  std::cout << "with an outlier   OLS: intercept " << ols[0] << ", slope " << ols[1] << "\n";
  std::cout << "                  median: intercept " << qb[0] << ", slope " << qb[1] << "   (true 1, 2), converged " << q.converged << ", iterations " << q.iterations << "\n";
  const auto se_ols = ols_fit.std_errors.to_vec<double>(), se_q = q.std_errors.to_vec<double>();
  std::cout << "slope std error   OLS " << se_ols[1] << ", median " << se_q[1] << "   (the robust covariance uses a bandwidth that the outlier does not inflate)\n";

  // heteroscedastic data: y = 1 + 2 x + x * e, with e cycling through fixed values in [-1, 1]
  std::vector<double> hx, hy;
  const double e[] = {-1.0, -0.6, -0.2, 0.2, 0.6, 1.0, 0.0, -0.8, 0.4, 0.8};
  for (int i = 0; i < 60; ++i) {
    const double x = 1.0 + i * 0.25;
    hx.push_back(x);
    hy.push_back(1.0 + 2.0 * x + x * e[i % 10]);
  }
  for (double tau : {0.1, 0.5, 0.9}) {
    reg::QuantileOptions o;
    o.tau = tau;
    const auto b = reg::quantile_fit(vec(hy), column(hx), o).coefficients.to_vec<double>();
    std::cout << "tau " << std::setprecision(1) << tau << std::setprecision(4) << ": intercept " << b[0] << ", slope " << b[1] << "\n";
  }
}
```

Output

```text
with an outlier   OLS: intercept -6.7867, slope 4.1267
                  median: intercept 1.0000, slope 2.0000   (true 1, 2), converged 1, iterations 141
slope std error   OLS 1.2279, median 0.0222   (the robust covariance uses a bandwidth that the outlier does not inflate)
tau 0.1: intercept 0.7860, slope 1.2140
tau 0.5: intercept 0.3936, slope 2.2426
tau 0.9: intercept 0.1663, slope 3.0565
```
One wild observation drags least squares far from the truth (the intercept goes from 1 to −6.8, the slope from 2 to 4.1) because squared error gives it enormous weight; the median fit is unmoved, and its standard error is not inflated by the outlier either. For data whose spread grows with `x`, different quantiles have different slopes: the lower quantile line is flatter and the upper steeper than the median line, which describes the *spread* of the response, something no mean regression can.
