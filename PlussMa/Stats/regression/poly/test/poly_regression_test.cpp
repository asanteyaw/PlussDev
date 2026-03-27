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

std::int64_t find_coef(const std::vector<std::string>& names, const std::string& needle) {
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (names[i] == needle) {
      return static_cast<std::int64_t>(i);
    }
  }
  return -1;
}

} // namespace

int main() {
  try {
    const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

    {
      std::vector<double> xv;
      std::vector<double> yv;
      for (std::int64_t i = -6; i <= 6; ++i) {
        const double x = static_cast<double>(i) / 2.0;
        xv.push_back(x);
        yv.push_back(1.0 + 2.0 * x + 0.5 * x * x);
      }
      const Series x = mat(xv, static_cast<std::int64_t>(xv.size()), 1, cfg);
      const Series y = vec(yv, cfg);

      reg::PolyOptions opt;
      opt.degree = 2;
      opt.include_cross = false;
      opt.predictor_names = {"u"};

      const auto fit = reg::poly_fit(y, x, opt, cfg);
      const auto coef = fit.ols.coefficients.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
      expect(fit.ols.k_params == 3, "poly linear-quadratic parameter count");
      expect(approx(coef[0], 1.0, 1e-7), "poly intercept");
      expect(approx(coef[1], 2.0, 1e-7), "poly linear coeff");
      expect(approx(coef[2], 0.5, 1e-7), "poly quadratic coeff");

      const Series pred = reg::poly_predict(fit, mat({1.5, -2.0}, 2, 1, cfg), cfg);
      const auto pv = pred.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
      expect(approx(pv[0], 1.0 + 2.0 * 1.5 + 0.5 * 1.5 * 1.5, 1e-7), "poly predict value 1");
      expect(approx(pv[1], 1.0 + 2.0 * -2.0 + 0.5 * 4.0, 1e-7), "poly predict value 2");
      expect(reg::poly_summary(fit).find("Polynomial Regression") != std::string::npos, "poly summary title");
    }

    {
      std::vector<double> xv;
      std::vector<double> yv;
      for (double x1 : std::vector<double>{-1.0, 0.0, 1.0, 2.0}) {
        for (double x2 : std::vector<double>{-2.0, -0.5, 1.0}) {
          xv.push_back(x1);
          xv.push_back(x2);
          yv.push_back(1.0 + x1 + x2 + 0.75 * x1 * x2);
        }
      }
      const Series x = mat(xv, static_cast<std::int64_t>(yv.size()), 2, cfg);
      const Series y = vec(yv, cfg);

      reg::PolyOptions opt;
      opt.degree = 2;
      opt.include_cross = true;
      opt.predictor_names = {"a", "b"};

      const auto fit = reg::poly_fit(y, x, opt, cfg);
      const auto coef = fit.ols.coefficients.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
      const auto& names = fit.ols.coefficient_names;
      const std::int64_t idx = find_coef(names, "a*b");
      expect(idx >= 0, "poly cross term present");
      expect(approx(coef[static_cast<std::size_t>(idx)], 0.75, 1e-7), "poly cross coefficient");
      const Series pred = reg::poly_predict(
          fit,
          mat({1.2, -0.7, 1.2, -0.7}, 2, 2, cfg),
          cfg);
      const auto pv = pred.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
      expect(approx(pv[0], 1.0 + 1.2 - 0.7 + 0.75 * 1.2 * -0.7, 1e-6), "poly cross predict");
    }

    std::cout << "poly_regression_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "poly_regression_test failed: " << e.what() << '\n';
    return 1;
  }
}
