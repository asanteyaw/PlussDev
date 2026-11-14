#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/integ.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::pricing {
using namespace detail;
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
              std::string("subsp::math::finance::pricing::") + op + ": option type must be Call or Put (OptionType::Auto is only valid for an instrument that supplies is_call)");
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
  PLUSS_CHECK(opt.sobol_scrambling >= 0 && opt.sobol_scrambling <= 3,
              std::string("subsp::math::finance::pricing::") + op + ": sobol_scrambling must be in [0,3]");
}

void validate_heston_integration_(const HestonIntegrationOptions& opt, const char* op) {
  PLUSS_CHECK(opt.integration_steps >= 2,
              std::string("subsp::math::finance::pricing::") + op + ": integration_steps must be >= 2");
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
  PLUSS_CHECK(std::isfinite(opt.cumulant_step) && opt.cumulant_step >= 1e-6,
              std::string("subsp::math::finance::pricing::") + op + ": cumulant_step must be finite and >= 1e-6");
}

void validate_heston_nandi_integration_(const HestonNandiIntegrationOptions& opt, const char* op) {
  PLUSS_CHECK(opt.integration_steps >= 2,
              std::string("subsp::math::finance::pricing::") + op + ": integration_steps must be >= 2");
  PLUSS_CHECK(std::isfinite(opt.integration_u_max) && opt.integration_u_max > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": integration_u_max must be finite and > 0");
  PLUSS_CHECK(opt.periods_per_year > 0,
              std::string("subsp::math::finance::pricing::") + op + ": periods_per_year must be > 0");
}

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_f64_series_(const Series& x, Config cfg) {
  return x.to(cfg.platform, pluss::ScalarType::Float64).contiguous();
}

Series scalar_series_(double value, Config cfg) {
  return pluss::series::full({}, value, f64_cfg_(cfg));
}

struct ComplexSeries_ {
  Series re = pluss::series::undefined();
  Series im = pluss::series::undefined();
};

ComplexSeries_ c_make_(const Series& re, const Series& im) {
  return ComplexSeries_{re, im};
}

ComplexSeries_ c_scalar_(double re, Config cfg) {
  return c_make_(scalar_series_(re, cfg), scalar_series_(0.0, cfg));
}

ComplexSeries_ c_add_(const ComplexSeries_& a, const ComplexSeries_& b) {
  return c_make_(a.re.add(b.re), a.im.add(b.im));
}

ComplexSeries_ c_sub_(const ComplexSeries_& a, const ComplexSeries_& b) {
  return c_make_(a.re.sub(b.re), a.im.sub(b.im));
}

ComplexSeries_ c_mul_(const ComplexSeries_& a, const ComplexSeries_& b) {
  return c_make_(a.re.mul(b.re).sub(a.im.mul(b.im)), a.re.mul(b.im).add(a.im.mul(b.re)));
}

ComplexSeries_ c_mul_(const ComplexSeries_& a, double b) {
  return c_make_(a.re.mul(b), a.im.mul(b));
}

ComplexSeries_ c_mul_real_(const ComplexSeries_& a, const Series& b) {
  return c_make_(a.re.mul(b), a.im.mul(b));
}

ComplexSeries_ c_div_(const ComplexSeries_& a, const ComplexSeries_& b) {
  const Series denom = b.re.square().add(b.im.square()).clamp_min(kEps);
  return c_make_(a.re.mul(b.re).add(a.im.mul(b.im)).div(denom),
                 a.im.mul(b.re).sub(a.re.mul(b.im)).div(denom));
}

ComplexSeries_ c_div_(const ComplexSeries_& a, double b) {
  return c_make_(a.re.div(b), a.im.div(b));
}

ComplexSeries_ c_exp_(const ComplexSeries_& z) {
  const Series er = z.re.exp();
  return c_make_(er.mul(z.im.cos()), er.mul(z.im.sin()));
}

ComplexSeries_ c_log_(const ComplexSeries_& z) {
  return c_make_(z.re.hypot(z.im).clamp_min(kEps).log(), z.im.atan2(z.re));
}

ComplexSeries_ c_sqrt_(const ComplexSeries_& z) {
  const Series radius = z.re.hypot(z.im).sqrt();
  const Series half_angle = z.im.atan2(z.re).mul(0.5);
  return c_make_(radius.mul(half_angle.cos()), radius.mul(half_angle.sin()));
}

// The log-spot characteristic function of Heston or Bates with every parameter a Series: [1, n] entries (one per option) or a single entry,
// broadcast against the frequency axis [N, 1]. With the jump members defined the Merton jump factor of Bates is applied. Besides phi it returns the
// two logarithmic derivatives the closed-form Greeks need: d log phi / d v0 (the affine coefficient D) and d log phi / d tau.
struct SvCfPieces_ {
  ComplexSeries_ phi, dlog_v0, dlog_tau;
};

SvCfPieces_ sv_logspot_cf_pieces_(const Series& u,
                                  const Series& spot,
                                  const Series& maturity,
                                  const Series& rate,
                                  const Series& dividend_yield,
                                  const SvParamsSeries& p,
                                  Config cfg) {
  const auto row = [&](const Series& x) { return as_f64_series_(x, cfg).flatten(0, -1).reshape({1, x.numel()}); };
  const Series spot2 = row(spot), tau = row(maturity);
  const Series uu = as_f64_series_(u, cfg);
  const Series kappa = row(p.kappa), theta = row(p.theta), v0 = row(p.v0);
  const Series sigma = row(p.sigma).clamp_min(1e-12);
  const Series sigma2 = sigma.square();
  const Series rho = row(p.rho).clamp_min(-0.999999).clamp_max(0.999999);

  const ComplexSeries_ alpha = c_make_(uu.square().mul(-0.5), uu.mul(-0.5));
  const ComplexSeries_ beta = c_make_(kappa, uu.mul(rho.mul(sigma).neg()));
  const ComplexSeries_ disc = c_sub_(c_mul_(beta, beta), c_mul_real_(alpha, sigma2.mul(2.0)));
  const ComplexSeries_ d = c_sqrt_(disc);
  const ComplexSeries_ beta_minus_d = c_sub_(beta, d);
  const ComplexSeries_ g = c_div_(beta_minus_d, c_add_(beta, d));
  const ComplexSeries_ exp_md = c_exp_(c_mul_real_(c_mul_(d, -1.0), tau));
  const ComplexSeries_ one = c_scalar_(1.0, cfg);
  const ComplexSeries_ psi = c_div_(c_sub_(one, c_mul_(g, exp_md)), c_sub_(one, g));
  const ComplexSeries_ a_term = c_mul_real_(
      c_sub_(c_mul_real_(beta_minus_d, tau), c_mul_(c_log_(psi), 2.0)),
      kappa.mul(theta).div(sigma2));
  const ComplexSeries_ one_minus_g_e = c_sub_(one, c_mul_(g, exp_md));
  const ComplexSeries_ b_term = c_mul_(
      c_make_(beta_minus_d.re.div(sigma2), beta_minus_d.im.div(sigma2)),
      c_div_(c_sub_(one, exp_md), one_minus_g_e));

  Series carry = row(rate).sub(row(dividend_yield));
  const bool jumps = p.jump_intensity.defined();
  Series lambda, m, delta;
  if (jumps) {
    // the compensator keeps the discounted price a martingale: the drift loses lambda * k, with k = E[e^J] - 1
    lambda = row(p.jump_intensity); m = row(p.jump_mean); delta = row(p.jump_vol);
    carry = carry.sub(lambda.mul(m.add(delta.square().mul(0.5)).exp().sub(1.0)));
  }
  ComplexSeries_ drift = c_make_(scalar_series_(0.0, cfg), uu.mul(spot2.log()).add(uu.mul(carry).mul(tau)));
  ComplexSeries_ log_phi = c_add_(c_add_(a_term, c_mul_real_(b_term, v0)), drift);
  // d log phi / d tau = (kappa theta / sigma^2)[(beta - d) - 2 d g e^{-d tau}/(1 - g e^{-d tau})] + v0 D_tau + i u carry (+ jumps)
  const ComplexSeries_ d_g_e = c_mul_(c_mul_(d, g), exp_md);
  const ComplexSeries_ a_tau = c_mul_real_(c_sub_(beta_minus_d, c_mul_(c_div_(d_g_e, one_minus_g_e), 2.0)), kappa.mul(theta).div(sigma2));
  const ComplexSeries_ d_tau = c_mul_(c_make_(beta_minus_d.re.div(sigma2), beta_minus_d.im.div(sigma2)),
                                      c_div_(c_mul_(c_mul_(d, exp_md), c_sub_(one, g)), c_mul_(one_minus_g_e, one_minus_g_e)));
  ComplexSeries_ dlog_tau = c_add_(c_add_(a_tau, c_mul_real_(d_tau, v0)), c_make_(scalar_series_(0.0, cfg), uu.mul(carry)));
  if (jumps) {
    const Series mag = uu.square().mul(delta.square()).mul(-0.5).exp();
    const Series ang = uu.mul(m);
    const ComplexSeries_ jump = c_make_(mag.mul(ang.cos()).sub(1.0).mul(lambda).mul(tau), mag.mul(ang.sin()).mul(lambda).mul(tau));
    log_phi = c_add_(log_phi, jump);
    dlog_tau = c_add_(dlog_tau, c_make_(mag.mul(ang.cos()).sub(1.0).mul(lambda), mag.mul(ang.sin()).mul(lambda)));
  }
  return SvCfPieces_{c_exp_(log_phi), b_term, dlog_tau};
}

