#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::pricing {
namespace {

using pluss::Config;
using pluss::Series;
using volatility::BatesParams;
using volatility::HestonParams;
using volatility::HestonNandiParams;
using volatility::SabrParams;
using volatility::SlvParams;
using volatility::SvParams;
using volatility::RoughHestonParams;
using volatility::RoughBergomiParams;

constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kEps = 1e-12;

void validate_option_(OptionType type, const char* op) {
  PLUSS_CHECK(type == OptionType::Call || type == OptionType::Put,
              std::string("subsp::math::finance::pricing::") + op + ": unsupported option type");
}

void validate_common_(double spot,
                      double strike,
                      double rate,
                      double dividend_yield,
                      double maturity,
                      const char* op) {
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": spot must be finite and > 0");
  PLUSS_CHECK(std::isfinite(strike) && strike > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": strike must be finite and > 0");
  PLUSS_CHECK(std::isfinite(rate), std::string("subsp::math::finance::pricing::") + op + ": rate must be finite");
  PLUSS_CHECK(std::isfinite(dividend_yield),
              std::string("subsp::math::finance::pricing::") + op + ": dividend_yield must be finite");
  PLUSS_CHECK(std::isfinite(maturity) && maturity >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": maturity must be finite and >= 0");
}

void validate_heston_(const HestonParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.kappa) && p.kappa > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": kappa must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.theta) && p.theta > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": theta must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.sigma) && p.sigma > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": sigma must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(p.v0) && p.v0 > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": v0 must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.variance_floor) && p.variance_floor > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": variance_floor must be finite and > 0");
}

void validate_sv_(const SvParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.phi) && std::abs(p.phi) < 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": abs(phi) must be < 1");
  PLUSS_CHECK(std::isfinite(p.vol_of_logvar) && p.vol_of_logvar > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": vol_of_logvar must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(p.variance_floor) && p.variance_floor > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": variance_floor must be finite and > 0");
}

void validate_heston_nandi_(const HestonNandiParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.omega) && p.omega > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": omega must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.alpha) && p.alpha >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": alpha must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.beta) && p.beta >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": beta must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.gamma), std::string("subsp::math::finance::pricing::") + op + ": gamma must be finite");
  PLUSS_CHECK(std::isfinite(p.lambda), std::string("subsp::math::finance::pricing::") + op + ": lambda must be finite");
  PLUSS_CHECK(std::isfinite(p.r), std::string("subsp::math::finance::pricing::") + op + ": r must be finite");
  PLUSS_CHECK(std::isfinite(p.variance_floor) && p.variance_floor > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": variance_floor must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.stationarity_margin) && p.stationarity_margin > 0.0 && p.stationarity_margin < 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": stationarity_margin must be in (0,1)");
  const double persistence = p.beta + p.alpha * p.gamma * p.gamma;
  PLUSS_CHECK(persistence < (1.0 - p.stationarity_margin),
              std::string("subsp::math::finance::pricing::") + op +
                  ": requires beta + alpha*gamma^2 < 1 - stationarity_margin");
}

void validate_bates_(const BatesParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.kappa) && p.kappa > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": kappa must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.theta) && p.theta > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": theta must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.sigma) && p.sigma > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": sigma must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(p.v0) && p.v0 > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": v0 must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.variance_floor) && p.variance_floor > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": variance_floor must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.jump_intensity) && p.jump_intensity >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": jump_intensity must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.jump_mean),
              std::string("subsp::math::finance::pricing::") + op + ": jump_mean must be finite");
  PLUSS_CHECK(std::isfinite(p.jump_vol) && p.jump_vol >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": jump_vol must be finite and >= 0");
}

void validate_sabr_(const SabrParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.alpha0) && p.alpha0 > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": alpha0 must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.beta) && p.beta >= 0.0 && p.beta <= 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": beta must be in [0,1]");
  PLUSS_CHECK(std::isfinite(p.nu) && p.nu >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": nu must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": rho must be in (-1,1)");
}

void validate_slv_(const SlvParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.kappa) && p.kappa > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": kappa must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.theta) && p.theta > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": theta must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.sigma) && p.sigma >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": sigma must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(p.v0) && p.v0 > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": v0 must be finite and > 0");
}

void validate_rough_heston_(const RoughHestonParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.kappa) && p.kappa >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": kappa must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.theta) && p.theta >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": theta must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.sigma) && p.sigma >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": sigma must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.alpha) && p.alpha > -0.5 && p.alpha < 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": alpha must be in (-0.5,0)");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": rho must be in (-1,1)");
  PLUSS_CHECK(std::isfinite(p.v0) && p.v0 >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": v0 must be finite and >= 0");
}

void validate_rough_bergomi_(const RoughBergomiParams& p, const char* op) {
  PLUSS_CHECK(std::isfinite(p.xi) && p.xi >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": xi must be finite and >= 0");
  PLUSS_CHECK(std::isfinite(p.eta) && p.eta > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": eta must be finite and > 0");
  PLUSS_CHECK(std::isfinite(p.alpha) && p.alpha > -0.5 && p.alpha < 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": alpha must be in (-0.5,0)");
  PLUSS_CHECK(std::isfinite(p.rho) && p.rho > -1.0 && p.rho < 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": rho must be in (-1,1)");
}

void validate_mc_(const MonteCarloOptions& opt, const char* op) {
  PLUSS_CHECK(opt.paths > 0,
              std::string("subsp::math::finance::pricing::") + op + ": monte-carlo paths must be > 0");
  PLUSS_CHECK(opt.steps > 0,
              std::string("subsp::math::finance::pricing::") + op + ": monte-carlo steps must be > 0");
  PLUSS_CHECK(std::isfinite(opt.importance_shift),
              std::string("subsp::math::finance::pricing::") + op + ": importance_shift must be finite");
  PLUSS_CHECK(std::isfinite(opt.control_vol) && opt.control_vol >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": control_vol must be finite and >= 0");
}

void validate_heston_integration_(const HestonIntegrationOptions& opt, const char* op) {
  PLUSS_CHECK(opt.integration_steps > 0,
              std::string("subsp::math::finance::pricing::") + op + ": integration_steps must be > 0");
  PLUSS_CHECK(std::isfinite(opt.integration_u_max) && opt.integration_u_max > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": integration_u_max must be finite and > 0");
}

void validate_heston_fft_(const HestonFftOptions& opt, const char* op) {
  PLUSS_CHECK(opt.num_fft >= 2,
              std::string("subsp::math::finance::pricing::") + op + ": num_fft must be >= 2");
  PLUSS_CHECK(std::isfinite(opt.characteristic_fcn_step) && opt.characteristic_fcn_step > 0.0,
              std::string("subsp::math::finance::pricing::") + op +
                  ": characteristic_fcn_step must be finite and > 0");
  PLUSS_CHECK((std::isnan(opt.log_strike_step) || (std::isfinite(opt.log_strike_step) && opt.log_strike_step > 0.0)),
              std::string("subsp::math::finance::pricing::") + op +
                  ": log_strike_step must be NaN or a finite value > 0");
  PLUSS_CHECK(std::isfinite(opt.damping_factor) && opt.damping_factor > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": damping_factor must be finite and > 0");
  PLUSS_CHECK(std::isfinite(opt.vol_risk_premium),
              std::string("subsp::math::finance::pricing::") + op + ": vol_risk_premium must be finite");
}

