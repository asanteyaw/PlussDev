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

    std::cout << "glm_regression_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "glm_regression_test failed: " << e.what() << '\n';
    return 1;
  }
}
