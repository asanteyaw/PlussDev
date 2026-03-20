#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace pluss::subsp::math::finance::pricing {
namespace {

using volatility::SabrParams;

constexpr double kInvSqrt2 = 0.70710678118654752440;
constexpr double kInvSqrt2Pi = 0.39894228040143267794;

double norm_cdf_(double x) {
  return 0.5 * std::erfc(-x * kInvSqrt2);
}

double norm_pdf_(double x) {
  return kInvSqrt2Pi * std::exp(-0.5 * x * x);
}

double x_of_z_(double z, double rho) {
  const double rad = std::max(0.0, 1.0 - 2.0 * rho * z + z * z);
  const double num = std::sqrt(rad) + z - rho;
  const double den = 1.0 - rho;
  if (!(num > 0.0) || !(den > 0.0)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return std::log(num / den);
}

double z_over_x_(double z, double rho) {
  if (std::abs(z) <= 1e-12) {
    return 1.0;
  }
  const double x = x_of_z_(z, rho);
  if (!std::isfinite(x) || std::abs(x) <= 1e-14) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return z / x;
}

void validate_sabr_(double forward,
                    double strike,
                    double maturity,
                    const SabrParams& params,
                    const SabrApproxOptions& options,
                    const char* op) {
  const std::string prefix = std::string("subsp::math::finance::pricing::") + op + ": ";
  PLUSS_CHECK(std::isfinite(forward), prefix + "forward must be finite");
  PLUSS_CHECK(std::isfinite(strike), prefix + "strike must be finite");
  PLUSS_CHECK(std::isfinite(maturity) && maturity >= 0.0, prefix + "maturity must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(params.alpha0) && params.alpha0 > 0.0, prefix + "alpha0 must be finite and > 0");
  PLUSS_CHECK(std::isfinite(params.beta) && params.beta >= 0.0 && params.beta <= 1.0, prefix + "beta must be in [0,1]");
  PLUSS_CHECK(std::isfinite(params.nu) && params.nu >= 0.0, prefix + "nu must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(params.rho) && params.rho > -1.0 && params.rho < 1.0, prefix + "rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(options.shift), prefix + "shift must be finite");
}

double bachelier_price_(OptionType type,
                        double forward,
                        double strike,
                        double rate,
                        double maturity,
                        double normal_vol) {
  const double disc = std::exp(-rate * maturity);
  if (!(maturity > 0.0) || !(normal_vol > 0.0) || !std::isfinite(normal_vol)) {
    const double intrinsic = (type == OptionType::Call) ? std::max(forward - strike, 0.0) : std::max(strike - forward, 0.0);
    return disc * intrinsic;
  }
  const double st = normal_vol * std::sqrt(maturity);
  const double d = (forward - strike) / st;
  if (type == OptionType::Call) {
    return disc * ((forward - strike) * norm_cdf_(d) + st * norm_pdf_(d));
  }
  return disc * ((strike - forward) * norm_cdf_(-d) + st * norm_pdf_(d));
}

} // namespace

double sabr_black_vol(double forward,
                      double strike,
                      double maturity,
                      const SabrParams& params,
                      const SabrApproxOptions& options) {
  validate_sabr_(forward, strike, maturity, params, options, "sabr_black_vol");
  if (maturity <= 0.0) {
    return 0.0;
  }

  const double alpha = params.alpha0;
  const double beta = params.beta;
  const double rho = params.rho;
  const double nu = params.nu;
  const double f = forward + options.shift;
  const double k = strike + options.shift;
  const double beta1_tol = 1e-12;
  const double atm_tol = std::max(1e-12, 1e-12 * std::max(std::abs(f), std::abs(k)));

  PLUSS_CHECK(f > 0.0, "subsp::math::finance::pricing::sabr_black_vol: forward + shift must be > 0");
  PLUSS_CHECK(k > 0.0, "subsp::math::finance::pricing::sabr_black_vol: strike + shift must be > 0");

  if (std::abs(f - k) <= atm_tol) {
    const double v1 = std::pow(f, 1.0 - beta);
    const double v2 = ((1.0 - beta) * (1.0 - beta) * alpha * alpha) / (24.0 * v1 * v1) +
                      0.25 * rho * beta * nu * alpha / v1 +
                      ((2.0 - 3.0 * rho * rho) * nu * nu) / 24.0;
    return alpha * (1.0 + v2 * maturity) / v1;
  }

  const double log_fk = std::log(f / k);
  if (std::abs(1.0 - beta) <= beta1_tol) {
    const double z = (nu / alpha) * log_fk;
    const double ratio = z_over_x_(z, rho);
    const double v2 = 0.25 * rho * alpha * nu + ((2.0 - 3.0 * rho * rho) * nu * nu) / 24.0;
    return alpha * ratio * (1.0 + v2 * maturity);
  }

  const double v1 = std::pow(f * k, 0.5 * (1.0 - beta));
  const double v2 = ((1.0 - beta) * (1.0 - beta) * alpha * alpha) / (24.0 * v1 * v1) +
                    0.25 * rho * beta * nu * alpha / v1 +
                    ((2.0 - 3.0 * rho * rho) * nu * nu) / 24.0;

  if (options.model == SabrApproxModel::Obloj2008 && nu > 1e-14) {
    const double pow_diff = std::pow(f, 1.0 - beta) - std::pow(k, 1.0 - beta);
    const double z = (nu / alpha) * (pow_diff / (1.0 - beta));
    const double ratio = z_over_x_(z, rho);
    const double pre = alpha * log_fk * (1.0 - beta) / pow_diff;
    return pre * ratio * (1.0 + v2 * maturity);
  }

  const double z = (nu / alpha) * v1 * log_fk;
  const double ratio = z_over_x_(z, rho);
  const double v3 = ((1.0 - beta) * (1.0 - beta) * log_fk * log_fk) / 24.0 +
                    (std::pow(1.0 - beta, 4.0) * std::pow(log_fk, 4.0)) / 1920.0;
  return alpha * ratio * (1.0 + v2 * maturity) / (v1 * (1.0 + v3));
}

double sabr_normal_vol(double forward,
                       double strike,
                       double maturity,
                       const SabrParams& params,
                       const SabrApproxOptions& options) {
  validate_sabr_(forward, strike, maturity, params, options, "sabr_normal_vol");
  if (maturity <= 0.0) {
    return 0.0;
  }
  PLUSS_CHECK(std::abs(options.shift) <= 1e-16,
              "subsp::math::finance::pricing::sabr_normal_vol: shift must be 0 for normal-vol approximation");

  const double alpha = params.alpha0;
  const double beta = params.beta;
  const double rho = params.rho;
  const double nu = params.nu;
  const double f = forward;
  const double k = strike;
  const double beta0_tol = 1e-15;
  const double beta1_tol = 1e-7;
  const double atm_tol = 1e-6;

  if (beta > beta0_tol) {
    PLUSS_CHECK(f > 0.0, "subsp::math::finance::pricing::sabr_normal_vol: forward must be > 0 when beta > 0");
    PLUSS_CHECK(k > 0.0, "subsp::math::finance::pricing::sabr_normal_vol: strike must be > 0 when beta > 0");
  }

  const bool is_atm = std::abs(f - k) <= atm_tol;
  if (beta <= beta0_tol) {
    const double v2 = 1.0 + ((2.0 - 3.0 * rho * rho) * nu * nu / 24.0) * maturity;
    if (is_atm) {
      return alpha * v2;
    }
    const double z = (nu / alpha) * (f - k);
    const double ratio = z_over_x_(z, rho);
    return alpha * v2 * ratio;
  }

  if (beta >= (1.0 - beta1_tol)) {
    const double v2 = 1.0 + (-alpha * alpha / 24.0 + 0.25 * rho * nu * alpha + ((2.0 - 3.0 * rho * rho) * nu * nu) / 24.0) *
                                maturity;
    if (is_atm) {
      return alpha * f * v2;
    }
    const double log_fk = std::log(f / k);
    const double z = (nu / alpha) * log_fk;
    const double ratio = z_over_x_(z, rho);
    return alpha * (f - k) * ratio / log_fk * v2;
  }

  if (is_atm) {
    const double v1 = std::pow(f, 1.0 - beta);
    const double v2 = 1.0 + (beta * (beta - 2.0) * alpha * alpha / (24.0 * v1 * v1) +
                              0.25 * rho * beta * nu * alpha / v1 +
                              ((2.0 - 3.0 * rho * rho) * nu * nu) / 24.0) *
                                 maturity;
    return alpha * std::pow(f, beta) * v2;
  }

  const double v1 = std::pow(0.5 * (f + k), 1.0 - beta);
  const double pow_diff = std::pow(f, 1.0 - beta) - std::pow(k, 1.0 - beta);
  const double z = (nu / alpha) * pow_diff / (1.0 - beta);
  const double ratio = z_over_x_(z, rho);
  const double v2 = 1.0 + (beta * (beta - 2.0) * alpha * alpha / (24.0 * v1 * v1) +
                            0.25 * rho * beta * nu * alpha / v1 +
                            ((2.0 - 3.0 * rho * rho) * nu * nu) / 24.0) *
                               maturity;
  return alpha * (f - k) * (1.0 - beta) * ratio / pow_diff * v2;
}

double sabr_price_approx(OptionType type,
                         double forward,
                         double strike,
                         double rate,
                         double maturity,
                         const SabrParams& params,
                         SabrVolType vol_type,
                         const SabrApproxOptions& options) {
  validate_sabr_(forward, strike, maturity, params, options, "sabr_price_approx");
  PLUSS_CHECK(std::isfinite(rate), "subsp::math::finance::pricing::sabr_price_approx: rate must be finite");
  PLUSS_CHECK(type == OptionType::Call || type == OptionType::Put,
              "subsp::math::finance::pricing::sabr_price_approx: unsupported option type");

  if (maturity <= 0.0) {
    return (type == OptionType::Call) ? std::max(forward - strike, 0.0) : std::max(strike - forward, 0.0);
  }

  if (vol_type == SabrVolType::Normal) {
    const double sigma_n = sabr_normal_vol(forward, strike, maturity, params, options);
    return bachelier_price_(type, forward, strike, rate, maturity, sigma_n);
  }

  const double sigma_b = sabr_black_vol(forward, strike, maturity, params, options);
  return black_scholes_price(type, forward, strike, rate, rate, maturity, sigma_b);
}

} // namespace pluss::subsp::math::finance::pricing