void validate_cos_options_(const CosOptions& opt, const char* op) {
  PLUSS_CHECK(opt.num_terms >= 2,
              std::string("subsp::math::finance::pricing::") + op + ": num_terms must be >= 2");
  PLUSS_CHECK(std::isfinite(opt.truncation) && opt.truncation > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": truncation must be finite and > 0");
  PLUSS_CHECK(std::isfinite(opt.cumulant_step) && opt.cumulant_step > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": cumulant_step must be finite and > 0");
}

void validate_heston_nandi_integration_(const HestonNandiIntegrationOptions& opt, const char* op) {
  PLUSS_CHECK(opt.integration_steps > 0,
              std::string("subsp::math::finance::pricing::") + op + ": integration_steps must be > 0");
  PLUSS_CHECK(std::isfinite(opt.integration_u_max) && opt.integration_u_max > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": integration_u_max must be finite and > 0");
  PLUSS_CHECK(opt.periods_per_year > 0,
              std::string("subsp::math::finance::pricing::") + op + ": periods_per_year must be > 0");
}

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

std::complex<double> heston_cf_(std::complex<double> phi,
                                double spot,
                                double rate,
                                double dividend_yield,
                                double tau,
                                const HestonParams& p,
                                HestonCfMode mode,
                                int numeraire,
                                double vol_risk_premium) {
  using C = std::complex<double>;
  const C i(0.0, 1.0);

  const double x = std::log(spot);
  const double u = (numeraire == 1) ? 0.5 : -0.5;
  const double b = (numeraire == 1) ? (p.kappa + vol_risk_premium - p.rho * p.sigma)
                                    : (p.kappa + vol_risk_premium);
  const double a = p.kappa * p.theta;

  const C d = std::sqrt(std::pow(b - p.rho * p.sigma * i * phi, 2.0) - p.sigma * p.sigma * (2.0 * u * i * phi - phi * phi));
  const C b_rho_i_phi = b - p.rho * p.sigma * i * phi;

  C c_term;
  C d_term;

  if (mode == HestonCfMode::LittleTrap) {
    const C g = (b_rho_i_phi - d) / (b_rho_i_phi + d);
    const C exp_md_tau = std::exp(-d * tau);
    const C log_term = std::log((C(1.0, 0.0) - g * exp_md_tau) / (C(1.0, 0.0) - g));
    c_term = (rate - dividend_yield) * i * phi * tau +
             (a / (p.sigma * p.sigma)) * ((b_rho_i_phi - d) * tau - C(2.0, 0.0) * log_term);
    d_term = ((b_rho_i_phi - d) / (p.sigma * p.sigma)) *
             ((C(1.0, 0.0) - exp_md_tau) / (C(1.0, 0.0) - g * exp_md_tau));
  } else {
    const C g = (b_rho_i_phi + d) / (b_rho_i_phi - d);
    const C exp_d_tau = std::exp(d * tau);
    const C log_term = std::log((C(1.0, 0.0) - g * exp_d_tau) / (C(1.0, 0.0) - g));
    c_term = (rate - dividend_yield) * i * phi * tau +
             (a / (p.sigma * p.sigma)) * ((b_rho_i_phi + d) * tau - C(2.0, 0.0) * log_term);
    d_term = ((b_rho_i_phi + d) / (p.sigma * p.sigma)) *
             ((C(1.0, 0.0) - exp_d_tau) / (C(1.0, 0.0) - g * exp_d_tau));
  }

  return std::exp(c_term + d_term * p.v0 + i * phi * x);
}

std::complex<double> bates_cf_(std::complex<double> phi,
                               double spot,
                               double rate,
                               double dividend_yield,
                               double tau,
                               const BatesParams& p,
                               HestonCfMode mode,
                               double vol_risk_premium) {
  const double kappa_jump = std::exp(p.jump_mean + 0.5 * p.jump_vol * p.jump_vol) - 1.0;
  const double drift_adj_rate = rate - p.jump_intensity * kappa_jump;
  const HestonParams hp{
      .kappa = p.kappa,
      .theta = p.theta,
      .sigma = p.sigma,
      .rho = p.rho,
      .v0 = p.v0,
      .mu = p.mu,
      .variance_floor = p.variance_floor,
  };
  const auto h_cf = heston_cf_(phi, spot, drift_adj_rate, dividend_yield, tau, hp, mode, 2, vol_risk_premium);
  const std::complex<double> i(0.0, 1.0);
  const auto jump_cf =
      std::exp(p.jump_intensity * tau * (std::exp(i * phi * p.jump_mean - 0.5 * p.jump_vol * p.jump_vol * phi * phi) - 1.0));
  return h_cf * jump_cf;
}

double simpson_integral_(const std::function<double(double)>& f,
                         double a,
                         double b,
                         std::int64_t n) {
  if (n % 2 != 0) {
    ++n;
  }
  const double h = (b - a) / static_cast<double>(n);

  double sum = f(a) + f(b);
  for (std::int64_t i = 1; i < n; ++i) {
    const double x = a + h * static_cast<double>(i);
    sum += ((i % 2 == 0) ? 2.0 : 4.0) * f(x);
  }
  return (h / 3.0) * sum;
}

double prob_j_(int numeraire,
               double spot,
               double strike,
               double rate,
               double dividend_yield,
               double maturity,
               const HestonParams& params,
               const HestonIntegrationOptions& options) {
  const double logk = std::log(strike);
  using C = std::complex<double>;
  const C i(0.0, 1.0);

  auto integrand = [&](double u) -> double {
    const C phi = C(u, 0.0);
    const C cf = heston_cf_(phi, spot, rate, dividend_yield, maturity, params, options.mode, numeraire, 0.0);
    const C num = std::exp(-i * phi * logk) * cf;
    const C den = i * phi;
    return std::real(num / den);
  };

  const double u_lo = 1e-8;
  const double integral = simpson_integral_(integrand, u_lo, options.integration_u_max, options.integration_steps);
  const double p = 0.5 + integral / kPi;
  return std::clamp(p, 0.0, 1.0);
}

double intrinsic_(OptionType type, double spot, double strike) {
  if (type == OptionType::Call) {
    return std::max(0.0, spot - strike);
  }
  return std::max(0.0, strike - spot);
}

Series discounted_payoff_(OptionType type, const Series& terminal, double strike, double rate, double maturity) {
  const double disc = std::exp(-rate * maturity);
  if (type == OptionType::Call) {
    return terminal.sub(strike).clamp_min(0.0).mul(disc);
  }
  return terminal.mul(-1.0).add(strike).clamp_min(0.0).mul(disc);
}

