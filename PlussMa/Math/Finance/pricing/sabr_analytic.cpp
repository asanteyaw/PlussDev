#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace pluss::subsp::math::finance::pricing {
using namespace detail;
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

double detail::sabr_black_vol(double forward,
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

double detail::sabr_normal_vol(double forward,
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

double detail::sabr_price_approx(OptionType type,
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
              "subsp::math::finance::pricing::sabr_price_approx: option type must be Call or Put (OptionType::Auto is only valid for an instrument that supplies is_call)");

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

ScalarGreeks detail::sabr_greeks(OptionType type, double spot, double strike, double rate, double dividend_yield, double maturity, const SabrParams& params,
                                 SabrVolType vol_type, const SabrApproxOptions& options) {
  const double forward = spot * std::exp((rate - dividend_yield) * maturity);
  ScalarGreeks g;
  g.price = sabr_price_approx(type, forward, strike, rate, maturity, params, vol_type, options);
  if (!(maturity > 0.0)) {
    g.delta = type == OptionType::Call ? (spot > strike ? 1.0 : 0.0) : (spot < strike ? -1.0 : 0.0);
    return g;
  }
  // The price is G(S, tau) = Price(F, K, r, tau, sigma(F, tau, alpha0)) with the forward F = S exp((r - q) tau). The smile sigma(F, tau, alpha0) is a smooth scalar
  // function: its derivatives come from fourth-order stencils on the expansion itself (no pricing involved), and the derivatives of Black's (or Bachelier's)
  // formula are exact.
  const bool normal = vol_type == SabrVolType::Normal;
  const auto sigma_of = [&](double f, double t, double a) {
    SabrParams p = params;
    p.alpha0 = a;
    return normal ? sabr_normal_vol(f, strike, t, p, options) : sabr_black_vol(f, strike, t, p, options);
  };
  const auto stencil = [&](double x, double h, const auto& value, double& d1, double& d2) {
    const double f0 = value(x), fp1 = value(x + h), fm1 = value(x - h), fp2 = value(x + 2.0 * h), fm2 = value(x - 2.0 * h);
    d1 = (-fp2 + 8.0 * fp1 - 8.0 * fm1 + fm2) / (12.0 * h);
    d2 = (-fp2 + 16.0 * fp1 - 30.0 * f0 + 16.0 * fm1 - fm2) / (12.0 * h * h);
  };
  double sig_f = 0.0, sig_ff = 0.0, sig_t = 0.0, sig_a = 0.0, unused = 0.0;
  stencil(forward, 1e-3 * forward, [&](double f) { return sigma_of(f, maturity, params.alpha0); }, sig_f, sig_ff);
  stencil(maturity, 1e-3 * maturity, [&](double t) { return sigma_of(forward, t, params.alpha0); }, sig_t, unused);
  stencil(params.alpha0, 1e-3 * params.alpha0, [&](double a) { return sigma_of(forward, maturity, a); }, sig_a, unused);
  const double sigma = sigma_of(forward, maturity, params.alpha0);

  const double disc = std::exp(-rate * maturity), sqrt_t = std::sqrt(maturity), st = sigma * sqrt_t;
  const bool call = type == OptionType::Call;
  double b_f, b_ff, b_s, b_fs, b_ss, b_t;   // derivatives of the price formula: in F, twice in F, in sigma, mixed, twice in sigma, explicit in tau
  if (!normal) {
    const double d1 = (std::log(forward / strike) + 0.5 * sigma * sigma * maturity) / st, d2 = d1 - st, phi = norm_pdf_(d1);
    b_f = call ? disc * norm_cdf_(d1) : -disc * norm_cdf_(-d1);
    b_ff = disc * phi / (forward * st);
    b_s = disc * forward * phi * sqrt_t;
    b_fs = -disc * phi * d2 / sigma;
    b_ss = b_s * d1 * d2 / sigma;
    b_t = -rate * g.price + disc * forward * phi * sigma / (2.0 * sqrt_t);
  } else {
    const double d = (forward - strike) / st, phi = norm_pdf_(d);
    b_f = call ? disc * norm_cdf_(d) : -disc * norm_cdf_(-d);
    b_ff = disc * phi / st;
    b_s = disc * sqrt_t * phi;
    b_fs = -disc * phi * d / sigma;
    b_ss = b_s * d * d / sigma;
    b_t = -rate * g.price + disc * sigma * phi / (2.0 * sqrt_t);
  }
  const double g_f = b_f + b_s * sig_f;
  const double g_ff = b_ff + 2.0 * b_fs * sig_f + b_ss * sig_f * sig_f + b_s * sig_ff;
  const double growth = forward / spot;
  g.delta = g_f * growth;
  g.gamma = g_ff * growth * growth;
  g.vega = b_s * sig_a;
  g.theta = -(b_t + b_s * sig_t + g_f * (rate - dividend_yield) * forward);
  g.rho = -maturity * g.price + g_f * maturity * forward;
  return g;
}

} // namespace pluss::subsp::math::finance::pricing
