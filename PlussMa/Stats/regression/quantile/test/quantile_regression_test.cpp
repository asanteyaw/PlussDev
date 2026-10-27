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
    const auto coef = fit.coefficients.to(pluss::pCPU, ScalarType::Float64).host<double>();

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

    {
      // Reference values from an independent implementation of the Hall-Sheather / Epanechnikov robust covariance (tau = 0.25):
      // coefficients (-1.058823, 2.052941), standard errors (2.560936, 0.2225928), Student-t p-values (0.6841599, 3.055392e-08).
      const std::vector<double> hx{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
      const std::vector<double> hy{1.9, 4.3, 5.1, 8.4, 9.7, 10.9, 15.8, 14.2, 19.9, 20.1, 27.0, 23.8, 31.2, 26.9, 36.5, 30.1, 41.0, 38.2, 46.3, 40.0};
      reg::QuantileOptions o;
      o.tau = 0.25;
      const auto f = reg::quantile_fit(pluss::make_series(hy, {20}).to(ScalarType::Float64), pluss::make_series(hx, {20, 1}).to(ScalarType::Float64), o);
      const auto b = f.coefficients.host<double>(), se = f.std_errors.host<double>(), pv = f.p_values.host<double>();
      expect(std::abs(b[0] + 1.058823) < 1e-4 && std::abs(b[1] - 2.052941) < 1e-4, "quantile coefficients match the reference");
      expect(std::abs(se[0] - 2.560936) < 1e-3 && std::abs(se[1] - 0.2225928) < 1e-4, "quantile standard errors match the reference");
      expect(std::abs(pv[0] - 0.6841599) < 1e-4 && std::abs(pv[1] - 3.055392e-08) < 1e-8, "quantile p-values use the Student t distribution");
      // an extreme tau with a small sample must still give finite standard errors
      o.tau = 0.9;
      const auto g = reg::quantile_fit(pluss::make_series(hy, {20}).to(ScalarType::Float64), pluss::make_series(hx, {20, 1}).to(ScalarType::Float64), o);
      const auto sg = g.std_errors.host<double>();
      expect(std::isfinite(sg[0]) && sg[0] < 10.0 && sg[1] < 1.0, "extreme tau gives sensible standard errors");
    }
    {
      // one wild observation must not inflate the standard errors of the median fit
      std::vector<double> hx, hy;
      for (int i = 1; i <= 10; ++i) {
        hx.push_back(i);
        hy.push_back(1.0 + 2.0 * i + (i % 2 ? 0.05 : -0.05));
      }
      hy[9] = 60.0;
      const auto f = reg::quantile_fit(pluss::make_series(hy, {10}).to(ScalarType::Float64), pluss::make_series(hx, {10, 1}).to(ScalarType::Float64));
      expect(f.std_errors.host<double>()[1] < 0.5, "an outlier does not inflate the quantile-regression standard errors");
    }

    std::cout << "quantile_regression_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "quantile_regression_test failed: " << e.what() << '\n';
    return 1;
  }
}