Series importance_lr_(const Series& innovation_spot, double shift) {
  const auto s = innovation_spot.sizes();
  PLUSS_CHECK(s.size() == 2, "subsp::math::finance::pricing: innovation_spot must be matrix");
  const std::int64_t steps = s[0];
  const std::int64_t paths = s[1];
  if (std::abs(shift) <= kEps) {
    return pluss::full({paths}, 1.0, Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(innovation_spot.platform()));
  }
  const Series sum_z = innovation_spot.sum(0, false).reshape({paths});
  const double c = 0.5 * static_cast<double>(steps) * shift * shift;
  return sum_z.mul(-shift).add(c).exp();
}

double mc_cv_estimate_(const Series& target, const Series& control, double control_mean) {
  const Series centered = control.sub(control_mean);
  const double var_c = centered.square().mean().item<double>();
  if (!std::isfinite(var_c) || var_c <= kEps) {
    return target.mean().item<double>();
  }
  const double cov_tc = target.mul(centered).mean().item<double>();
  const double beta = cov_tc / var_c;
  return target.sub(centered.mul(beta)).mean().item<double>();
}

Series bs_control_discounted_payoff_(OptionType type,
                                     const Series& innovation_spot,
                                     double spot,
                                     double strike,
                                     double rate,
                                     double dividend_yield,
                                     double maturity,
                                     double sigma) {
  const auto s = innovation_spot.sizes();
  PLUSS_CHECK(s.size() == 2, "subsp::math::finance::pricing: innovation_spot must be matrix");
  const std::int64_t steps = s[0];
  const double dt = maturity / static_cast<double>(steps);
  const Series w_t = innovation_spot.sum(0, false).mul(std::sqrt(dt));
  const double drift = (rate - dividend_yield - 0.5 * sigma * sigma) * maturity;
  const Series s_t = w_t.mul(sigma).add(drift).exp().mul(spot);
  return discounted_payoff_(type, s_t, strike, rate, maturity);
}

double positive_or_(double v, double fallback) {
  return (std::isfinite(v) && v > 0.0) ? v : fallback;
}

bool is_pow2_(std::int64_t n) {
  return n > 0 && (n & (n - 1)) == 0;
}

std::int64_t next_pow2_(std::int64_t n) {
  std::int64_t out = 1;
  while (out < n) {
    out <<= 1;
  }
  return out;
}

void fft_radix2_(std::vector<std::complex<double>>& a, bool inverse) {
  const auto n = static_cast<std::int64_t>(a.size());
  for (std::int64_t i = 1, j = 0; i < n; ++i) {
    std::int64_t bit = n >> 1;
    while (j & bit) {
      j ^= bit;
      bit >>= 1;
    }
    j ^= bit;
    if (i < j) {
      std::swap(a[static_cast<std::size_t>(i)], a[static_cast<std::size_t>(j)]);
    }
  }

  for (std::int64_t len = 2; len <= n; len <<= 1) {
    const double ang = (inverse ? 2.0 : -2.0) * kPi / static_cast<double>(len);
    const std::complex<double> wlen(std::cos(ang), std::sin(ang));
    for (std::int64_t i = 0; i < n; i += len) {
      std::complex<double> w(1.0, 0.0);
      const auto half = len >> 1;
      for (std::int64_t j = 0; j < half; ++j) {
        const auto u = a[static_cast<std::size_t>(i + j)];
        const auto v = a[static_cast<std::size_t>(i + j + half)] * w;
        a[static_cast<std::size_t>(i + j)] = u + v;
        a[static_cast<std::size_t>(i + j + half)] = u - v;
        w *= wlen;
      }
    }
  }

  if (inverse) {
    const double inv_n = 1.0 / static_cast<double>(n);
    for (auto& z : a) {
      z *= inv_n;
    }
  }
}

void dft_fallback_(std::vector<std::complex<double>>& a, bool inverse) {
  const auto n = static_cast<std::int64_t>(a.size());
  std::vector<std::complex<double>> src = a;
  const double sign = inverse ? 1.0 : -1.0;
  for (std::int64_t k = 0; k < n; ++k) {
    std::complex<double> acc(0.0, 0.0);
    for (std::int64_t t = 0; t < n; ++t) {
      const double ang = sign * 2.0 * kPi * static_cast<double>(k * t) / static_cast<double>(n);
      acc += src[static_cast<std::size_t>(t)] * std::complex<double>(std::cos(ang), std::sin(ang));
    }
    a[static_cast<std::size_t>(k)] = acc;
  }
  if (inverse) {
    const double inv_n = 1.0 / static_cast<double>(n);
    for (auto& z : a) {
      z *= inv_n;
    }
  }
}

void fft_any_(std::vector<std::complex<double>>& a, bool inverse) {
  if (is_pow2_(static_cast<std::int64_t>(a.size()))) {
    fft_radix2_(a, inverse);
    return;
  }
  dft_fallback_(a, inverse);
}

std::vector<std::complex<double>> frft_(const std::vector<std::complex<double>>& x, double a) {
  const auto n = static_cast<std::int64_t>(x.size());
  const std::int64_t m = next_pow2_(2 * n - 1);
  std::vector<std::complex<double>> u(static_cast<std::size_t>(m), {0.0, 0.0});
  std::vector<std::complex<double>> v(static_cast<std::size_t>(m), {0.0, 0.0});

  for (std::int64_t j = 0; j < n; ++j) {
    const double phase = -kPi * a * static_cast<double>(j * j);
    u[static_cast<std::size_t>(j)] = x[static_cast<std::size_t>(j)] * std::complex<double>(std::cos(phase), std::sin(phase));
  }
  for (std::int64_t t = -(n - 1); t <= (n - 1); ++t) {
    const double phase = kPi * a * static_cast<double>(t * t);
    v[static_cast<std::size_t>(t + n - 1)] = std::complex<double>(std::cos(phase), std::sin(phase));
  }

  auto fu = u;
  auto fv = v;
  fft_any_(fu, false);
  fft_any_(fv, false);
  for (std::int64_t i = 0; i < m; ++i) {
    fu[static_cast<std::size_t>(i)] *= fv[static_cast<std::size_t>(i)];
  }
  fft_any_(fu, true);

  std::vector<std::complex<double>> out(static_cast<std::size_t>(n), {0.0, 0.0});
  for (std::int64_t k = 0; k < n; ++k) {
    const auto wk = fu[static_cast<std::size_t>(k + n - 1)];
    const double phase = -kPi * a * static_cast<double>(k * k);
    out[static_cast<std::size_t>(k)] = wk * std::complex<double>(std::cos(phase), std::sin(phase));
  }
  return out;
}

double interp_linear_on_log_strike_(const std::vector<double>& strikes,
                                    const std::vector<double>& prices,
                                    double strike) {
  PLUSS_CHECK(strikes.size() == prices.size() && !strikes.empty(),
              "subsp::math::finance::pricing::heston_price_fft: invalid strike/price grid");
  const double min_k = strikes.front();
  const double max_k = strikes.back();
  PLUSS_CHECK(strike >= min_k && strike <= max_k,
              "subsp::math::finance::pricing::heston_price_fft: strike must lie in FFT strike grid range");
  if (strike <= min_k + kEps) {
    return prices.front();
  }
  if (strike >= max_k - kEps) {
    return prices.back();
  }

  const double logk = std::log(strike);
  auto it = std::lower_bound(strikes.begin(), strikes.end(), strike);
  const std::size_t hi = static_cast<std::size_t>(std::distance(strikes.begin(), it));
  const std::size_t lo = hi - 1;

  const double x0 = std::log(strikes[lo]);
  const double x1 = std::log(strikes[hi]);
  const double y0 = prices[lo];
  const double y1 = prices[hi];
  const double w = (logk - x0) / (x1 - x0);
  return y0 + w * (y1 - y0);
}

