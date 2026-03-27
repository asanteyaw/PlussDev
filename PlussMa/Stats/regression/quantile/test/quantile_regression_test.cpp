#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
namespace reg = pluss::subsp::stats::regression;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

bool approx(double a, double b, double tol) {
  return std::abs(a - b) <= tol;
}

Series vec(const std::vector<double>& data, const pluss::Config& cfg) {
  return pluss::make_series(data, {static_cast<std::int64_t>(data.size())}, cfg);
}

Series mat(const std::vector<double>& data, std::int64_t rows, std::int64_t cols, const pluss::Config& cfg) {
  return pluss::make_series(data, {rows, cols}, cfg);
}

} // namespace

int main() {
  try {
    const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

    std::vector<double> xv;
    std::vector<double> yv;
    for (std::int64_t i = 0; i < 40; ++i) {
      const double x = static_cast<double>(i) / 5.0;
      const double noise = (i % 3 == 0) ? 0.8 : ((i % 3 == 1) ? -0.2 : 0.15);
      xv.push_back(x);
      yv.push_back(1.2 + 2.1 * x + noise);
    }

    const Series x = mat(xv, static_cast<std::int64_t>(xv.size()), 1, cfg);
    const Series y = vec(yv, cfg);

    reg::QuantileOptions opt;
    opt.tau = 0.5;
    opt.alpha = 0.05;
    opt.max_iter = 600;
    const auto fit = reg::quantile_fit(y, x, opt, cfg);
    const auto coef = fit.coefficients.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();

    expect(fit.converged, "quantile converged");
    expect(fit.k_params == 2, "quantile k_params");
    expect(approx(coef[0], 1.2, 0.7), "quantile intercept");
    expect(approx(coef[1], 2.1, 0.35), "quantile slope");
    expect(std::isfinite(fit.objective), "quantile objective finite");
    expect(std::isfinite(fit.pseudo_rsquared), "quantile pseudo-r2 finite");

    const Series pred = reg::quantile_predict(fit, x, cfg);
    expect(pred.numel() == x.numel(), "quantile predict shape");

    const std::string summary = reg::quantile_summary(fit);
    expect(summary.find("Quantile Regression Results") != std::string::npos, "quantile summary title");

    std::cout << "quantile_regression_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "quantile_regression_test failed: " << e.what() << '\n';
    return 1;
  }
}
