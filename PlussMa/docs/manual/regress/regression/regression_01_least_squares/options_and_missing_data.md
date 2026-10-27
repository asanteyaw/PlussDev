<!-- generated from options_and_missing_data.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Options and missing data

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
  const double nan = std::numeric_limits<double>::quiet_NaN();
  std::vector<double> xs{1, 2, 3, 4, 5, 6, 7, 8}, ys{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1};
  std::cout << std::fixed << std::setprecision(4);
  {
    reg::OLSOptions o;
    o.alpha = 0.10;
    const auto ci = reg::ols_fit(vec(ys), column(xs), o).conf_int.host<double>();
    std::cout << "alpha 0.10, slope interval : [" << ci[2] << ", " << ci[3] << "]   (95% was [1.9296, 2.0656])\n";
  }
  {
    reg::OLSOptions o;
    o.add_intercept = false;
    const auto fit = reg::ols_fit(vec(ys), column(xs), o);
    std::cout << "no intercept, slope        : " << fit.coefficients.host<double>()[0] << "  (" << fit.coefficient_names[0] << "), parameters " << fit.k_params << "\n";
  }
  {
    auto ym = ys;
    auto xm = xs;
    ym[2] = nan;
    xm[5] = nan;
    const auto fit = reg::ols_fit(vec(ym), column(xm));
    std::cout << "with 2 missing rows        : used " << fit.nobs << " of 8 observations, dropped " << fit.dropped_observations << ", slope " << fit.coefficients.host<double>()[1] << "\n";
  }
}
```

Output

```text
alpha 0.10, slope interval : [1.9436, 2.0516]   (95% was [1.9296, 2.0656])
no intercept, slope        : 2.0039  (x1), parameters 1
with 2 missing rows        : used 6 of 8 observations, dropped 2, slope 1.9973
```
A looser `alpha` gives a narrower interval. Without an intercept the line is forced through the origin, and the fit reports a warning that R² and F no longer have their usual meaning: here R² is still measured against the mean of `y`, so it can be negative for a poor fit. With `drop_missing` (the default) any row that has a non-finite value in `y` or in any predictor is removed before fitting, and `nobs` and `dropped_observations` say how many remain and how many went; if you would rather be told, set it to false and the fit throws.