Series series_from_vec_f64_(const std::vector<double>& values, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = static_cast<std::int64_t>(values.size());
  return pluss::make_series(values, {n}, out_cfg);
}

std::pair<std::vector<double>, std::vector<double>> option_fft_grid_values_(double spot,
                                                                             double rate,
                                                                             double dividend_yield,
                                                                             double maturity,
                                                                             const CharacteristicFn& char_fn,
                                                                             const HestonFftOptions& options) {
  (void)rate;
  (void)dividend_yield;
  const std::int64_t n = options.num_fft;
  const double eta = options.characteristic_fcn_step;
  const double lambda = std::isnan(options.log_strike_step)
                            ? (2.0 * kPi / static_cast<double>(n) / eta)
                            : options.log_strike_step;
  const double alpha = options.damping_factor;
  const double log_s = std::log(spot);
  const double frft_param = (lambda * eta) / (2.0 * kPi);
  const double fft_param = 1.0 / static_cast<double>(n);

  std::vector<double> weights(static_cast<std::size_t>(n), 1.0);
  if (options.quadrature == HestonFftQuadrature::Simpson) {
    weights.front() = 1.0 / 3.0;
    weights.back() = 1.0 / 3.0;
    for (std::int64_t j = 1; j < n - 1; ++j) {
      const std::int64_t idx1 = j + 1;
      weights[static_cast<std::size_t>(j)] = (3.0 + ((idx1 % 2 == 0) ? 1.0 : -1.0)) / 3.0;
    }
  } else {
    weights.front() = 0.5;
    weights.back() = 0.5;
  }

  const std::complex<double> i(0.0, 1.0);
  std::vector<std::complex<double>> x(static_cast<std::size_t>(n), {0.0, 0.0});
  for (std::int64_t j = 0; j < n; ++j) {
    const double u = eta * static_cast<double>(j);
    const std::complex<double> phi(u, 0.0);
    const auto f2 = char_fn(phi - (alpha + 1.0) * i);
    const auto den = (alpha * alpha + alpha - u * u) + i * u * (2.0 * alpha + 1.0);
    const auto psi = f2 / den;
    const auto phase = std::exp(i * phi * (static_cast<double>(n) * lambda * 0.5 - log_s));
    x[static_cast<std::size_t>(j)] = phase * psi * weights[static_cast<std::size_t>(j)];
  }

  std::vector<std::complex<double>> transform;
  if (std::abs(frft_param - fft_param) < 1e-14) {
    transform = x;
    fft_any_(transform, false);
  } else {
    transform = frft_(x, frft_param);
  }

  std::vector<double> strikes(static_cast<std::size_t>(n), 0.0);
  std::vector<double> prices(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t j = 0; j < n; ++j) {
    const double logk = log_s + (static_cast<double>(j) - static_cast<double>(n) * 0.5) * lambda;
    const double strike = std::exp(logk);
    const double call = eta * std::exp(-alpha * logk) / kPi * std::real(transform[static_cast<std::size_t>(j)]);
    strikes[static_cast<std::size_t>(j)] = strike;
    prices[static_cast<std::size_t>(j)] = call;
  }

  return {std::move(prices), std::move(strikes)};
}

std::pair<std::vector<double>, std::vector<double>> heston_fft_grid_values_(double spot,
                                                                             double rate,
                                                                             double dividend_yield,
                                                                             double maturity,
                                                                             const HestonParams& params,
                                                                             const HestonFftOptions& options) {
  const CharacteristicFn cf = [&](std::complex<double> phi) {
    return std::exp(-rate * maturity) *
           heston_cf_(phi,
                      spot,
                      rate,
                      dividend_yield,
                      maturity,
                      params,
                      options.mode,
                      2,
                      options.vol_risk_premium);
  };
  return option_fft_grid_values_(spot, rate, dividend_yield, maturity, cf, options);
}

std::pair<std::vector<double>, std::vector<double>> bates_fft_grid_values_(double spot,
                                                                            double rate,
                                                                            double dividend_yield,
                                                                            double maturity,
                                                                            const BatesParams& params,
                                                                            const HestonFftOptions& options) {
  const CharacteristicFn cf = [&](std::complex<double> phi) {
    return std::exp(-rate * maturity) *
           bates_cf_(phi, spot, rate, dividend_yield, maturity, params, options.mode, options.vol_risk_premium);
  };
  return option_fft_grid_values_(spot, rate, dividend_yield, maturity, cf, options);
}

double heston_nandi_unc_var_(const HestonNandiParams& p) {
  const double persistence = p.beta + p.alpha * p.gamma * p.gamma;
  const double denom = 1.0 - persistence;
  if (denom <= 0.0) {
    return std::max(p.variance_floor, 1.0e-6);
  }
  return std::max(p.variance_floor, (p.omega + p.alpha) / denom);
}

using Cplx = std::complex<double>;

Cplx heston_nandi_gen_fn_(Cplx q, double spot, double h0, double r_step, std::int64_t steps, const HestonNandiParams& p) {
  const Cplx half(0.5, 0.0);
  Cplx a(0.0, 0.0);
  Cplx b(0.0, 0.0);

  for (std::int64_t k = 0; k < steps; ++k) {
    const Cplx den = Cplx(1.0, 0.0) - Cplx(2.0 * p.alpha, 0.0) * b;
    if (!std::isfinite(std::abs(den)) || std::abs(den) <= kEps) {
      return Cplx(std::numeric_limits<double>::quiet_NaN(), 0.0);
    }
    a += q * r_step + b * p.omega - half * std::log(den);
    const Cplx q_minus_gamma = q - p.gamma;
    b = q * (p.lambda + p.gamma) - half * (p.gamma * p.gamma) + p.beta * b +
        half * (q_minus_gamma * q_minus_gamma) / den;
  }
  return std::exp(q * std::log(spot) + a + b * h0);
}

double heston_nandi_prob_j_(int numeraire,
                            double strike,
                            double spot,
                            double h0,
                            double r_step,
                            std::int64_t steps,
                            const HestonNandiParams& q_params,
                            const HestonNandiIntegrationOptions& options) {
  const Cplx i(0.0, 1.0);
  const double logk = std::log(strike);

  const Cplx mgf1 = heston_nandi_gen_fn_(Cplx(1.0, 0.0), spot, h0, r_step, steps, q_params);
  PLUSS_CHECK(std::isfinite(std::abs(mgf1)) && std::abs(mgf1) > kEps,
              "subsp::math::finance::pricing::heston_nandi_price: non-finite E[S_T] in transform");

  auto integrand = [&](double u) -> double {
    const Cplx phi(u, 0.0);
    const Cplx den = i * phi;
    if (std::abs(den) <= kEps) {
      return 0.0;
    }
    const Cplx exp_term = std::exp(-i * phi * logk);
    const Cplx cf = (numeraire == 1)
                        ? heston_nandi_gen_fn_(Cplx(1.0, u), spot, h0, r_step, steps, q_params) / mgf1
                        : heston_nandi_gen_fn_(Cplx(0.0, u), spot, h0, r_step, steps, q_params);
    const Cplx v = exp_term * cf / den;
    return std::real(v);
  };

  const double integral = simpson_integral_(integrand, 1e-8, options.integration_u_max, options.integration_steps);
  return std::clamp(0.5 + integral / kPi, 0.0, 1.0);
}

} // namespace

