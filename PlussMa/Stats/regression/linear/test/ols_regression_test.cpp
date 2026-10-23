#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
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

    {
      const Series x = mat(
          {
              1.0, 2.0,
              2.0, 1.0,
              3.0, 0.0,
              4.0, 2.0,
              5.0, 1.0,
              6.0, 3.0,
              7.0, 4.0,
              8.0, 5.0,
          },
          8,
          2,
          cfg);
      const Series y = vec({0.9, 4.9, 7.2, 6.7, 10.0, 10.8, 10.6, 11.9}, cfg);

      reg::OLSOptions opt;
      opt.alpha = 0.05;
      opt.add_intercept = true;
      opt.response_name = "target";
      opt.predictor_names = {"feat1", "feat2"};

      const auto fit = reg::ols_fit(y, x, opt, cfg);
      const auto coef = fit.coefficients.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();

      expect(fit.nobs == 8, "ols_fit nobs");
      expect(fit.k_params == 3, "ols_fit k_params");
      expect(fit.df_resid == 5, "ols_fit df_resid");
      expect(coef.size() == 3, "ols_fit coef size");
      expect(approx(coef[0], 1.04, 0.40), "ols_fit intercept approx");
      expect(approx(coef[1], 1.97, 0.25), "ols_fit beta1 approx");
      expect(approx(coef[2], -1.02, 0.35), "ols_fit beta2 approx");
      expect(fit.rsquared > 0.95, "ols_fit rsquared");

      const Series x_test = mat({9.0, 6.0, 10.0, 7.0}, 2, 2, cfg);
      const Series pred = reg::ols_predict(fit, x_test, cfg);
      const auto pv = pred.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
      expect(pred.numel() == 2, "ols_predict shape");
      expect(pv[1] > pv[0], "ols_predict monotonic check");

      const std::string summary = reg::ols_summary(fit);
      expect(summary.find("OLS Regression Results") != std::string::npos, "summary title");
      expect(summary.find("R-squared") != std::string::npos, "summary rsquared");
      expect(summary.find("feat1") != std::string::npos, "summary predictor name");
    }

    {
      const double nan = std::numeric_limits<double>::quiet_NaN();
      const Series x = mat({1.0, 2.0, nan, 3.0, 4.0, 5.0}, 3, 2, cfg);
      const Series y = vec({1.0, 2.0, 3.0}, cfg);

      reg::OLSOptions opt;
      opt.drop_missing = true;
      opt.add_intercept = false;
      const auto fit = reg::ols_fit(y, x, opt, cfg);
      expect(fit.nobs == 2, "drop_missing nobs");
      expect(fit.dropped_observations == 1, "drop_missing dropped rows");
    }

    {
      const Series x = mat({1.0, 2.0, 2.0, 4.0, 3.0, 6.0, 4.0, 8.0, 5.0, 10.0}, 5, 2, cfg);
      const Series y = vec({1.0, 2.0, 3.0, 4.0, 5.0}, cfg);
      const auto fit = reg::ols_fit(y, x, {}, cfg);
      expect(fit.rank < fit.k_params, "rank-deficient rank check");
      expect(!fit.warnings.empty(), "rank-deficient warning check");
    }

    {
      // intercept-only model: the single coefficient is the sample mean
      const std::vector<double> yv{1.0, 2.0, 6.0, 3.0};
      const Series y = pluss::make_series(yv, {4}).to(ScalarType::Float64);
      const Series none = Series::zeros({4, 0}, ScalarType::Float64);
      const auto fit = reg::ols_fit(y, none);
      expect(fit.k_params == 1 && std::abs(fit.coefficients.item<double>() - 3.0) < 1e-12, "intercept-only OLS gives the mean");
      bool threw = false;
      try {
        reg::OLSOptions o;
        o.add_intercept = false;
        (void)reg::ols_fit(y, none, o);
      } catch (const std::exception&) {
        threw = true;
      }
      expect(threw, "a model with no predictors and no intercept is rejected");
    }
    {
      // nearly collinear predictors are flagged through the condition number
      std::vector<double> flat, yv;
      for (int i = 0; i < 10; ++i) {
        flat.push_back(i + 1.0);
        flat.push_back(2.0 * (i + 1.0) + (i % 2 ? 1e-4 : -1e-4));
        yv.push_back(2.0 * i + 0.1 * (i % 3));
      }
      const auto fit = reg::ols_fit(pluss::make_series(yv, {10}).to(ScalarType::Float64), pluss::make_series(flat, {10, 2}).to(ScalarType::Float64));
      expect(fit.rank == fit.k_params && fit.condition_number > 1e4 && !fit.warnings.empty(), "near-collinear design warns about the condition number");
    }

    {
      // reference values from an independent implementation: omnibus (D'Agostino-Pearson) 2.3395, p = 0.31044
      const std::vector<double> xs{1, 2, 3, 4, 5, 6, 7, 8}, ys{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1};
      const auto fit = reg::ols_fit(pluss::make_series(ys, {8}).to(ScalarType::Float64), pluss::make_series(xs, {8, 1}).to(ScalarType::Float64));
      expect(std::abs(fit.omnibus - 2.3395305) < 1e-4 && std::abs(fit.omnibus_pvalue - 0.3104398) < 1e-4, "omnibus test matches the reference");
      expect(std::abs(fit.jarque_bera - 0.9501294) < 1e-5 && std::abs(fit.durbin_watson - 3.2862673) < 1e-5, "Jarque-Bera and Durbin-Watson match");
      expect(fit.warnings.empty(), "a well-conditioned fit has no warnings");
    }

    std::cout << "ols_regression_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "ols_regression_test failed: " << e.what() << '\n';
    return 1;
  }
}
