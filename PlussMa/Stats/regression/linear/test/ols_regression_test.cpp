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
      const auto coef = fit.coefficients.to(pluss::pCPU, ScalarType::Float64).host<double>();

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
      const auto pv = pred.to(pluss::pCPU, ScalarType::Float64).host<double>();
      expect(pred.numel() == 2, "ols_predict shape");
      expect(pv[1] > pv[0], "ols_predict monotonic check");

      const std::string summary = reg::ols_summary(fit);
      expect(summary.find("OLS Regression Results") != std::string::npos, "summary title");
      expect(summary.find("R-squared") != std::string::npos, "summary rsquared");
      expect(summary.find("feat1") != std::string::npos, "summary predictor name");
      expect(summary.find("[0.025") != std::string::npos && summary.find("0.975]") != std::string::npos, "summary interval heading at alpha 0.05");

      // The interval heading names the limits actually used, so it follows alpha.
      reg::OLSOptions ten = opt;
      ten.alpha = 0.10;
      const std::string ten_summary = reg::ols_summary(reg::ols_fit(y, x, ten, cfg));
      expect(ten_summary.find("[0.05") != std::string::npos && ten_summary.find("0.95]") != std::string::npos, "summary interval heading at alpha 0.10");
      expect(ten_summary.find("[0.025") == std::string::npos, "summary interval heading no longer fixed at 0.025");
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
      expect(fit.k_params == 1 && std::abs(fit.coefficients.host<double>().front() - 3.0) < 1e-12, "intercept-only OLS gives the mean");
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
      // without an intercept the total sum of squares is about zero: R-squared = 1 - SSE / sum(y^2), and F tests every coefficient
      const std::vector<double> xs{1, 2, 3, 4, 5, 6, 7, 8}, ys{10.1, 9.9, 10.2, 9.8, 10.0, 10.1, 9.9, 10.0};
      double sxy = 0.0, sxx = 0.0, syy = 0.0;
      for (std::size_t i = 0; i < xs.size(); ++i) {
        sxy += xs[i] * ys[i];
        sxx += xs[i] * xs[i];
        syy += ys[i] * ys[i];
      }
      const double slope = sxy / sxx;
      double sse = 0.0;
      for (std::size_t i = 0; i < xs.size(); ++i) {
        sse += (ys[i] - slope * xs[i]) * (ys[i] - slope * xs[i]);
      }
      reg::OLSOptions o;
      o.add_intercept = false;
      const auto fit = reg::ols_fit(pluss::make_series(ys, {8}).to(ScalarType::Float64), pluss::make_series(xs, {8, 1}).to(ScalarType::Float64), o);
      expect(std::abs(fit.sst - syy) < 1e-9, "no-intercept total sum of squares is about zero");
      expect(std::abs(fit.rsquared - (1.0 - sse / syy)) < 1e-12 && fit.rsquared >= 0.0, "no-intercept R-squared is uncentered");
      const double f = ((syy - sse) / 1.0) / (sse / 7.0);
      expect(std::abs(fit.f_stat - f) < 1e-8 * f, "no-intercept F statistic uses the uncentered explained sum of squares");
      expect(fit.df_model == 1 && fit.df_resid == 7, "no-intercept degrees of freedom");
    }
    {
      // one new observation of several predictors: a [1, p] row is that observation, with or without the column of ones
      const Series x = mat({1, 2, 2, 1, 3, 0, 4, 2, 5, 1, 6, 3, 7, 4, 8, 5}, 8, 2, cfg);
      const Series y = vec({0.9, 4.9, 7.2, 6.7, 10.0, 10.8, 10.6, 11.9}, cfg);
      const auto fit = reg::ols_fit(y, x, {}, cfg);
      const auto b = fit.coefficients.host<double>();
      const double expected = b[0] + b[1] * 9.0 + b[2] * 4.0;
      const Series row = reg::ols_predict(fit, mat({9.0, 4.0}, 1, 2, cfg), cfg);
      const Series full = reg::ols_predict(fit, mat({1.0, 9.0, 4.0}, 1, 3, cfg), cfg);
      expect(row.numel() == 1 && approx(row.host<double>().front(), expected, 1e-9), "a [1, p] row is one observation of p predictors");
      expect(full.numel() == 1 && approx(full.host<double>().front(), expected, 1e-9), "a [1, k] row is one observation of the full design");
      // a single predictor keeps the older reading: a [1, 2] row is two values of that predictor
      const Series one = vec({1, 2, 3, 4, 5}, cfg);
      const auto line = reg::ols_fit(vec({2, 4, 6, 8, 10}, cfg), one, {}, cfg);
      expect(reg::ols_predict(line, mat({6.0, 7.0}, 1, 2, cfg), cfg).numel() == 2, "a [1, 2] row for a one-predictor fit is two values");
    }
    {
      // integer case weights are the same as repeating the rows: coefficients, sums of squares, R-squared and likelihood agree, and
      // the covariance differs only by the residual degrees of freedom, which follow the rows
      const std::vector<double> xs{1, 2, 3, 4, 5, 6}, ys{1.1, 2.3, 2.8, 4.4, 4.7, 6.5}, counts{3, 1, 2, 1, 2, 1};
      std::vector<double> rx, ry;
      for (std::size_t i = 0; i < xs.size(); ++i) {
        for (int c = 0; c < static_cast<int>(counts[i]); ++c) {
          rx.push_back(xs[i]);
          ry.push_back(ys[i]);
        }
      }
      reg::OLSOptions weighted;
      weighted.weights = vec(counts, cfg);
      const auto a = reg::ols_fit(vec(ys, cfg), vec(xs, cfg), weighted, cfg);
      const auto b = reg::ols_fit(vec(ry, cfg), vec(rx, cfg), {}, cfg);
      const auto ca = a.coefficients.host<double>(), cb = b.coefficients.host<double>();
      expect(approx(ca[0], cb[0], 1e-9) && approx(ca[1], cb[1], 1e-9), "weighted OLS equals replicated rows: coefficients");
      expect(approx(a.sse, b.sse, 1e-9) && approx(a.sst, b.sst, 1e-9) && approx(a.rsquared, b.rsquared, 1e-9), "weighted OLS equals replicated rows: sums of squares");
      // the Gaussian likelihood of precision weights (variance sigma^2 / w_i): -(n (ln(2 pi sse / n) + 1) - sum ln w) / 2, with n the rows
      double sum_log_w = 0.0;
      for (double c : counts) {
        sum_log_w += std::log(c);
      }
      expect(approx(a.log_likelihood, -0.5 * (6.0 * (std::log(2.0 * 3.14159265358979323846 * a.sse / 6.0) + 1.0) - sum_log_w), 1e-9), "weighted OLS log-likelihood of precision weights");
      const double ratio = (a.mse / b.mse);
      expect(approx(ratio, static_cast<double>(b.df_resid) / static_cast<double>(a.df_resid), 1e-9), "weighted OLS: the covariance follows the row degrees of freedom");
      expect(a.nobs == 6 && a.df_resid == 4, "weighted OLS counts the rows used");

      reg::OLSOptions with_zero;
      with_zero.weights = vec({1, 1, 0, 1, 1, 1}, cfg);
      const auto z = reg::ols_fit(vec(ys, cfg), vec(xs, cfg), with_zero, cfg);
      expect(z.nobs == 5 && z.dropped_observations == 1, "a zero weight removes the row");
      bool negative = false;
      try {
        reg::OLSOptions bad;
        bad.weights = vec({1, 1, -1, 1, 1, 1}, cfg);
        (void)reg::ols_fit(vec(ys, cfg), vec(xs, cfg), bad, cfg);
      } catch (const std::exception&) {
        negative = true;
      }
      expect(negative, "a negative weight is rejected");

      reg::PolyOptions poly;
      poly.weights = vec(counts, cfg);
      const auto pf = reg::poly_fit(vec(ys, cfg), vec(xs, cfg), poly, cfg);
      reg::PolyOptions poly_rep;
      const auto pr = reg::poly_fit(vec(ry, cfg), vec(rx, cfg), poly_rep, cfg);
      expect(approx(pf.ols.coefficients.host<double>()[2], pr.ols.coefficients.host<double>()[2], 1e-8), "poly_fit passes the weights through");
    }
    {
      // the rank cutoff and the pseudoinverse that produces the coefficients are one and the same: once rcond declares the nearly
      // dependent direction lost, the minimum-norm solution (proportional to (1, 2)) is returned, not the full-rank solution
      std::vector<double> flat, yv;
      for (int i = 0; i < 10; ++i) {
        flat.push_back(i + 1.0);
        flat.push_back(2.0 * (i + 1.0) + (i % 2 ? 1e-4 : -1e-4));
        yv.push_back(2.0 * (i + 1.0) + 0.3 * (i % 3));
      }
      const Series y = pluss::make_series(yv, {10}).to(ScalarType::Float64);
      const Series x = pluss::make_series(flat, {10, 2}).to(ScalarType::Float64);
      reg::OLSOptions strict;
      strict.rcond = 1e-12;
      const auto full = reg::ols_fit(y, x, strict);
      expect(full.rank == 3, "tiny rcond keeps all three directions");
      expect(std::abs(full.coefficients.host<double>()[1]) > 100.0, "the full-rank solution has large offsetting coefficients");
      reg::OLSOptions loose;
      loose.rcond = 1e-6;
      const auto cut = reg::ols_fit(y, x, loose);
      const auto b = cut.coefficients.host<double>();
      expect(cut.rank == 2, "a larger rcond drops the nearly dependent direction");
      expect(std::abs(b[1]) < 5.0 && std::abs(b[2] / b[1] - 2.0) < 1e-3, "the coefficients are the minimum-norm solution for the reported rank");
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

    {
      // a default-constructed fit has nothing to report: the summary names the missing coefficients instead of failing inside the formatter
      bool threw = false;
      try {
        (void)reg::ols_summary(reg::OLSFitResult{});
      } catch (const std::exception& e) {
        threw = std::string(e.what()).find("print_ols_summary: fit.coefficients is undefined") != std::string::npos;
      }
      expect(threw, "ols_summary of an undefined fit reports the undefined coefficients");
    }

    std::cout << "ols_regression_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "ols_regression_test failed: " << e.what() << '\n';
    return 1;
  }
}