double heston_price(OptionType type,
                    double spot,
                    double strike,
                    double rate,
                    double dividend_yield,
                    double maturity,
                    const HestonParams& params,
                    const HestonIntegrationOptions& options) {
  validate_option_(type, "heston_price");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "heston_price");
  validate_heston_(params, "heston_price");
  validate_heston_integration_(options, "heston_price");

  if (maturity <= 0.0) {
    return intrinsic_(type, spot, strike);
  }

  const double p1 = prob_j_(1, spot, strike, rate, dividend_yield, maturity, params, options);
  const double p2 = prob_j_(2, spot, strike, rate, dividend_yield, maturity, params, options);
  const double disc_q = std::exp(-dividend_yield * maturity);
  const double disc_r = std::exp(-rate * maturity);
  const double call = spot * disc_q * p1 - strike * disc_r * p2;

  if (type == OptionType::Call) {
    return std::max(0.0, call);
  }

  const double parity = strike * disc_r - spot * disc_q;
  return std::max(0.0, call + parity);
}

HestonFftResult heston_price_fft_grid(OptionType type,
                                      double spot,
                                      double rate,
                                      double dividend_yield,
                                      double maturity,
                                      const HestonParams& params,
                                      const HestonFftOptions& options,
                                      Config cfg) {
  validate_option_(type, "heston_price_fft_grid");
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0,
              "subsp::math::finance::pricing::heston_price_fft_grid: spot must be finite and > 0");
  PLUSS_CHECK(std::isfinite(rate), "subsp::math::finance::pricing::heston_price_fft_grid: rate must be finite");
  PLUSS_CHECK(std::isfinite(dividend_yield),
              "subsp::math::finance::pricing::heston_price_fft_grid: dividend_yield must be finite");
  PLUSS_CHECK(std::isfinite(maturity) && maturity >= 0.0,
              "subsp::math::finance::pricing::heston_price_fft_grid: maturity must be finite and >= 0");
  validate_heston_(params, "heston_price_fft_grid");
  validate_heston_fft_(options, "heston_price_fft_grid");

  if (maturity <= 0.0) {
    const std::int64_t n = options.num_fft;
    const double eta = options.characteristic_fcn_step;
    const double lambda = std::isnan(options.log_strike_step)
                              ? (2.0 * kPi / static_cast<double>(n) / eta)
                              : options.log_strike_step;
    const double log_s = std::log(spot);
    std::vector<double> strikes(static_cast<std::size_t>(n), 0.0);
    std::vector<double> prices(static_cast<std::size_t>(n), 0.0);
    for (std::int64_t j = 0; j < n; ++j) {
      const double logk = log_s + (static_cast<double>(j) - static_cast<double>(n) * 0.5) * lambda;
      const double k = std::exp(logk);
      strikes[static_cast<std::size_t>(j)] = k;
      prices[static_cast<std::size_t>(j)] = intrinsic_(type, spot, k);
    }
    return HestonFftResult{
        .price = series_from_vec_f64_(prices, cfg),
        .strike = series_from_vec_f64_(strikes, cfg),
    };
  }

  auto [prices, strikes] = heston_fft_grid_values_(spot, rate, dividend_yield, maturity, params, options);
  if (type == OptionType::Put) {
    const double disc_q = std::exp(-dividend_yield * maturity);
    const double disc_r = std::exp(-rate * maturity);
    for (std::size_t i = 0; i < prices.size(); ++i) {
      prices[i] = prices[i] + strikes[i] * disc_r - spot * disc_q;
    }
  }
  return HestonFftResult{
      .price = series_from_vec_f64_(prices, cfg),
      .strike = series_from_vec_f64_(strikes, cfg),
  };
}

double heston_price_fft(OptionType type,
                        double spot,
                        double strike,
                        double rate,
                        double dividend_yield,
                        double maturity,
                        const HestonParams& params,
                        const HestonFftOptions& options,
                        Config cfg) {
  (void)cfg;
  validate_option_(type, "heston_price_fft");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "heston_price_fft");
  validate_heston_(params, "heston_price_fft");
  validate_heston_fft_(options, "heston_price_fft");

  if (maturity <= 0.0) {
    return intrinsic_(type, spot, strike);
  }

  const auto [call_prices, strikes] = heston_fft_grid_values_(spot, rate, dividend_yield, maturity, params, options);
  const double call = interp_linear_on_log_strike_(strikes, call_prices, strike);
  if (type == OptionType::Call) {
    return call;
  }
  const double disc_q = std::exp(-dividend_yield * maturity);
  const double disc_r = std::exp(-rate * maturity);
  return call + strike * disc_r - spot * disc_q;
}

HestonFftResult option_price_fft_grid(OptionType type,
                                      double spot,
                                      double rate,
                                      double dividend_yield,
                                      double maturity,
                                      const CharacteristicFn& char_fn,
                                      const HestonFftOptions& options,
                                      Config cfg) {
  validate_option_(type, "option_price_fft_grid");
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0,
              "subsp::math::finance::pricing::option_price_fft_grid: spot must be finite and > 0");
  PLUSS_CHECK(std::isfinite(rate), "subsp::math::finance::pricing::option_price_fft_grid: rate must be finite");
  PLUSS_CHECK(std::isfinite(dividend_yield),
              "subsp::math::finance::pricing::option_price_fft_grid: dividend_yield must be finite");
  PLUSS_CHECK(std::isfinite(maturity) && maturity >= 0.0,
              "subsp::math::finance::pricing::option_price_fft_grid: maturity must be finite and >= 0");
  PLUSS_CHECK(static_cast<bool>(char_fn),
              "subsp::math::finance::pricing::option_price_fft_grid: char_fn must be valid");
  validate_heston_fft_(options, "option_price_fft_grid");

  if (maturity <= 0.0) {
    const std::int64_t n = options.num_fft;
    const double eta = options.characteristic_fcn_step;
    const double lambda = std::isnan(options.log_strike_step)
                              ? (2.0 * kPi / static_cast<double>(n) / eta)
                              : options.log_strike_step;
    const double log_s = std::log(spot);
    std::vector<double> strikes(static_cast<std::size_t>(n), 0.0);
    std::vector<double> prices(static_cast<std::size_t>(n), 0.0);
    for (std::int64_t j = 0; j < n; ++j) {
      const double logk = log_s + (static_cast<double>(j) - static_cast<double>(n) * 0.5) * lambda;
      const double k = std::exp(logk);
      strikes[static_cast<std::size_t>(j)] = k;
      prices[static_cast<std::size_t>(j)] = intrinsic_(type, spot, k);
    }
    return HestonFftResult{
        .price = series_from_vec_f64_(prices, cfg),
        .strike = series_from_vec_f64_(strikes, cfg),
    };
  }

  auto [prices, strikes] = option_fft_grid_values_(spot, rate, dividend_yield, maturity, char_fn, options);
  if (type == OptionType::Put) {
    const double disc_q = std::exp(-dividend_yield * maturity);
    const double disc_r = std::exp(-rate * maturity);
    for (std::size_t i = 0; i < prices.size(); ++i) {
      prices[i] = prices[i] + strikes[i] * disc_r - spot * disc_q;
    }
  }
  return HestonFftResult{
      .price = series_from_vec_f64_(prices, cfg),
      .strike = series_from_vec_f64_(strikes, cfg),
  };
}

