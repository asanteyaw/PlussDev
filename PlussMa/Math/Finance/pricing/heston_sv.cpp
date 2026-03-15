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
                                int numeraire) {
  using C = std::complex<double>;
  const C i(0.0, 1.0);

  const double x = std::log(spot);
  const double u = (numeraire == 1) ? 0.5 : -0.5;
  const double b = (numeraire == 1) ? (p.kappa - p.rho * p.sigma) : p.kappa;
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
    const C cf = heston_cf_(phi, spot, rate, dividend_yield, maturity, params, options.mode, numeraire);
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
  q_params.mu = rate;
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
  const double sigma_cv = positive_or_(options.control_vol, std::max(params.alpha0, 1e-8));
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

} // namespace pluss::subsp::math::finance::pricing
