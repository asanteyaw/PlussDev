// Polynomial regression. `reg::poly_fit(y, x, options)` expands the predictors into powers and fits by least squares:
// `PolyOptions::degree` is the highest power and `include_cross` adds products of different predictors (for two predictors and degree 2:
// x1, x2, x1^2, x2^2 and x1 x2). `reg::poly_expand(x, degree, include_cross)` returns the expanded design matrix on its own, and
// `reg::poly_predict` evaluates the fit. The result holds the underlying OLS fit in `ols`. Here: data from y = 1 - 2x + 0.5x^2 (no noise),
// recovered exactly at degree 2, a straight line fitted to the same data for comparison, and a two-predictor surface with a cross term.
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
  std::vector<double> xs, ys;
  for (int i = 0; i < 9; ++i) { xs.push_back(i * 0.5); ys.push_back(1.0 - 2.0 * xs.back() + 0.5 * xs.back() * xs.back()); }
  std::cout << std::fixed << std::setprecision(6);
  for (std::int64_t degree : {1, 2, 3}) {
    reg::PolyOptions o;
    o.degree = degree;
    const auto fit = reg::poly_fit(vec(ys), column(xs), o);
    std::cout << "degree " << degree << ": coefficients";
    for (double c : fit.ols.coefficients.host<double>()) std::cout << " " << std::round(c * 1e6) / 1e6 + 0.0;
    std::cout << "   R-squared " << fit.ols.rsquared << "\n";
  }
  std::cout << "poly_expand of x = 2 at degree 3: ";
  for (double v : reg::poly_expand(column({2.0}), 3).host<double>()) std::cout << v << " ";
  std::cout << "\n";

  // y = 1 + x1 + 2 x2 + 0.5 x1 x2 on a grid
  std::vector<double> grid, z;
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j) { grid.push_back(i); grid.push_back(j * 0.5); z.push_back(1.0 + i + 2.0 * j * 0.5 + 0.5 * i * j * 0.5); }
  reg::PolyOptions cross;
  cross.degree = 2;
  cross.include_cross = true;
  const auto fit = reg::poly_fit(vec(z), table(grid, 25, 2), cross);
  std::cout << "two predictors with cross term, degree 2:";
  for (const auto& name : fit.ols.coefficient_names) std::cout << " " << name;
  std::cout << "\n  coefficients:";
  for (double c : fit.ols.coefficients.host<double>()) std::cout << " " << std::setprecision(4) << c;
  std::cout << "\n";
}