double option_price_fft(OptionType type,
                        double spot,
                        double strike,
                        double rate,
                        double dividend_yield,
                        double maturity,
                        const CharacteristicFn& char_fn,
                        const HestonFftOptions& options,
                        Config cfg) {
  (void)cfg;
  validate_option_(type, "option_price_fft");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "option_price_fft");
  PLUSS_CHECK(static_cast<bool>(char_fn), "subsp::math::finance::pricing::option_price_fft: char_fn must be valid");
  validate_heston_fft_(options, "option_price_fft");
  if (maturity <= 0.0) return intrinsic_(type, spot, strike);
  const auto [call_prices, strikes] = option_fft_grid_values_(spot, rate, dividend_yield, maturity, char_fn, options);
  const double call = interp_linear_on_log_strike_(strikes, call_prices, strike);
  if (type == OptionType::Call) return call;
  const double disc_q = std::exp(-dividend_yield * maturity);
  const double disc_r = std::exp(-rate * maturity);
  return call + strike * disc_r - spot * disc_q;
}

HestonFftResult bates_price_fft_grid(OptionType type,
                                     double spot,
                                     double rate,
                                     double dividend_yield,
                                     double maturity,
                                     const BatesParams& params,
                                     const HestonFftOptions& options,
                                     Config cfg) {
  validate_bates_(params, "bates_price_fft_grid");
  const CharacteristicFn cf = [&](std::complex<double> phi) {
    return std::exp(-rate * maturity) *
           bates_cf_(phi, spot, rate, dividend_yield, maturity, params, options.mode, options.vol_risk_premium);
  };
  return option_price_fft_grid(type, spot, rate, dividend_yield, maturity, cf, options, cfg);
}

double bates_price_fft(OptionType type,
                       double spot,
                       double strike,
                       double rate,
                       double dividend_yield,
                       double maturity,
                       const BatesParams& params,
                       const HestonFftOptions& options,
                       Config cfg) {
  validate_bates_(params, "bates_price_fft");
  const CharacteristicFn cf = [&](std::complex<double> phi) {
    return std::exp(-rate * maturity) *
           bates_cf_(phi, spot, rate, dividend_yield, maturity, params, options.mode, options.vol_risk_premium);
  };
  return option_price_fft(type, spot, strike, rate, dividend_yield, maturity, cf, options, cfg);
}

double heston_price_cos(OptionType type,
                        double spot,
                        double strike,
                        double rate,
                        double dividend_yield,
                        double maturity,
                        const HestonParams& params,
                        const CosOptions& options,
                        Config cfg) {
  validate_option_(type, "heston_price_cos");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "heston_price_cos");
  validate_heston_(params, "heston_price_cos");
  validate_cos_options_(options, "heston_price_cos");
  const CharacteristicFn cf = [&](std::complex<double> phi) {
    return std::exp(-rate * maturity) *
           heston_cf_(phi, spot, rate, dividend_yield, maturity, params, HestonCfMode::LittleTrap, 2, 0.0);
  };
  return option_price_cos(type, spot, strike, rate, dividend_yield, maturity, cf, options, cfg);
}

double bates_price_cos(OptionType type,
                       double spot,
                       double strike,
                       double rate,
                       double dividend_yield,
                       double maturity,
                       const BatesParams& params,
                       const CosOptions& options,
                       Config cfg) {
  validate_option_(type, "bates_price_cos");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "bates_price_cos");
  validate_bates_(params, "bates_price_cos");
  validate_cos_options_(options, "bates_price_cos");
  const CharacteristicFn cf = [&](std::complex<double> phi) {
    return std::exp(-rate * maturity) *
           bates_cf_(phi, spot, rate, dividend_yield, maturity, params, HestonCfMode::LittleTrap, 0.0);
  };
  return option_price_cos(type, spot, strike, rate, dividend_yield, maturity, cf, options, cfg);
}

double heston_price_mc(OptionType type,
                       double spot,
                       double strike,
                       double rate,
                       double dividend_yield,
                       double maturity,
                       const HestonParams& params,
                       const MonteCarloOptions& options,
                       Config cfg) {
  validate_option_(type, "heston_price_mc");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "heston_price_mc");
  validate_heston_(params, "heston_price_mc");
  validate_mc_(options, "heston_price_mc");

  if (maturity <= 0.0) {
    return intrinsic_(type, spot, strike);
  }

  const auto out_cfg = f64_cfg_(cfg);
  HestonParams q_params = params;
  q_params.mu = rate - dividend_yield;
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme;
  sim_opt.method = options.rand_method;
  sim_opt.randomized_quasi = options.randomized_quasi;
  sim_opt.sobol_scrambling = options.sobol_scrambling;
  sim_opt.antithetic = options.antithetic;
  sim_opt.normal_shift = options.importance_sampling ? options.importance_shift : 0.0;
  sim_opt.seed = options.seed;
  sim_opt.offset = options.offset;

  const double dt = maturity / static_cast<double>(options.steps);
  const auto sim = volatility::heston_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::full({options.paths}, 1.0, out_cfg);
  const Series target = disc_payoff.mul(lr);

  if (!options.control_variate) {
    return target.mean().item<double>();
  }
  const double sigma_cv = positive_or_(options.control_vol, std::sqrt(std::max(params.theta, 1e-12)));
  const Series control = bs_control_discounted_payoff_(
      type, sim.innovation_spot, spot, strike, rate, dividend_yield, maturity, sigma_cv)
                             .mul(lr);
  const double control_mean = black_scholes_price(type, spot, strike, rate, dividend_yield, maturity, sigma_cv);
  return mc_cv_estimate_(target, control, control_mean);
}