ComplexSeries_ sv_logspot_cf_series_(const Series& u,
                                     const Series& spot,
                                     const Series& maturity,
                                     const Series& rate,
                                     const Series& dividend_yield,
                                     const SvParamsSeries& p,
                                     Config cfg) {
  return sv_logspot_cf_pieces_(u, spot, maturity, rate, dividend_yield, p, cfg).phi;
}

ComplexSeries_ heston_logspot_cf_series_(const Series& u,
                                          const Series& spot,
                                          const Series& maturity,
                                          double rate,
                                          double dividend_yield,
                                          const HestonParams& p,
                                          Config cfg) {
  const auto c = [&](double v) { return pluss::series::full({1}, v, f64_cfg_(cfg)); };
  const SvParamsSeries params{.kappa = c(p.kappa), .theta = c(p.theta), .sigma = c(std::max(p.sigma, p.variance_floor)), .rho = c(p.rho), .v0 = c(p.v0)};
  return sv_logspot_cf_series_(u, spot, maturity, c(rate), c(dividend_yield), params, cfg);
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

// Composite Simpson's rule (scalar characteristic-function inversion
// integrand, fixed n = options.integration_steps nodes). Delegates to
// pluss::subsp::math::integ::quadra_scalar (PlussMa/include/pluss/subsp/
// math/integ.h) rather than reimplementing the rule locally -- this was a
// THIRD independent copy of the identical formula already consolidated
// out of PlussMa/Stats/distributions's distr.cpp/distr_objects_continuous.cpp.
// The Carr-Madan FFT-grid Simpson/Trapezoidal weighting scheme elsewhere in
// this file (HestonFftQuadrature, heston_fft_grid_values_) is NOT the same
// computational pattern -- those weights feed an FFT producing an entire
// strike grid in one transform, not a direct numerical integral -- and is
// deliberately left untouched.
double simpson_integral_(const std::function<double(double)>& f, double a, double b, std::int64_t n) {
  pluss::subsp::math::integ::QuadraScalarOptions options;
  options.nodes = n;
  return pluss::subsp::math::integ::quadra_scalar(f, a, b, options).value;
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
    return pluss::series::full({paths}, 1.0, Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(innovation_spot.platform()));
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

std::int64_t next_pow2_(std::int64_t n) {
  PLUSS_CHECK(n > 0, "subsp::math::finance::pricing: FFT length must be positive");
  std::int64_t out = 1;
  while (out < n) {
    PLUSS_CHECK(out <= std::numeric_limits<std::int64_t>::max() / 2,
                "subsp::math::finance::pricing: FFT length is too large");
    out <<= 1;
  }
  return out;
}

std::vector<std::complex<double>> fourier_(
    const std::vector<std::complex<double>>& values, bool inverse) {
  const auto n = static_cast<std::int64_t>(values.size());
  const Series input = pluss::make_series(values, {n});
  const Series output = inverse ? input.ifft() : input.fft();
  return output.items<std::complex<double>>();
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

  const Series fu = pluss::make_series(u, {m}).fft();
  const Series fv = pluss::make_series(v, {m}).fft();
  const auto convolution = fu.mul(fv).ifft().items<std::complex<double>>();

  std::vector<std::complex<double>> out(static_cast<std::size_t>(n), {0.0, 0.0});
  for (std::int64_t k = 0; k < n; ++k) {
    const auto wk = convolution[static_cast<std::size_t>(k + n - 1)];
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

  // Cubic (four-point Lagrange) interpolation in the log strike: the grid is uniform in log K, and the price is smooth, so the error
  // falls as the fourth power of the grid spacing instead of the second. Next to the ends of the grid the linear interpolant is used.
  if (lo >= 1 && hi + 1 < strikes.size()) {
    const double x[4] = {std::log(strikes[lo - 1]), std::log(strikes[lo]), std::log(strikes[hi]), std::log(strikes[hi + 1])};
    const double y[4] = {prices[lo - 1], prices[lo], prices[hi], prices[hi + 1]};
    double value = 0.0;
    for (int i = 0; i < 4; ++i) {
      double basis = 1.0;
      for (int j = 0; j < 4; ++j) {
        if (j != i) {
          basis *= (logk - x[j]) / (x[i] - x[j]);
        }
      }
      value += y[i] * basis;
    }
    return value;
  }
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
                                                                             const HestonFftOptions& options,
                                                                             const CharacteristicFn* multiplier = nullptr,
                                                                             double forced_alpha = -1.0,
                                                                             double* alpha_out = nullptr) {
  // `char_fn` is the characteristic function of ln S_T, E[exp(i u ln S_T)], as for the COS pricer; the Carr-Madan transform is of the
  // discounted payoff, so the discount factor is applied here.
  const double discount = std::exp(-rate * maturity);
  const std::int64_t n = options.num_fft;
  const double eta = options.characteristic_fcn_step;
  const double lambda = std::isnan(options.log_strike_step)
                            ? (2.0 * kPi / static_cast<double>(n) / eta)
                            : options.log_strike_step;
  const double log_s = std::log(spot);
  const std::complex<double> i(0.0, 1.0);
  const double forward = std::exp(log_s + (rate - dividend_yield) * maturity);

  // The damped transform exists only if the moment E[S_T^(alpha + 1)] is finite, and it is accurate only if the damped integrand has
  // decayed by the end of the frequency grid. A model can lose the moment (the Heston model does at a large volatility of variance and
  // a long maturity), and near that point the integrand decays too slowly for any grid. Past it the characteristic function continues
  // analytically through the singularity to numbers that are not a moment: not real, not positive, or below the Jensen bound
  // F^(alpha + 1), or finite but so close to the explosion that the damped transform has lost its accuracy (the moment more than twice F^(alpha + 1)). So halve the damping exponent until the moment is a genuine one and the integrand has decayed, rather than return a
  // price that is silently wrong.
  auto moment_exists = [&](double a) {
    const std::complex<double> m = char_fn(std::complex<double>(0.0, -(a + 1.0)));
    return std::isfinite(m.real()) && std::isfinite(m.imag()) && m.real() > 0.0 && std::abs(m.imag()) <= 1e-6 * m.real() &&
           m.real() >= std::pow(forward, a + 1.0) * (1.0 - 1e-6) &&
           m.real() <= 2.0 * std::pow(forward, a + 1.0);
  };
  auto integrand = [&](double a, std::vector<std::complex<double>>& psi) {
    for (std::int64_t j = 0; j < n; ++j) {
      const double u = eta * static_cast<double>(j);
      const std::complex<double> z = std::complex<double>(u, 0.0) - (a + 1.0) * i;
      const auto f2 = discount * char_fn(z) * (multiplier ? (*multiplier)(z) : std::complex<double>(1.0, 0.0));
      const auto den = (a * a + a - u * u) + i * u * (2.0 * a + 1.0);
      psi[static_cast<std::size_t>(j)] = f2 / den;
    }
  };
  auto decayed = [&](const std::vector<std::complex<double>>& psi) {
    double peak = 0.0, tail = 0.0;
    for (std::int64_t j = 1; j < n; ++j) {  // the first node divides by a polynomial that vanishes only for alpha = 0 or -1
      const double magnitude = std::abs(psi[static_cast<std::size_t>(j)]);
      peak = std::max(peak, magnitude);
      if (j >= n - n / 10) {
        tail = std::max(tail, magnitude);
      }
    }
    return std::isfinite(tail) && tail <= 1e-6 * peak;
  };
  std::vector<std::complex<double>> psi(static_cast<std::size_t>(n));
  double alpha = options.damping_factor;
  double first_valid = -1.0;
  bool accepted = false;
  if (forced_alpha > 0.0) {   // a Greek's transform reuses the damping exponent chosen for the price
    alpha = forced_alpha;
    integrand(alpha, psi);
    accepted = true;
  }
  for (int halving = 0; halving < 12 && !accepted; ++halving) {
    const double candidate = options.damping_factor * std::pow(0.5, halving);
    if (!moment_exists(candidate)) {
      continue;
    }
    if (first_valid < 0.0) {
      first_valid = candidate;
    }
    integrand(candidate, psi);
    if (decayed(psi)) {
      alpha = candidate;
      accepted = true;
    }
  }
  if (!accepted) {
    // No exponent gives a decayed integrand (for instance, the frequency range was set too short): use the largest one with a genuine moment.
    PLUSS_CHECK(first_valid > 0.0,
                "subsp::math::finance::pricing::option_price_fft: the characteristic function has no finite moment E[S_T^(1 + damping_factor)] "
                "for any damping factor tried; use the COS pricer");
    alpha = first_valid;
    integrand(alpha, psi);
  }
  if (alpha_out) *alpha_out = alpha;
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

  std::vector<std::complex<double>> x(static_cast<std::size_t>(n), {0.0, 0.0});
  for (std::int64_t j = 0; j < n; ++j) {
    const double u = eta * static_cast<double>(j);
    const std::complex<double> phi(u, 0.0);
    const auto phase = std::exp(i * phi * (static_cast<double>(n) * lambda * 0.5 - log_s));
    x[static_cast<std::size_t>(j)] = phase * psi[static_cast<std::size_t>(j)] * weights[static_cast<std::size_t>(j)];
  }

  std::vector<std::complex<double>> transform;
  if (std::abs(frft_param - fft_param) < 1e-14) {
    transform = fourier_(x, false);
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

  // The original Heston form of the characteristic function has a branch discontinuity that overflows at long maturities, and the
  // transform then comes back as NaN: say so instead of returning NaN prices.
  for (const double price : prices) {
    PLUSS_CHECK(std::isfinite(price),
                "subsp::math::finance::pricing::option_price_fft: the transform produced a non-finite price (the characteristic function "
                "overflowed, as the original Heston form does at long maturities or large volatilities of variance); use HestonCfMode::LittleTrap "
                "or the COS pricer");
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
    return heston_cf_(phi,
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
    return bates_cf_(phi, spot, rate, dividend_yield, maturity, params, options.mode, options.vol_risk_premium);
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

double detail::heston_price(OptionType type,
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

HestonFftResult detail::heston_price_fft_grid(OptionType type,
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

namespace {

struct CfPieces_ {
  std::complex<double> phi, dlog_v0, dlog_tau;
};

// Characteristic function of ln S_T for Heston (LittleTrap form) or Bates with its two logarithmic derivatives, d log phi / d v0 and d log phi / d tau, at a
// complex frequency.
CfPieces_ sv_cf_pieces_host_(std::complex<double> phi, double spot, double rate, double dividend_yield, double tau, const BatesParams& p, bool bates, double vrp) {
  using C = std::complex<double>;
  const C i(0.0, 1.0);
  double carry_rate = rate;
  if (bates) carry_rate = rate - p.jump_intensity * (std::exp(p.jump_mean + 0.5 * p.jump_vol * p.jump_vol) - 1.0);
  const double sigma2 = p.sigma * p.sigma, a = p.kappa * p.theta, b = p.kappa + vrp;
  const C bt = b - p.rho * p.sigma * i * phi;
  const C d = std::sqrt(bt * bt - sigma2 * (-i * phi - phi * phi));
  const C g = (bt - d) / (bt + d);
  const C e = std::exp(-d * tau);
  const C one(1.0, 0.0);
  const C c_term = (carry_rate - dividend_yield) * i * phi * tau + (a / sigma2) * ((bt - d) * tau - 2.0 * std::log((one - g * e) / (one - g)));
  const C d_term = ((bt - d) / sigma2) * ((one - e) / (one - g * e));
  C log_phi = c_term + d_term * p.v0 + i * phi * std::log(spot);
  C dlog_tau = (carry_rate - dividend_yield) * i * phi + (a / sigma2) * ((bt - d) - 2.0 * d * g * e / (one - g * e)) +
               p.v0 * (((bt - d) / sigma2) * d * e * (one - g) / ((one - g * e) * (one - g * e)));
  if (bates) {
    const C jump = std::exp(i * phi * p.jump_mean - 0.5 * p.jump_vol * p.jump_vol * phi * phi) - 1.0;
    log_phi += p.jump_intensity * tau * jump;
    dlog_tau += p.jump_intensity * jump;
  }
  return CfPieces_{std::exp(log_phi), d_term, dlog_tau};
}

// Value, first and second derivative with respect to x of the cubic through the four grid points around x (uniform grid in x).
void lagrange4_(const std::vector<double>& xs, const std::vector<double>& ys, std::size_t lo, double x, double& value, double& d1, double& d2) {
  const std::size_t first = lo >= 1 ? lo - 1 : 0;
  const std::size_t base = std::min(first, xs.size() - 4);
  value = d1 = d2 = 0.0;
  for (std::size_t a = 0; a < 4; ++a) {
    // basis polynomial L_a(x) = prod_{c != a} (x - x_c) / (x_a - x_c)
    double denom = 1.0;
    for (std::size_t c = 0; c < 4; ++c) if (c != a) denom *= xs[base + a] - xs[base + c];
    double l = 1.0, dl = 0.0, ddl = 0.0;
    for (std::size_t c = 0; c < 4; ++c) {
      if (c == a) continue;
      const double f = x - xs[base + c];
      ddl = ddl * f + 2.0 * dl;
      dl = dl * f + l;
      l *= f;
    }
    value += ys[base + a] * l / denom;
    d1 += ys[base + a] * dl / denom;
    d2 += ys[base + a] * ddl / denom;
  }
}

}  // namespace

McStripGreeks detail::sv_fft_strip(const BatesParams& params,
                                   bool bates,
                                   double spot,
                                   double rate,
                                   double dividend_yield,
                                   double maturity,
                                   const std::vector<double>& strikes,
                                   const std::vector<bool>& is_call,
                                   const HestonFftOptions& options,
                                   bool greeks,
                                   Config cfg) {
  (void)cfg;
  const char* op = bates ? "bates_price_fft" : "heston_price_fft";
  validate_heston_fft_(options, op);
  const std::size_t n = strikes.size();
  McStripGreeks out;
  out.price.assign(n, 0.0); out.delta.assign(n, 0.0); out.gamma.assign(n, 0.0); out.vega.assign(n, 0.0); out.theta.assign(n, 0.0); out.rho.assign(n, 0.0);
  if (maturity <= 0.0) {
    for (std::size_t i = 0; i < n; ++i) {
      out.price[i] = intrinsic_(is_call[i] ? OptionType::Call : OptionType::Put, spot, strikes[i]);
      out.delta[i] = is_call[i] ? (spot > strikes[i] ? 1.0 : 0.0) : (spot < strikes[i] ? -1.0 : 0.0);
    }
    return out;
  }
  const double vrp = options.vol_risk_premium;
  const CharacteristicFn cf = [&](std::complex<double> z) { return sv_cf_pieces_host_(z, spot, rate, dividend_yield, maturity, params, bates, vrp).phi; };
  double alpha = 0.0;
  const auto [call_grid, strike_grid] = option_fft_grid_values_(spot, rate, dividend_yield, maturity, cf, options, nullptr, -1.0, &alpha);
  const double disc_r = std::exp(-rate * maturity), disc_q = std::exp(-dividend_yield * maturity);
  std::vector<double> log_strikes(strike_grid.size());
  for (std::size_t j = 0; j < strike_grid.size(); ++j) log_strikes[j] = std::log(strike_grid[j]);
  const auto locate = [&](double strike) {
    PLUSS_CHECK(strike >= strike_grid.front() && strike <= strike_grid.back(),
                std::string("subsp::math::finance::pricing::") + op + ": strike must lie in FFT strike grid range");
    return static_cast<std::size_t>(std::distance(strike_grid.begin(), std::lower_bound(strike_grid.begin(), strike_grid.end(), strike)));
  };
  std::vector<std::size_t> position(n);
  for (std::size_t i = 0; i < n; ++i) {
    position[i] = std::max<std::size_t>(locate(strikes[i]), 1) - 1;
    const double call = interp_linear_on_log_strike_(strike_grid, call_grid, strikes[i]);
    out.price[i] = is_call[i] ? call : call + strikes[i] * disc_r - spot * disc_q;
  }
  if (!greeks) return out;

  // delta and gamma from the homogeneity V(S, K) = S g(K / S): delta = (V - V_k) / S, gamma = (V_kk - V_k) / S^2, with k = ln K read off the same grid
  std::vector<double> call_value(n), vk(n), vkk(n);
  for (std::size_t i = 0; i < n; ++i) lagrange4_(log_strikes, call_grid, position[i], std::log(strikes[i]), call_value[i], vk[i], vkk[i]);
  // vega, rho and theta: the transform of phi times the Greek's factor, with the damping exponent of the price
  const auto transform_of = [&](const std::function<std::complex<double>(std::complex<double>)>& factor) {
    const CharacteristicFn m = factor;
    return option_fft_grid_values_(spot, rate, dividend_yield, maturity, cf, options, &m, alpha, nullptr).first;
  };
  const auto vega_grid = transform_of([&](std::complex<double> z) { return sv_cf_pieces_host_(z, spot, rate, dividend_yield, maturity, params, bates, vrp).dlog_v0; });
  const auto rho_grid = transform_of([&](std::complex<double> z) { return std::complex<double>(0.0, 1.0) * z * maturity; });
  const auto tau_grid = transform_of([&](std::complex<double> z) { return sv_cf_pieces_host_(z, spot, rate, dividend_yield, maturity, params, bates, vrp).dlog_tau; });
  for (std::size_t i = 0; i < n; ++i) {
    const double k = strikes[i];
    const double call_delta = (call_value[i] - vk[i]) / spot, gamma = (vkk[i] - vk[i]) / (spot * spot);
    const double vega = interp_linear_on_log_strike_(strike_grid, vega_grid, k);
    // the call grids of the multiplied transforms are already discounted by e^{-rT}; the derivative of the discount adds -T V (rho) and -r V (tau)
    double rho = interp_linear_on_log_strike_(strike_grid, rho_grid, k) - maturity * call_value[i];
    double dtau = interp_linear_on_log_strike_(strike_grid, tau_grid, k) - rate * call_value[i];
    double delta = call_delta;
    if (!is_call[i]) {   // P = C - S e^{-qT} + K e^{-rT}
      delta -= disc_q;
      rho -= maturity * k * disc_r;
      dtau += dividend_yield * spot * disc_q - rate * k * disc_r;
    }
    out.delta[i] = delta; out.gamma[i] = gamma; out.vega[i] = vega; out.rho[i] = rho; out.theta[i] = -dtau;
  }
  return out;
}


double detail::heston_price_fft(OptionType type,
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

HestonFftResult detail::bates_price_fft_grid(OptionType type,
                                     double spot,
                                     double rate,
                                     double dividend_yield,
                                     double maturity,
                                     const BatesParams& params,
                                     const HestonFftOptions& options,
                                     Config cfg) {
  validate_bates_(params, "bates_price_fft_grid");
  const CharacteristicFn cf = [&](std::complex<double> phi) {
    return bates_cf_(phi, spot, rate, dividend_yield, maturity, params, options.mode, options.vol_risk_premium);
  };
  return option_price_fft_grid(type, spot, rate, dividend_yield, maturity, cf, options, cfg);
}

double detail::bates_price_fft(OptionType type,
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
    return bates_cf_(phi, spot, rate, dividend_yield, maturity, params, options.mode, options.vol_risk_premium);
  };
  return option_price_fft(type, spot, strike, rate, dividend_yield, maturity, cf, options, cfg);
}

double detail::heston_price_cos(OptionType type,
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
  const Config out_cfg = f64_cfg_(cfg);
  const Series s = pluss::series::full({1}, spot, out_cfg);
  const Series k = pluss::series::full({1}, strike, out_cfg);
  const Series t = pluss::series::full({1}, maturity, out_cfg);
  return heston_price_cos(type, s, k, rate, dividend_yield, t, params, options, out_cfg).reshape({1}).select(0, 0).item<double>();
}

Series detail::heston_price_cos(OptionType type,
                        const Series& spot,
                        const Series& strike,
                        double rate,
                        double dividend_yield,
                        const Series& maturity,
                        const HestonParams& params,
                        const CosOptions& options,
                        Config cfg) {
  validate_option_(type, "heston_price_cos");
  validate_heston_(params, "heston_price_cos");
  validate_cos_options_(options, "heston_price_cos");
  const Config out_cfg = f64_cfg_(cfg);
  // Small batches are cheaper through the per-option loop: the vectorized path pays a fixed cost for its dozens of
  // whole-grid operations, which only amortizes once options x terms is large (measured crossover near 10^4).
  constexpr std::int64_t kScalarWork = 12000;
  if (spot.numel() == strike.numel() && spot.numel() == maturity.numel() &&
      spot.numel() * std::max<std::int64_t>(options.num_terms, 8) < kScalarWork) {
    const auto s = spot.reshape({spot.numel()}).host<double>();
    const auto k = strike.reshape({strike.numel()}).host<double>();
    const auto t = maturity.reshape({maturity.numel()}).host<double>();
    std::vector<double> out(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
      const CharacteristicFn cf = [&, i](std::complex<double> phi) {
        return heston_cf_(phi, s[i], rate, dividend_yield, t[i], params, HestonCfMode::LittleTrap, 2, 0.0);
      };
      out[i] = option_price_cos(type, s[i], k[i], rate, dividend_yield, t[i], cf, options, out_cfg);
    }
    return pluss::make_series(out, {static_cast<std::int64_t>(out.size())}, out_cfg);
  }
  // A strike chain (one spot, one maturity) needs the characteristic function once per frequency, not once per
  // option: hand it single-element spot/maturity and let the strike axis broadcast.
  const auto constant = [&](const Series& x) {
    return x.numel() > 1 && x.amax().item<double>() == x.amin().item<double>();
  };
  const Series spot_c = constant(spot) ? spot.flatten(0, -1).slice(0, 0, 1) : spot;
  const Series maturity_c = constant(maturity) ? maturity.flatten(0, -1).slice(0, 0, 1) : maturity;
  const CharacteristicSeriesFn cf = [&](const Series& u) {
    const ComplexSeries_ z = heston_logspot_cf_series_(u, spot_c, maturity_c, rate, dividend_yield, params, out_cfg);
    return ComplexSeries{z.re, z.im};
  };
  return option_price_cos(type, spot_c, strike, rate, dividend_yield, maturity_c, cf, options, out_cfg);
}

CosGreeksV detail::sv_cos_greeks_v(const Series& is_call,
                                   const Series& spot,
                                   const Series& strike,
                                   const Series& rate,
                                   const Series& dividend_yield,
                                   const Series& maturity,
                                   const SvParamsSeries& params,
                                   const CosOptions& options,
                                   Config cfg) {
  validate_cos_options_(options, "cos_greeks");
  const Config out_cfg = f64_cfg_(cfg);
  const CosCfPiecesFn pieces = [&](const Series& u) {
    const SvCfPieces_ z = sv_logspot_cf_pieces_(u, spot, maturity, rate, dividend_yield, params, out_cfg);
    return CosCfPieces{ComplexSeries{z.phi.re, z.phi.im}, ComplexSeries{z.dlog_v0.re, z.dlog_v0.im}, ComplexSeries{z.dlog_tau.re, z.dlog_tau.im}};
  };
  return cos_greeks_v(is_call, spot, strike, rate, dividend_yield, maturity, pieces, options, out_cfg);
}

Series detail::heston_price_cos_v(OptionType type,
                                  const Series& spot,
                                  const Series& strike,
                                  const Series& rate,
                                  const Series& dividend_yield,
                                  const Series& maturity,
                                  const SvParamsSeries& params,
                                  const CosOptions& options,
                                  Config cfg) {
  validate_option_(type, "heston_price_cos");
  validate_cos_options_(options, "heston_price_cos");
  const Config out_cfg = f64_cfg_(cfg);
  const CharacteristicSeriesFn cf = [&](const Series& u) {
    const ComplexSeries_ z = sv_logspot_cf_series_(u, spot, maturity, rate, dividend_yield, params, out_cfg);
    return ComplexSeries{z.re, z.im};
  };
  return option_price_cos_v(type, spot, strike, rate, dividend_yield, maturity, cf, options, out_cfg);
}

Series detail::bates_price_cos_v(OptionType type,
                                 const Series& spot,
                                 const Series& strike,
                                 const Series& rate,
                                 const Series& dividend_yield,
                                 const Series& maturity,
                                 const SvParamsSeries& params,
                                 const CosOptions& options,
                                 Config cfg) {
  validate_option_(type, "bates_price_cos");
  validate_cos_options_(options, "bates_price_cos");
  PLUSS_CHECK(params.jump_intensity.defined() && params.jump_mean.defined() && params.jump_vol.defined(),
              "subsp::math::finance::pricing::bates_price_cos: the jump parameters must be defined");
  const Config out_cfg = f64_cfg_(cfg);
  const CharacteristicSeriesFn cf = [&](const Series& u) {
    const ComplexSeries_ z = sv_logspot_cf_series_(u, spot, maturity, rate, dividend_yield, params, out_cfg);
    return ComplexSeries{z.re, z.im};
  };
  return option_price_cos_v(type, spot, strike, rate, dividend_yield, maturity, cf, options, out_cfg);
}

double detail::bates_price_cos(OptionType type,
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
    return bates_cf_(phi, spot, rate, dividend_yield, maturity, params, HestonCfMode::LittleTrap, 0.0);
  };
  return option_price_cos(type, spot, strike, rate, dividend_yield, maturity, cf, options, cfg);
}

double detail::heston_price_mc(OptionType type,
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
  const auto sim = volatility::detail::heston_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::series::full({options.paths}, 1.0, out_cfg);
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

double detail::sv_price_mc(OptionType type,
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
  const auto sim = volatility::detail::sv_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::series::full({options.paths}, 1.0, out_cfg);
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

double detail::heston_nandi_price(OptionType type,
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

double detail::heston_nandi_price_mc(OptionType type,
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

  // The GARCH recursion is defined per period, so the number of periods is part of the model, not of the discretisation: as in the
  // closed form, `steps` counts periods per year and the path has round(maturity * steps) of them.
  const std::int64_t periods =
      std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(maturity * static_cast<double>(options.steps))));
  const double dt = maturity / static_cast<double>(periods);
  const auto sim = volatility::detail::heston_nandi_paths(periods, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, periods).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::series::full({options.paths}, 1.0, out_cfg);
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

using McSimulateFn_ = std::function<volatility::StochVolPaths(double level, const pluss::codem::Series* tracked)>;

// The shared-path estimator behind every Monte Carlo strip: `simulate(level)` returns the paths with the volatility level (initial variance, forward variance...)
// set to `level` (or read from the tracked scalar, which makes the paths differentiable in it for the pathwise vega). `cv_sigma` is the default volatility of the Black-Scholes control variate.
McStripGreeks mc_strip_core_(const McSimulateFn_& simulate,
                             double base_level,
                             double cv_sigma,
                             const char* op,
                             double spot,
                             double rate,
                             double dividend_yield,
                             double maturity,
                             const std::vector<double>& strikes,
                             const std::vector<bool>& is_call,
                             const MonteCarloOptions& options,
                             bool greeks,
                             Config cfg) {
  validate_mc_(options, op);
  validate_common_(spot, strikes.empty() ? 1.0 : strikes.front(), rate, dividend_yield, maturity, op);
  const std::size_t n = strikes.size();
  McStripGreeks out;
  out.price.assign(n, 0.0); out.delta.assign(n, 0.0); out.gamma.assign(n, 0.0); out.vega.assign(n, 0.0); out.theta.assign(n, 0.0); out.rho.assign(n, 0.0);
  const auto type_of = [&](std::size_t i) { return is_call[i] ? OptionType::Call : OptionType::Put; };
  if (maturity <= 0.0) {
    for (std::size_t i = 0; i < n; ++i) {
      out.price[i] = intrinsic_(type_of(i), spot, strikes[i]);
      out.delta[i] = is_call[i] ? (spot > strikes[i] ? 1.0 : 0.0) : (spot < strikes[i] ? -1.0 : 0.0);
    }
    return out;
  }
  const auto out_cfg = f64_cfg_(cfg);
  const double dt = maturity / static_cast<double>(options.steps);
  const auto sim = simulate(base_level, nullptr);
  const double sigma_cv = positive_or_(options.control_vol, cv_sigma);

  // lr[c]: importance likelihood ratio using the first c steps; payoff of option i at date c with the spot scaled and the rate shifted
  std::vector<Series> lr_cache(static_cast<std::size_t>(options.steps) + 1);
  const auto lr_at = [&](std::int64_t c) -> const Series& {
    Series& cached = lr_cache[static_cast<std::size_t>(c)];
    if (!cached.defined()) {
      cached = options.importance_sampling ? importance_lr_(sim.innovation_spot.slice(0, 0, c), options.importance_shift) : pluss::series::full({options.paths}, 1.0, out_cfg);
    }
    return cached;
  };
  const auto terminal_at = [&](std::int64_t c, double scale, double rate_shift) {
    return sim.spot.select(0, c).reshape({options.paths}).mul(scale * std::exp(rate_shift * static_cast<double>(c) * dt));
  };
  const auto plain = [&](std::size_t i, std::int64_t c, double scale, double rate_shift) {
    const double t = static_cast<double>(c) * dt;
    return discounted_payoff_(type_of(i), terminal_at(c, scale, rate_shift), strikes[i], rate + rate_shift, t).mul(lr_at(c)).mean().item<double>();
  };
  for (std::size_t i = 0; i < n; ++i) {
    if (!options.control_variate) {
      out.price[i] = plain(i, options.steps, 1.0, 0.0);
    } else {
      const Series target = discounted_payoff_(type_of(i), terminal_at(options.steps, 1.0, 0.0), strikes[i], rate, maturity).mul(lr_at(options.steps));
      const Series control = bs_control_discounted_payoff_(type_of(i), sim.innovation_spot, spot, strikes[i], rate, dividend_yield, maturity, sigma_cv).mul(lr_at(options.steps));
      out.price[i] = mc_cv_estimate_(target, control, black_scholes_price(type_of(i), spot, strikes[i], rate, dividend_yield, maturity, sigma_cv));
    }
  }
  if (!greeks) return out;

  // delta and rho pathwise on the shared paths; gamma by a second difference of the payoff in the spot scale; theta by a backward difference over dates
  const double eps = 0.01;   // relative spot step of the second difference: the payoff is piecewise linear, a wider step lowers the variance
  const Series terminal = terminal_at(options.steps, 1.0, 0.0);
  const Series lr = lr_at(options.steps);
  const double disc = std::exp(-rate * maturity);
  for (std::size_t i = 0; i < n; ++i) {
    const bool call = is_call[i];
    const Series itm = call ? terminal.gt(strikes[i]) : terminal.lt(strikes[i]);
    const Series ind = itm.where(terminal, terminal.mul(0.0));                       // S_T on the exercise region
    const double sign = call ? 1.0 : -1.0;
    out.delta[i] = sign * disc * ind.mul(lr).mean().item<double>() / spot;
    const Series payoff = discounted_payoff_(type_of(i), terminal, strikes[i], rate, maturity);
    out.rho[i] = maturity * (sign * disc * ind.mul(lr).mean().item<double>() - payoff.mul(lr).mean().item<double>());
    const double up = plain(i, options.steps, 1.0 + eps, 0.0), down = plain(i, options.steps, 1.0 - eps, 0.0);
    out.gamma[i] = (up - 2.0 * plain(i, options.steps, 1.0, 0.0) + down) / (eps * eps * spot * spot);
    if (options.steps >= 3) {
      const double p0 = plain(i, options.steps, 1.0, 0.0), p1 = plain(i, options.steps - 1, 1.0, 0.0), p2 = plain(i, options.steps - 2, 1.0, 0.0);
      out.theta[i] = -(3.0 * p0 - 4.0 * p1 + p2) / (2.0 * dt);
    } else {
      out.theta[i] = -(plain(i, options.steps, 1.0, 0.0) - plain(i, options.steps - 1, 1.0, 0.0)) / dt;
    }
  }
  // vega in the initial variance (forward-variance level): pathwise, by differentiating the simulated paths themselves with the level a tracked scalar
  {
    Series level = pluss::make_series(base_level).to(out_cfg.platform, out_cfg.dtype);
    level.tracks_slope_(true);
    const auto sim_t = simulate(base_level, &level);
    const Series terminal_t = sim_t.spot.select(0, options.steps).reshape({options.paths});
    for (std::size_t i = 0; i < n; ++i) {
      discounted_payoff_(type_of(i), terminal_t, strikes[i], rate, maturity).mul(lr_at(options.steps)).mean().reverse(true);   // the graph is kept for the next strike
      out.vega[i] = level.sensitivity().item<double>();
      level.clear();
    }
  }
  return out;
}
McStripGreeks detail::sv_mc_strip(const BatesParams& params, bool bates, double spot, double rate, double dividend_yield, double maturity, const std::vector<double>& strikes,
                                  const std::vector<bool>& is_call, const MonteCarloOptions& options, bool greeks, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme; sim_opt.method = options.rand_method; sim_opt.randomized_quasi = options.randomized_quasi; sim_opt.sobol_scrambling = options.sobol_scrambling;
  sim_opt.antithetic = options.antithetic; sim_opt.normal_shift = options.importance_sampling ? options.importance_shift : 0.0; sim_opt.seed = options.seed; sim_opt.offset = options.offset;
  const double dt = maturity / static_cast<double>(options.steps);
  const McSimulateFn_ simulate = [&](double v0, const Series* tracked) {
    volatility::SimOptions so = sim_opt;
    so.tracked_level = tracked;
    BatesParams q = params;
    q.mu = rate - dividend_yield;
    q.v0 = v0;
    if (!bates) q.jump_intensity = 0.0;
    return bates ? volatility::detail::bates_paths(options.steps, options.paths, spot, dt, q, so, out_cfg, options.seed)
                 : volatility::detail::heston_paths(options.steps, options.paths, spot, dt, HestonParams{.kappa = q.kappa, .theta = q.theta, .sigma = q.sigma, .rho = q.rho, .v0 = q.v0, .mu = q.mu, .variance_floor = q.variance_floor},
                                                    so, out_cfg, options.seed);
  };
  return mc_strip_core_(simulate, params.v0, std::sqrt(std::max(params.theta, 1e-12)), bates ? "bates_price_mc" : "heston_price_mc", spot, rate, dividend_yield, maturity, strikes, is_call, options, greeks, cfg);
}

McStripGreeks detail::rough_heston_mc_strip(const RoughHestonParams& params, double spot, double rate, double dividend_yield, double maturity, const std::vector<double>& strikes,
                                            const std::vector<bool>& is_call, const MonteCarloOptions& options, bool greeks, Config cfg) {
  validate_rough_heston_(params, "rough_heston_price_mc");
  const auto out_cfg = f64_cfg_(cfg);
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme; sim_opt.method = options.rand_method; sim_opt.randomized_quasi = options.randomized_quasi; sim_opt.sobol_scrambling = options.sobol_scrambling;
  sim_opt.antithetic = options.antithetic; sim_opt.normal_shift = options.importance_sampling ? options.importance_shift : 0.0; sim_opt.seed = options.seed; sim_opt.offset = options.offset;
  const double dt = maturity / static_cast<double>(std::max<std::int64_t>(options.steps, 1));
  const McSimulateFn_ simulate = [&](double v0, const Series* tracked) {
    volatility::SimOptions so = sim_opt;
    so.tracked_level = tracked;
    RoughHestonParams q = params;
    q.mu = rate - dividend_yield;
    q.v0 = v0;
    return volatility::detail::rough_heston_paths(options.steps, options.paths, spot, dt, q, so, out_cfg, options.seed);
  };
  return mc_strip_core_(simulate, params.v0, std::sqrt(std::max(params.theta, 1e-12)), "rough_heston_price_mc", spot, rate, dividend_yield, maturity, strikes, is_call, options, greeks, cfg);
}

McStripGreeks detail::rough_bergomi_mc_strip(const RoughBergomiParams& params, double spot, double rate, double dividend_yield, double maturity, const std::vector<double>& strikes,
                                             const std::vector<bool>& is_call, const MonteCarloOptions& options, bool greeks, Config cfg) {
  validate_rough_bergomi_(params, "rough_bergomi_price_mc");
  const auto out_cfg = f64_cfg_(cfg);
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme; sim_opt.method = options.rand_method; sim_opt.randomized_quasi = options.randomized_quasi; sim_opt.sobol_scrambling = options.sobol_scrambling;
  sim_opt.antithetic = options.antithetic; sim_opt.normal_shift = options.importance_sampling ? options.importance_shift : 0.0; sim_opt.seed = options.seed; sim_opt.offset = options.offset;
  const double dt = maturity / static_cast<double>(std::max<std::int64_t>(options.steps, 1));
  const McSimulateFn_ simulate = [&](double xi, const Series* tracked) {
    volatility::SimOptions so = sim_opt;
    so.tracked_level = tracked;
    RoughBergomiParams q = params;
    q.mu = rate - dividend_yield;
    q.xi = xi;
    return volatility::detail::rough_bergomi_paths(options.steps, options.paths, spot, dt, q, so, out_cfg, options.seed);
  };
  return mc_strip_core_(simulate, params.xi, std::sqrt(std::max(params.xi, 1e-12)), "rough_bergomi_price_mc", spot, rate, dividend_yield, maturity, strikes, is_call, options, greeks, cfg);
}

std::vector<McStripGreeks> detail::sv_mc_assets(const std::vector<McAsset>& assets, double maturity, const MonteCarloOptions& options, bool greeks, Config cfg) {
  validate_mc_(options, "sv_mc_assets");
  PLUSS_CHECK(!assets.empty(), "subsp::math::finance::pricing::sv_mc_assets: no assets");
  PLUSS_CHECK(!options.control_variate && !options.importance_sampling,
              "subsp::math::finance::pricing::sv_mc_assets: control variates and importance sampling need one asset at a time");
  const std::int64_t A = static_cast<std::int64_t>(assets.size());
  const auto out_cfg = f64_cfg_(cfg);
  std::vector<McStripGreeks> results(assets.size());
  for (std::size_t a = 0; a < assets.size(); ++a) {
    const std::size_t n = assets[a].strikes.size();
    results[a].price.assign(n, 0.0); results[a].delta.assign(n, 0.0); results[a].gamma.assign(n, 0.0);
    results[a].vega.assign(n, 0.0); results[a].theta.assign(n, 0.0); results[a].rho.assign(n, 0.0);
  }
  if (maturity <= 0.0) {
    for (std::size_t a = 0; a < assets.size(); ++a) {
      for (std::size_t i = 0; i < assets[a].strikes.size(); ++i) {
        const double k = assets[a].strikes[i], s = assets[a].spot;
        results[a].price[i] = intrinsic_(assets[a].is_call[i] ? OptionType::Call : OptionType::Put, s, k);
        results[a].delta[i] = assets[a].is_call[i] ? (s > k ? 1.0 : 0.0) : (s < k ? -1.0 : 0.0);
      }
    }
    return results;
  }
  const double dt = maturity / static_cast<double>(options.steps);
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme; sim_opt.method = options.rand_method; sim_opt.randomized_quasi = options.randomized_quasi;
  sim_opt.sobol_scrambling = options.sobol_scrambling; sim_opt.antithetic = options.antithetic; sim_opt.seed = options.seed; sim_opt.offset = options.offset;
  const auto column = [&](const std::vector<double>& v) { return pluss::make_series(v, {A}, out_cfg); };
  const auto simulate = [&](const Series& v0_column) {
    volatility::detail::MultiSvParams p;
    std::vector<double> spot, growth, kappa, theta, sigma, rho, lambda, jm, jv;
    for (const auto& as : assets) {
      spot.push_back(as.spot); growth.push_back(as.rate - as.dividend_yield); kappa.push_back(as.params.kappa); theta.push_back(as.params.theta);
      sigma.push_back(as.params.sigma); rho.push_back(as.params.rho);
      lambda.push_back(as.bates ? as.params.jump_intensity : 0.0); jm.push_back(as.params.jump_mean); jv.push_back(as.params.jump_vol);
    }
    p.spot0 = column(spot); p.growth = column(growth); p.kappa = column(kappa); p.theta = column(theta); p.sigma = column(sigma); p.rho = column(rho); p.v0 = v0_column;
    p.jump_intensity = column(lambda); p.jump_mean = column(jm); p.jump_vol = column(jv);
    return volatility::detail::multi_sv_paths(options.steps, options.paths, dt, p, sim_opt, out_cfg, options.seed).spot;   // [paths, steps + 1, A]
  };
  std::vector<double> v0;
  for (const auto& as : assets) v0.push_back(as.params.v0);
  const Series base = simulate(column(v0));

  const auto terminal_of = [&](const Series& paths, std::int64_t a, std::int64_t c, double scale, double rate_shift) {
    return paths.select(2, a).select(1, c).mul(scale * std::exp(rate_shift * static_cast<double>(c) * dt));
  };
  const auto plain = [&](const Series& paths, std::size_t a, std::size_t i, std::int64_t c, double scale, double rate_shift) {
    return discounted_payoff_(assets[a].is_call[i] ? OptionType::Call : OptionType::Put, terminal_of(paths, static_cast<std::int64_t>(a), c, scale, rate_shift), assets[a].strikes[i],
                              assets[a].rate + rate_shift, static_cast<double>(c) * dt).mean().item<double>();
  };
  constexpr double eps = 0.01;
  for (std::size_t a = 0; a < assets.size(); ++a) {
    const double rate = assets[a].rate, spot = assets[a].spot, disc = std::exp(-rate * maturity);
    const Series terminal = terminal_of(base, static_cast<std::int64_t>(a), options.steps, 1.0, 0.0);
    for (std::size_t i = 0; i < assets[a].strikes.size(); ++i) {
      const bool call = assets[a].is_call[i];
      const double k = assets[a].strikes[i], sign = call ? 1.0 : -1.0;
      const double p0 = plain(base, a, i, options.steps, 1.0, 0.0);
      results[a].price[i] = p0;
      if (!greeks) continue;
      const Series itm = call ? terminal.gt(k) : terminal.lt(k);
      const double indicator_mean = itm.where(terminal, terminal.mul(0.0)).mean().item<double>();
      results[a].delta[i] = sign * disc * indicator_mean / spot;
      results[a].rho[i] = maturity * (sign * disc * indicator_mean - p0);
      results[a].gamma[i] = (plain(base, a, i, options.steps, 1.0 + eps, 0.0) - 2.0 * p0 + plain(base, a, i, options.steps, 1.0 - eps, 0.0)) / (eps * eps * spot * spot);
      if (options.steps >= 3) {
        results[a].theta[i] = -(3.0 * p0 - 4.0 * plain(base, a, i, options.steps - 1, 1.0, 0.0) + plain(base, a, i, options.steps - 2, 1.0, 0.0)) / (2.0 * dt);
      } else {
        results[a].theta[i] = -(p0 - plain(base, a, i, options.steps - 1, 1.0, 0.0)) / dt;
      }
    }
  }
  if (greeks) {
    // pathwise vega: the initial variances are a tracked column, and the assets are independent, so the sensitivity of an asset's estimate is read at its entry
    Series level = column(v0);
    level.tracks_slope_(true);
    const Series sim_t = simulate(level);
    for (std::size_t a = 0; a < assets.size(); ++a) {
      for (std::size_t i = 0; i < assets[a].strikes.size(); ++i) {
        discounted_payoff_(assets[a].is_call[i] ? OptionType::Call : OptionType::Put, terminal_of(sim_t, static_cast<std::int64_t>(a), options.steps, 1.0, 0.0), assets[a].strikes[i],
                           assets[a].rate, maturity).mean().reverse(true);
        results[a].vega[i] = level.sensitivity().items<double>()[a];
        level.clear();
      }
    }
  }
  return results;
}

double detail::bates_price_mc(OptionType type,
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
  const auto sim = volatility::detail::bates_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::series::full({options.paths}, 1.0, out_cfg);
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

double detail::sabr_price_mc(OptionType type,
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
  const auto sim = volatility::detail::sabr_paths(options.steps, options.paths, forward, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::series::full({options.paths}, 1.0, out_cfg);
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

ScalarGreeks detail::sabr_mc_greeks(OptionType type, double spot, double strike, double rate, double dividend_yield, double maturity, const SabrParams& params,
                                    const MonteCarloOptions& options, Config cfg) {
  validate_option_(type, "sabr_mc_greeks");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "sabr_mc_greeks");
  validate_sabr_(params, "sabr_mc_greeks");
  validate_mc_(options, "sabr_mc_greeks");
  ScalarGreeks g;
  const double forward = spot * std::exp((rate - dividend_yield) * maturity);
  if (maturity <= 0.0) {
    g.price = intrinsic_(type, spot, strike);
    g.delta = type == OptionType::Call ? (spot > strike ? 1.0 : 0.0) : (spot < strike ? -1.0 : 0.0);
    return g;
  }
  const auto out_cfg = f64_cfg_(cfg);
  SabrParams q_params = params;
  q_params.mu = 0.0;
  volatility::SimOptions sim_opt;
  sim_opt.scheme = options.scheme; sim_opt.method = options.rand_method; sim_opt.randomized_quasi = options.randomized_quasi; sim_opt.sobol_scrambling = options.sobol_scrambling;
  sim_opt.antithetic = options.antithetic; sim_opt.normal_shift = options.importance_sampling ? options.importance_shift : 0.0; sim_opt.seed = options.seed; sim_opt.offset = options.offset;
  const double dt = maturity / static_cast<double>(options.steps);
  // the initial forward (one copy per path) and the initial volatility are tracked, so the paths are differentiable in both
  Series f0 = pluss::series::full({options.paths}, forward, out_cfg);
  f0.tracks_slope_(true);
  Series a0 = pluss::make_series(params.alpha0).to(out_cfg.platform, out_cfg.dtype);
  a0.tracks_slope_(true);
  sim_opt.tracked_spot = &f0;
  sim_opt.tracked_level = &a0;
  const auto sim = volatility::detail::sabr_paths(options.steps, options.paths, forward, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series path = sim.spot;   // [steps + 1, paths]
  const Series terminal = path.select(0, options.steps).reshape({options.paths});
  const double disc = std::exp(-rate * maturity);
  const bool call = type == OptionType::Call;
  const double sign = call ? 1.0 : -1.0;
  const auto payoff = [&](const Series& f) { return (call ? f.sub(strike) : f.mul(-1.0).add(strike)).clamp_min(0.0); };
  // importance sampling reweights every payoff by the likelihood ratio of the innovations, which does not depend on the parameters
  const auto lr_at = [&](std::int64_t c) {
    return options.importance_sampling ? importance_lr_(sim.innovation_spot.slice(0, 0, c), options.importance_shift) : pluss::series::full({options.paths}, 1.0, out_cfg);
  };
  const Series lr = lr_at(options.steps);
  const Series estimate = payoff(terminal).mul(lr).mean().mul(disc);
  g.price = estimate.item<double>();
  // delta (in the forward) and vega: one reverse pass on the estimate
  estimate.reverse(true);
  const double dp_df = f0.sensitivity().sum().item<double>();
  g.vega = a0.sensitivity().item<double>();
  f0.clear();
  a0.clear();
  // the pathwise Jacobian of the terminal forward in the initial forward, for the second difference of the payoff
  terminal.sum().reverse(true);
  const Series jac = f0.sensitivity().reshape({options.paths});
  f0.clear();
  const double h = 0.01 * forward;
  const auto shifted = [&](double sgn) { return payoff(terminal.add(jac.mul(sgn * h))).mul(lr).mean().item<double>() * disc; };
  const double d2 = (shifted(1.0) - 2.0 * g.price + shifted(-1.0)) / (h * h);
  const double growth = std::exp((rate - dividend_yield) * maturity);
  g.delta = dp_df * growth;
  g.gamma = d2 * growth * growth;
  // rho: the discount factor and the forward's growth with the rate; theta: the same terms plus the forward process's own time derivative, a second-order
  // backward difference over the two earlier dates of the same paths
  g.rho = -maturity * g.price + maturity * forward * dp_df;
  const auto at_step = [&](std::int64_t c) { return payoff(path.select(0, c).reshape({options.paths})).mul(lr_at(c)).mean().item<double>(); };
  const double p0 = at_step(options.steps);
  const double g_time = options.steps >= 3 ? (3.0 * p0 - 4.0 * at_step(options.steps - 1) + at_step(options.steps - 2)) / (2.0 * dt) : (p0 - at_step(options.steps - 1)) / dt;
  const double dp_dt = -rate * g.price + (rate - dividend_yield) * forward * dp_df + disc * g_time;   // total derivative in the maturity
  g.theta = -dp_dt;
  (void)sign;
  if (options.control_variate) {   // the control variate sharpens the price; the Greeks stay those of the unbiased plain estimator
    const double sigma_sabr = sabr_black_vol(forward, strike, maturity, params);
    const double sigma_cv = positive_or_(options.control_vol, positive_or_(sigma_sabr, std::max(params.alpha0, 1e-8)));
    const Series control = bs_control_discounted_payoff_(type, sim.innovation_spot, forward, strike, rate, rate, maturity, sigma_cv).mul(lr);
    g.price = mc_cv_estimate_(payoff(terminal).mul(disc).mul(lr), control, black_scholes_price(type, forward, strike, rate, rate, maturity, sigma_cv));
  }
  return g;
}

double detail::slv_price_mc(OptionType type,
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
  const auto sim = volatility::detail::slv_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::series::full({options.paths}, 1.0, out_cfg);
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

double detail::rough_heston_price_mc(OptionType type,
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
      volatility::detail::rough_heston_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::series::full({options.paths}, 1.0, out_cfg);
  const Series target = disc_payoff.mul(lr);
  if (!options.control_variate) return target.mean().item<double>();

  const double sigma_cv = positive_or_(options.control_vol, std::sqrt(std::max(params.theta, 1e-12)));
  const Series control = bs_control_discounted_payoff_(
      type, sim.innovation_spot, spot, strike, rate, dividend_yield, maturity, sigma_cv)
                             .mul(lr);
  const double control_mean = black_scholes_price(type, spot, strike, rate, dividend_yield, maturity, sigma_cv);
  return mc_cv_estimate_(target, control, control_mean);
}

double detail::rough_bergomi_price_mc(OptionType type,
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
      volatility::detail::rough_bergomi_paths(options.steps, options.paths, spot, dt, q_params, sim_opt, out_cfg, options.seed);
  const Series terminal = sim.spot.select(0, options.steps).reshape({options.paths});
  const Series disc_payoff = discounted_payoff_(type, terminal, strike, rate, maturity);
  const Series lr = options.importance_sampling ? importance_lr_(sim.innovation_spot, options.importance_shift)
                                                : pluss::series::full({options.paths}, 1.0, out_cfg);
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
