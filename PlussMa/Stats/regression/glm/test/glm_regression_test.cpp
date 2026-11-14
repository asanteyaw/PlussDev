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

double logistic(double z) {
  return 1.0 / (1.0 + std::exp(-z));
}

} // namespace

int main() {
  try {
    const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

    {
      const Series x = mat({1, 2, 2, 1, 3, 0, 4, 2, 5, 1, 6, 3, 7, 4, 8, 5}, 8, 2, cfg);
      const Series y = vec({0.9, 4.9, 7.2, 6.7, 10.0, 10.8, 10.6, 11.9}, cfg);

      reg::GLMOptions opt;
      opt.family = reg::GLMFamily::Normal;
      opt.link = reg::GLMLink::Identity;
      const auto fit = reg::glm_fit(y, x, opt, cfg);
      const auto coef = fit.coefficients.to(pluss::pCPU, ScalarType::Float64).host<double>();
      expect(fit.converged, "glm normal converged");
      expect(fit.k_params == 3, "glm normal k_params");
      expect(approx(coef[0], 1.04, 0.40), "glm normal intercept");
      expect(approx(coef[1], 1.97, 0.25), "glm normal beta1");
      expect(approx(coef[2], -1.02, 0.35), "glm normal beta2");
      expect(reg::glm_summary(fit).find("GLM Regression Results") != std::string::npos, "glm summary title");
      expect(reg::glm_summary(fit).find("[0.025") != std::string::npos, "glm summary interval heading at alpha 0.05");
      reg::GLMOptions ten = opt;
      ten.alpha = 0.10;
      const std::string ten_summary = reg::glm_summary(reg::glm_fit(y, x, ten, cfg));
      expect(ten_summary.find("[0.05") != std::string::npos && ten_summary.find("0.95]") != std::string::npos, "glm summary interval heading at alpha 0.10");
    }

    {
      const Series x = mat({1, 2, 2, 1, 3, 0, 4, 2, 5, 1, 6, 3, 7, 4, 8, 5}, 8, 2, cfg);
      const Series y = vec({0.9, 4.9, 7.2, 6.7, 10.0, 10.8, 10.6, 11.9}, cfg);
      const auto fit = reg::glm_fit(y, x, {}, cfg);
      const Series row = reg::glm_predict(fit, mat({9.0, 4.0}, 1, 2, cfg), reg::GLMPredictionType::Mean, cfg);
      const Series pair = reg::glm_predict(fit, mat({9.0, 4.0, 9.0, 4.0}, 2, 2, cfg), reg::GLMPredictionType::Mean, cfg);
      expect(row.numel() == 1 && approx(row.host<double>().front(), pair.host<double>().front(), 1e-12), "glm_predict accepts one observation as a [1, p] row");
    }

    {
      // integer case weights are the same as repeating the rows
      const std::vector<double> xs{1, 2, 3, 4, 5, 6}, ys{0, 1, 0, 1, 1, 1}, counts{3, 1, 2, 1, 2, 1};
      std::vector<double> rx, ry;
      for (std::size_t i = 0; i < xs.size(); ++i) {
        for (int c = 0; c < static_cast<int>(counts[i]); ++c) {
          rx.push_back(xs[i]);
          ry.push_back(ys[i]);
        }
      }
      reg::GLMOptions weighted;
      weighted.family = reg::GLMFamily::Binomial;
      weighted.weights = vec(counts, cfg);
      const auto a = reg::glm_fit(vec(ys, cfg), vec(xs, cfg), weighted, cfg);
      reg::GLMOptions plain;
      plain.family = reg::GLMFamily::Binomial;
      const auto b = reg::glm_fit(vec(ry, cfg), vec(rx, cfg), plain, cfg);
      const auto ca = a.coefficients.host<double>(), cb = b.coefficients.host<double>();
      expect(approx(ca[0], cb[0], 1e-7) && approx(ca[1], cb[1], 1e-7), "weights equal replicated rows: coefficients");
      expect(approx(a.deviance, b.deviance, 1e-7) && approx(a.null_deviance, b.null_deviance, 1e-7), "weights equal replicated rows: deviances");
      expect(approx(a.log_likelihood, b.log_likelihood, 1e-7), "weights equal replicated rows: log-likelihood");
      const auto va = a.covariance.host<double>(), vb = b.covariance.host<double>();
      expect(approx(va[0], vb[0], 1e-7) && approx(va[3], vb[3], 1e-7), "weights equal replicated rows: covariance");

      // a row of weight zero is not used, and is counted as dropped
      reg::GLMOptions with_zero = plain;
      with_zero.weights = vec({1, 1, 1, 0, 1, 1}, cfg);
      const auto z = reg::glm_fit(vec(ys, cfg), vec(xs, cfg), with_zero, cfg);
      expect(z.nobs == 5 && z.dropped_observations == 1, "a zero weight removes the row");

      bool negative = false;
      try {
        reg::GLMOptions bad = plain;
        bad.weights = vec({1, 1, -1, 1, 1, 1}, cfg);
        (void)reg::glm_fit(vec(ys, cfg), vec(xs, cfg), bad, cfg);
      } catch (const std::exception&) {
        negative = true;
      }
      expect(negative, "a negative weight is rejected");
    }

    {
      // a Poisson rate model: counts over unequal exposures, with the log exposure as an offset
      const std::vector<double> exposure{1, 2, 4, 1, 3, 5}, events{2, 5, 7, 3, 4, 11};
      std::vector<double> log_exposure;
      for (double e : exposure) {
        log_exposure.push_back(std::log(e));
      }
      reg::GLMOptions o;
      o.family = reg::GLMFamily::Poisson;
      o.offset = vec(log_exposure, cfg);
      const auto fit = reg::glm_fit(vec(events, cfg), pluss::series::zeros({6, 0}, cfg), o, cfg);
      double total_events = 0.0, total_exposure = 0.0;
      for (std::size_t i = 0; i < events.size(); ++i) {
        total_events += events[i];
        total_exposure += exposure[i];
      }
      expect(approx(fit.coefficients.host<double>().front(), std::log(total_events / total_exposure), 1e-7), "offset model recovers the overall rate");
      const auto mu = fit.fitted_values.host<double>();
      expect(approx(mu[2], 4.0 * total_events / total_exposure, 1e-6), "fitted counts scale with exposure");
      expect(fit.null_deviance > 0.0 && approx(fit.deviance, fit.null_deviance, 1e-6), "null model with an offset equals the intercept-only fit");
      const Series predicted = reg::glm_predict(fit, pluss::series::zeros({2, 0}, cfg), vec({std::log(10.0), std::log(0.5)}, cfg), reg::GLMPredictionType::Mean, cfg);
      expect(approx(predicted.host<double>()[0], 10.0 * total_events / total_exposure, 1e-6), "prediction with an offset");
      bool mismatched = false;
      try {
        (void)reg::glm_predict(fit, pluss::series::zeros({2, 0}, cfg), vec({1.0}, cfg), reg::GLMPredictionType::Mean, cfg);
      } catch (const std::exception&) {
        mismatched = true;
      }
      expect(mismatched, "an offset of the wrong length is rejected");
    }

    {
      // the logit link cannot match a response outside [0,1]
      bool rejected = false;
      try {
        reg::GLMOptions o;
        o.family = reg::GLMFamily::Poisson;
        o.link = reg::GLMLink::Logit;
        (void)reg::glm_fit(vec({1, 0, 2, 1, 3, 2}, cfg), vec({1, 2, 3, 4, 5, 6}, cfg), o, cfg);
      } catch (const std::exception& e) {
        rejected = std::string(e.what()).find("logit link expects y in [0,1]") != std::string::npos;
      }
      expect(rejected, "logit link with counts is rejected up front");
    }

    {
      // rank and coefficients share one cutoff, as in least squares
      std::vector<double> flat, yv;
      for (int i = 0; i < 10; ++i) {
        flat.push_back(i + 1.0);
        flat.push_back(2.0 * (i + 1.0) + (i % 2 ? 1e-4 : -1e-4));
        yv.push_back(2.0 * (i + 1.0) + 0.3 * (i % 3));
      }
      reg::GLMOptions o;
      o.rcond = 1e-6;
      const auto fit = reg::glm_fit(vec(yv, cfg), mat(flat, 10, 2, cfg), o, cfg);
      const auto b = fit.coefficients.host<double>();
      expect(fit.rank == 2, "glm rcond drops the nearly dependent direction");
      expect(std::abs(b[1]) < 5.0 && std::abs(b[2] / b[1] - 2.0) < 1e-3, "glm coefficients are the minimum-norm solution for the reported rank");
    }

    {
      std::vector<double> xv;
      std::vector<double> yv;
      for (std::int64_t i = -6; i <= 6; ++i) {
        const double x = static_cast<double>(i) / 3.0;
        xv.push_back(x);
        yv.push_back(logistic(-0.7 + 1.8 * x));
      }
      const Series x = mat(xv, static_cast<std::int64_t>(xv.size()), 1, cfg);
      const Series y = vec(yv, cfg);

      reg::GLMOptions opt;
      opt.family = reg::GLMFamily::Binomial;
      opt.link = reg::GLMLink::Logit;
      const auto fit = reg::glm_fit(y, x, opt, cfg);
      const auto coef = fit.coefficients.to(pluss::pCPU, ScalarType::Float64).host<double>();
      expect(fit.converged, "glm binomial converged");
      expect(approx(coef[0], -0.7, 0.35), "glm binomial intercept");
      expect(approx(coef[1], 1.8, 0.35), "glm binomial slope");
      const Series pred = reg::glm_predict(fit, x, reg::GLMPredictionType::Mean, cfg);
      expect(pred.numel() == x.numel(), "glm binomial predict shape");
    }

    {
      std::vector<double> xv;
      std::vector<double> yv;
      for (std::int64_t i = 1; i <= 16; ++i) {
        const double x = static_cast<double>(i) / 2.0;
        xv.push_back(x);
        yv.push_back(std::exp(0.4 + 0.18 * x));
      }
      const Series x = mat(xv, static_cast<std::int64_t>(xv.size()), 1, cfg);
      const Series y = vec(yv, cfg);

      reg::GLMOptions opt;
      opt.family = reg::GLMFamily::Poisson;
      opt.link = reg::GLMLink::Log;
      const auto fit = reg::glm_fit(y, x, opt, cfg);
      const auto coef = fit.coefficients.to(pluss::pCPU, ScalarType::Float64).host<double>();
      expect(fit.converged, "glm poisson converged");
      expect(approx(coef[0], 0.4, 0.30), "glm poisson intercept");
      expect(approx(coef[1], 0.18, 0.20), "glm poisson slope");
    }

    {
      // intercept-only models have closed-form maximum likelihood estimates: logit of the mean, log of the mean
      const std::vector<double> yb{0, 1, 1, 0, 1, 1, 0, 1};
      reg::GLMOptions ob;
      ob.family = reg::GLMFamily::Binomial;
      const auto fb = reg::glm_fit(pluss::make_series(yb, {8}).to(pluss::ScalarType::Float64), pluss::Series::zeros({8, 0}, pluss::ScalarType::Float64), ob);
      expect(std::abs(fb.coefficients.host<double>().front() - std::log(0.625 / 0.375)) < 1e-7, "intercept-only logistic gives logit of the mean");
      const std::vector<double> yp{2, 0, 3, 1, 4, 2};
      reg::GLMOptions op;
      op.family = reg::GLMFamily::Poisson;
      const auto fp = reg::glm_fit(pluss::make_series(yp, {6}).to(pluss::ScalarType::Float64), pluss::Series::zeros({6, 0}, pluss::ScalarType::Float64), op);
      expect(std::abs(fp.coefficients.host<double>().front() - std::log(2.0)) < 1e-7, "intercept-only Poisson gives log of the mean");
    }

    {
      // a default-constructed fit has nothing to report: the summary names the missing coefficients instead of failing inside the formatter
      bool threw = false;
      try {
        (void)reg::glm_summary(reg::GLMFitResult{});
      } catch (const std::exception& e) {
        threw = std::string(e.what()).find("print_glm_summary: fit.coefficients is undefined") != std::string::npos;
      }
      expect(threw, "glm_summary of an undefined fit reports the undefined coefficients");
    }

    std::cout << "glm_regression_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "glm_regression_test failed: " << e.what() << '\n';
    return 1;
  }
}