double sv_price_mc(OptionType type,
                   double spot,
                   double strike,
                   double rate,
                   double dividend_yield,
                   double maturity,
                   const SvParams& params,
                   const MonteCarloOptions& options,
                   Config cfg) {
  validate_option_(type, "sv_price_mc");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "sv_price_mc");
  validate_sv_(params, "sv_price_mc");
  validate_mc_(options, "sv_price_mc");

  if (maturity <= 0.0) {
    return intrinsic_(type, spot, strike);
  }

  const auto out_cfg = f64_cfg_(cfg);
  SvParams q_params = params;
  q_params.mu = rate - dividend_yield;
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme;
  sim_opt.method = options.rand_method;
  sim_opt.randomized_quasi = options.randomized_quasi;
  sim_opt.sobol_scrambling = options.sobol_scrambling;
  sim_opt.antithetic = options.antithetic;
  sim_opt.normal_shift = options.importance_sampling ? options.importance_shift : 0.0;
  sim_opt.seed = options.seed;
  sim_opt.offset = options.offset;

  const double dt = maturity / static_cast<double>(options.steps);
  const auto sim = volatility::sv_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::full({options.paths}, 1.0, out_cfg);
  const Series target = disc_payoff.mul(lr);

  if (!options.control_variate) {
    return target.mean().item<double>();
  }
  const double sigma_cv = positive_or_(options.control_vol, std::exp(0.5 * params.logvar_mean));
  const Series control = bs_control_discounted_payoff_(
      type, sim.innovation_spot, spot, strike, rate, dividend_yield, maturity, sigma_cv)
                             .mul(lr);
  const double control_mean = black_scholes_price(type, spot, strike, rate, dividend_yield, maturity, sigma_cv);
  return mc_cv_estimate_(target, control, control_mean);
}

double heston_nandi_price(OptionType type,
                          double spot,
                          double strike,
                          double rate,
                          double dividend_yield,
                          double maturity,
                          const HestonNandiParams& params,
                          const HestonNandiIntegrationOptions& options) {
  validate_option_(type, "heston_nandi_price");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "heston_nandi_price");
  validate_heston_nandi_(params, "heston_nandi_price");
  validate_heston_nandi_integration_(options, "heston_nandi_price");

  if (maturity <= 0.0) {
    return intrinsic_(type, spot, strike);
  }

  const std::int64_t steps = std::max<std::int64_t>(
      1, static_cast<std::int64_t>(std::llround(maturity * static_cast<double>(options.periods_per_year))));
  const double dt = maturity / static_cast<double>(steps);
  const double r_step = (rate - dividend_yield) * dt;

  const double h0 = std::isfinite(params.initial_variance) && params.initial_variance > 0.0
                        ? std::max(params.initial_variance, params.variance_floor)
                        : heston_nandi_unc_var_(params);

  HestonNandiParams q_params = params;
  q_params.lambda = -0.5;
  q_params.gamma = params.gamma + params.lambda + 0.5;

  const double p1 = heston_nandi_prob_j_(1, strike, spot, h0, r_step, steps, q_params, options);
  const double p2 = heston_nandi_prob_j_(2, strike, spot, h0, r_step, steps, q_params, options);

  const double disc_q = std::exp(-dividend_yield * maturity);
  const double disc_r = std::exp(-rate * maturity);
  const double call = spot * disc_q * p1 - strike * disc_r * p2;
  if (type == OptionType::Call) {
    return std::max(0.0, call);
  }
  const double parity = strike * disc_r - spot * disc_q;
  return std::max(0.0, call + parity);
}

double heston_nandi_price_mc(OptionType type,
                             double spot,
                             double strike,
                             double rate,
                             double dividend_yield,
                             double maturity,
                             const HestonNandiParams& params,
                             const MonteCarloOptions& options,
                             Config cfg) {
  validate_option_(type, "heston_nandi_price_mc");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "heston_nandi_price_mc");
  validate_heston_nandi_(params, "heston_nandi_price_mc");
  validate_mc_(options, "heston_nandi_price_mc");

  if (maturity <= 0.0) {
    return intrinsic_(type, spot, strike);
  }

  const auto out_cfg = f64_cfg_(cfg);
  HestonNandiParams q_params = params;
  q_params.lambda = -0.5;
  q_params.gamma = params.gamma + params.lambda + 0.5;
  q_params.r = rate - dividend_yield;

  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme;
  sim_opt.method = options.rand_method;
  sim_opt.randomized_quasi = options.randomized_quasi;
  sim_opt.sobol_scrambling = options.sobol_scrambling;
  sim_opt.antithetic = options.antithetic;
  sim_opt.normal_shift = options.importance_sampling ? options.importance_shift : 0.0;
  sim_opt.seed = options.seed;
  sim_opt.offset = options.offset;

  const double dt = maturity / static_cast<double>(options.steps);
  const auto sim =
      volatility::heston_nandi_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::full({options.paths}, 1.0, out_cfg);
  const Series target = disc_payoff.mul(lr);

  if (!options.control_variate) {
    return target.mean().item<double>();
  }
  const double sigma_cv = positive_or_(options.control_vol, std::sqrt(heston_nandi_unc_var_(params)));
  const Series control = bs_control_discounted_payoff_(
      type, sim.innovation_spot, spot, strike, rate, dividend_yield, maturity, sigma_cv)
                             .mul(lr);
  const double control_mean = black_scholes_price(type, spot, strike, rate, dividend_yield, maturity, sigma_cv);
  return mc_cv_estimate_(target, control, control_mean);
}

double bates_price_mc(OptionType type,
                      double spot,
                      double strike,
                      double rate,
                      double dividend_yield,
                      double maturity,
                      const BatesParams& params,
                      const MonteCarloOptions& options,
                      Config cfg) {
  validate_option_(type, "bates_price_mc");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "bates_price_mc");
  validate_bates_(params, "bates_price_mc");
  validate_mc_(options, "bates_price_mc");

  if (maturity <= 0.0) {
    return intrinsic_(type, spot, strike);
  }

  const auto out_cfg = f64_cfg_(cfg);
  BatesParams q_params = params;
  q_params.mu = rate - dividend_yield;
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme;
  sim_opt.method = options.rand_method;
  sim_opt.randomized_quasi = options.randomized_quasi;
  sim_opt.sobol_scrambling = options.sobol_scrambling;
  sim_opt.antithetic = options.antithetic;
  sim_opt.normal_shift = options.importance_sampling ? options.importance_shift : 0.0;
  sim_opt.seed = options.seed;
  sim_opt.offset = options.offset;

  const double dt = maturity / static_cast<double>(options.steps);
  const auto sim = volatility::bates_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::full({options.paths}, 1.0, out_cfg);
  const Series target = disc_payoff.mul(lr);

  if (!options.control_variate) {
    return target.mean().item<double>();
  }
  const double sigma_cv = positive_or_(options.control_vol, std::sqrt(std::max(params.theta, 1e-12)));
  const Series control = bs_control_discounted_payoff_(
      type, sim.innovation_spot, spot, strike, rate, dividend_yield, maturity, sigma_cv)
                             .mul(lr);
  const double control_mean = black_scholes_price(type, spot, strike, rate, dividend_yield, maturity, sigma_cv);
  return mc_cv_estimate_(target, control, control_mean);
}

