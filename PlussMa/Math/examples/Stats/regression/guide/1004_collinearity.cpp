// Collinear predictors. If one predictor is (nearly) a combination of the others, the coefficients are not separately determined: the
// standard errors blow up and the fit is unstable. The result reports `condition_number` (the ratio of the largest to the smallest singular
// value of the design matrix; values above about 30 deserve attention), `rank`, and a list of `warnings`. Here: x2 = 2 x1 exactly (rank
// deficient), then x2 = 2 x1 plus a small perturbation (full rank, but very ill conditioned), compared with an unrelated second predictor.
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
  const std::vector<double> x1{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  const std::vector<double> y{2.3, 4.1, 6.4, 7.7, 10.2, 11.8, 14.1, 16.3, 17.8, 20.2};
  auto run = [&](const char* label, const std::vector<double>& x2) {
    std::vector<double> flat;
    for (std::size_t i = 0; i < x1.size(); ++i) { flat.push_back(x1[i]); flat.push_back(x2[i]); }
    const auto fit = reg::ols_fit(vec(y), table(flat, 10, 2));
    const auto se = fit.std_errors.to_vec<double>();
    std::cout << std::left << std::setw(30) << label << std::setw(8) << fit.rank << std::scientific << std::setprecision(1) << std::setw(14) << fit.condition_number << std::fixed
              << std::setprecision(3) << "slope-1 std error " << se[1] << "   warnings " << fit.warnings.size() << "\n";
    for (const auto& w : fit.warnings) std::cout << "    warning: " << w << "\n";
  };
  std::cout << std::left << std::setw(30) << "second predictor" << std::setw(8) << "rank" << std::setw(14) << "condition no." << "\n";
  std::vector<double> exact(10), near_exact(10), unrelated{3, 1, 4, 1, 5, 9, 2, 6, 5, 3};
  for (int i = 0; i < 10; ++i) { exact[i] = 2.0 * x1[i]; near_exact[i] = 2.0 * x1[i] + (i % 2 ? 1e-4 : -1e-4); }
  run("unrelated", unrelated);
  run("2 * x1 plus 1e-4 noise", near_exact);
  run("2 * x1 exactly", exact);
}
