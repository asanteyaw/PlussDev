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
      const auto coef = fit.ols.coefficients.to(pluss::pCPU, ScalarType::Float64).host<double>();
      expect(fit.ols.k_params == 3, "poly linear-quadratic parameter count");
      expect(approx(coef[0], 1.0, 1e-7), "poly intercept");
      expect(approx(coef[1], 2.0, 1e-7), "poly linear coeff");
      expect(approx(coef[2], 0.5, 1e-7), "poly quadratic coeff");

      const Series pred = reg::poly_predict(fit, mat({1.5, -2.0}, 2, 1, cfg), cfg);
      const auto pv = pred.to(pluss::pCPU, ScalarType::Float64).host<double>();
      expect(approx(pv[0], 1.0 + 2.0 * 1.5 + 0.5 * 1.5 * 1.5, 1e-7), "poly predict value 1");
      expect(approx(pv[1], 1.0 + 2.0 * -2.0 + 0.5 * 4.0, 1e-7), "poly predict value 2");
      expect(reg::poly_summary(fit).find("Polynomial Regression") != std::string::npos, "poly summary title");

      // a single observation of two predictors is a [1, 2] row
      std::vector<double> flat, outcome;
      for (int i = 0; i < 12; ++i) {
        const double a = (i % 4) * 0.5, b = (i / 4) * 0.7;
        flat.push_back(a);
        flat.push_back(b);
        outcome.push_back(1.0 + a + b + 0.3 * a * b + 0.02 * (i % 3));
      }
      reg::PolyOptions two_options;
      two_options.degree = 2;
      two_options.include_cross = true;
      const auto two_fit = reg::poly_fit(vec(outcome, cfg), mat(flat, 12, 2, cfg), two_options, cfg);
      const Series one_row = reg::poly_predict(two_fit, mat({0.5, 0.7}, 1, 2, cfg), cfg);
      const Series two_rows = reg::poly_predict(two_fit, mat({0.5, 0.7, 0.5, 0.7}, 2, 2, cfg), cfg);
      expect(one_row.numel() == 1 && approx(one_row.host<double>().front(), two_rows.host<double>().front(), 1e-12), "poly_predict accepts one observation as a [1, p] row");

      // a prediction set with the wrong number of columns is refused with a message about the columns
      bool refused = false;
      try {
        (void)reg::poly_predict(fit, mat({1.0, 2.0, 3.0, 4.0}, 2, 2, cfg), cfg);
      } catch (const std::exception& e) {
        refused = std::string(e.what()).find("expected X with 1 columns") != std::string::npos;
      }
      expect(refused, "poly_predict names the expected number of columns");
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
      const auto coef = fit.ols.coefficients.to(pluss::pCPU, ScalarType::Float64).host<double>();
      const auto& names = fit.ols.coefficient_names;
      const std::int64_t idx = find_coef(names, "a*b");
      expect(idx >= 0, "poly cross term present");
      expect(approx(coef[static_cast<std::size_t>(idx)], 0.75, 1e-7), "poly cross coefficient");
      const Series pred = reg::poly_predict(
          fit,
          mat({1.2, -0.7, 1.2, -0.7}, 2, 2, cfg),
          cfg);
      const auto pv = pred.to(pluss::pCPU, ScalarType::Float64).host<double>();
      expect(approx(pv[0], 1.0 + 1.2 - 0.7 + 0.75 * 1.2 * -0.7, 1e-6), "poly cross predict");
    }

    std::cout << "poly_regression_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "poly_regression_test failed: " << e.what() << '\n';
    return 1;
  }
}