double sabr_price_mc(OptionType type,
                     double forward,
                     double strike,
                     double rate,
                     double maturity,
                     const SabrParams& params,
                     const MonteCarloOptions& options,
                     Config cfg) {
  validate_option_(type, "sabr_price_mc");
  validate_common_(forward, strike, rate, 0.0, maturity, "sabr_price_mc");
  validate_sabr_(params, "sabr_price_mc");
  validate_mc_(options, "sabr_price_mc");

  if (maturity <= 0.0) {
    return intrinsic_(type, forward, strike);
  }

  const auto out_cfg = f64_cfg_(cfg);
  SabrParams q_params = params;
  q_params.mu = 0.0;
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme;
  sim_opt.method = options.rand_method;
  sim_opt.randomized_quasi = options.randomized_quasi;
  sim_opt.sobol_scrambling = options.sobol_scrambling;
  sim_opt.antithetic = options.antithetic;
  sim_opt.normal_shift = options.importance_sampling ? options.importance_shift : 0.0;
  sim_opt.seed = options.seed;
  sim_opt.offset = options.offset;

  const double dt = maturity / static_cast<double>(options.steps);
  const auto sim = volatility::sabr_paths(options.steps, options.paths, forward, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::full({options.paths}, 1.0, out_cfg);
  const Series target = disc_payoff.mul(lr);

  if (!options.control_variate) {
    return target.mean().item<double>();
  }
  const double sigma_sabr = sabr_black_vol(forward, strike, maturity, params);
  const double sigma_cv = positive_or_(options.control_vol, positive_or_(sigma_sabr, std::max(params.alpha0, 1e-8)));
  const Series control = bs_control_discounted_payoff_(
      type, sim.innovation_spot, forward, strike, rate, rate, maturity, sigma_cv)
                             .mul(lr);
  const double control_mean = black_scholes_price(type, forward, strike, rate, rate, maturity, sigma_cv);
  return mc_cv_estimate_(target, control, control_mean);
}

double slv_price_mc(OptionType type,
                    double spot,
                    double strike,
                    double rate,
                    double dividend_yield,
                    double maturity,
                    const SlvParams& params,
                    const MonteCarloOptions& options,
                    Config cfg) {
  validate_option_(type, "slv_price_mc");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "slv_price_mc");
  validate_slv_(params, "slv_price_mc");
  validate_mc_(options, "slv_price_mc");

  if (maturity <= 0.0) {
    return intrinsic_(type, spot, strike);
  }

  const auto out_cfg = f64_cfg_(cfg);
  SlvParams q_params = params;
  q_params.mu = rate - dividend_yield;
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme;
  sim_opt.method = options.rand_method;
  sim_opt.randomized_quasi = options.randomized_quasi;
  sim_opt.sobol_scrambling = options.sobol_scrambling;
  sim_opt.antithetic = options.antithetic;
  sim_opt.normal_shift = options.importance_sampling ? options.importance_shift : 0.0;
  sim_opt.seed = options.seed;
  sim_opt.offset = options.offset;

  const double dt = maturity / static_cast<double>(options.steps);
  const auto sim = volatility::slv_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::full({options.paths}, 1.0, out_cfg);
  const Series target = disc_payoff.mul(lr);

  if (!options.control_variate) {
    return target.mean().item<double>();
  }
  const double sigma_cv = positive_or_(options.control_vol, std::sqrt(std::max(params.theta, 1e-12)) *
                                                            std::max(1e-8, params.local_vol_level));
  const Series control = bs_control_discounted_payoff_(
      type, sim.innovation_spot, spot, strike, rate, dividend_yield, maturity, sigma_cv)
                             .mul(lr);
  const double control_mean = black_scholes_price(type, spot, strike, rate, dividend_yield, maturity, sigma_cv);
  return mc_cv_estimate_(target, control, control_mean);
}

double rough_heston_price_mc(OptionType type,
                             double spot,
                             double strike,
                             double rate,
                             double dividend_yield,
                             double maturity,
                             const RoughHestonParams& params,
                             const MonteCarloOptions& options,
                             Config cfg) {
  validate_option_(type, "rough_heston_price_mc");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "rough_heston_price_mc");
  validate_rough_heston_(params, "rough_heston_price_mc");
  validate_mc_(options, "rough_heston_price_mc");

  if (maturity <= 0.0) return intrinsic_(type, spot, strike);

  const auto out_cfg = f64_cfg_(cfg);
  RoughHestonParams q_params = params;
  q_params.mu = rate - dividend_yield;
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme;
  sim_opt.method = options.rand_method;
  sim_opt.randomized_quasi = options.randomized_quasi;
  sim_opt.sobol_scrambling = options.sobol_scrambling;
  sim_opt.antithetic = options.antithetic;
  sim_opt.normal_shift = options.importance_sampling ? options.importance_shift : 0.0;
  sim_opt.seed = options.seed;
  sim_opt.offset = options.offset;

  const double dt = maturity / static_cast<double>(options.steps);
  const auto sim =
      volatility::rough_heston_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::full({options.paths}, 1.0, out_cfg);
  const Series target = disc_payoff.mul(lr);
  if (!options.control_variate) return target.mean().item<double>();

  const double sigma_cv = positive_or_(options.control_vol, std::sqrt(std::max(params.theta, 1e-12)));
  const Series control = bs_control_discounted_payoff_(
      type, sim.innovation_spot, spot, strike, rate, dividend_yield, maturity, sigma_cv)
                             .mul(lr);
  const double control_mean = black_scholes_price(type, spot, strike, rate, dividend_yield, maturity, sigma_cv);
  return mc_cv_estimate_(target, control, control_mean);
}

double rough_bergomi_price_mc(OptionType type,
                              double spot,
                              double strike,
                              double rate,
                              double dividend_yield,
                              double maturity,
                              const RoughBergomiParams& params,
                              const MonteCarloOptions& options,
                              Config cfg) {
  validate_option_(type, "rough_bergomi_price_mc");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "rough_bergomi_price_mc");
  validate_rough_bergomi_(params, "rough_bergomi_price_mc");
  validate_mc_(options, "rough_bergomi_price_mc");

  if (maturity <= 0.0) return intrinsic_(type, spot, strike);

  const auto out_cfg = f64_cfg_(cfg);
  RoughBergomiParams q_params = params;
  q_params.mu = rate - dividend_yield;
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme;
  sim_opt.method = options.rand_method;
  sim_opt.randomized_quasi = options.randomized_quasi;
  sim_opt.sobol_scrambling = options.sobol_scrambling;
  sim_opt.antithetic = options.antithetic;
  sim_opt.normal_shift = options.importance_sampling ? options.importance_shift : 0.0;
  sim_opt.seed = options.seed;
  sim_opt.offset = options.offset;

  const double dt = maturity / static_cast<double>(options.steps);
  const auto sim =
      volatility::rough_bergomi_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::full({options.paths}, 1.0, out_cfg);
  const Series target = disc_payoff.mul(lr);
  if (!options.control_variate) return target.mean().item<double>();

  const double sigma_cv = positive_or_(options.control_vol, std::sqrt(std::max(params.xi, 1e-12)));
  const Series control = bs_control_discounted_payoff_(
      type, sim.innovation_spot, spot, strike, rate, dividend_yield, maturity, sigma_cv)
                             .mul(lr);
  const double control_mean = black_scholes_price(type, spot, strike, rate, dividend_yield, maturity, sigma_cv);
  return mc_cv_estimate_(target, control, control_mean);
}

} // namespace pluss::subsp::math::finance::pricing
