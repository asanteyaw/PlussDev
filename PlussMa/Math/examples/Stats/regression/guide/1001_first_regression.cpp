// Ordinary least squares. `reg::ols_fit(y, x)` fits y = b0 + b1 x1 + ... by minimising the sum of squared residuals. `y` has shape [n] and
// the predictors `x` have shape [n, p]; an intercept column is added by default. The result holds the coefficients (intercept first), their
// standard errors, t statistics, p-values and confidence intervals, the fitted values and residuals, and the summary statistics (R-squared,
// F, information criteria, diagnostics). Here: eight points that lie close to a line, with the slope and intercept also computed by hand from
// slope = Sxy / Sxx so the fit can be checked against the textbook formula.
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
