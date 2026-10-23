// Options. `OLSOptions` controls the confidence level (`alpha`, 0.05 gives 95% intervals), the intercept (`add_intercept`), missing data
// (`drop_missing` removes any row with a NaN in y or x and reports how many) and `rcond`, the cutoff below which a direction of the design
// matrix is treated as rank deficient. Here: the same data with (a) a different confidence level, (b) no intercept (a line through the origin,
// where R-squared is computed about zero), and (c) two rows with missing values.
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
    const auto ci = reg::ols_fit(vec(ys), column(xs), o).conf_int.to_vec<double>();
    std::cout << "alpha 0.10, slope interval : [" << ci[2] << ", " << ci[3] << "]   (95% was [1.9296, 2.0656])\n";
  }
  {
    reg::OLSOptions o;
    o.add_intercept = false;
    const auto fit = reg::ols_fit(vec(ys), column(xs), o);
    std::cout << "no intercept, slope        : " << fit.coefficients.to_vec<double>()[0] << "  (" << fit.coefficient_names[0] << "), parameters " << fit.k_params << "\n";
  }
  {
    auto ym = ys;
    auto xm = xs;
    ym[2] = nan;
    xm[5] = nan;
    const auto fit = reg::ols_fit(vec(ym), column(xm));
    std::cout << "with 2 missing rows        : used " << fit.nobs << " of 8 observations, dropped " << fit.dropped_observations << ", slope " << fit.coefficients.to_vec<double>()[1] << "\n";
  }
}
