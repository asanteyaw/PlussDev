// The summary and prediction. `reg::print_ols_summary(os, fit)` writes a formatted report, and `reg::ols_summary(fit)` returns it as a string;
// names come from `OLSOptions` (`response_name`, `predictor_names`). `reg::ols_predict(fit, x_new)` evaluates the fitted line at new predictor
// values, adding the intercept the fit used. Here: the same eight points with named variables, then a prediction at x = 9 and x = 10.
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
