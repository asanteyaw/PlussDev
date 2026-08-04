#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <cmath>
#include <string>

namespace pluss::subsp::math::finance::pricing {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kInvSqrt2 = 0.70710678118654752440;
constexpr double kInvSqrt2Pi = 0.39894228040143267794;

struct BsSeriesCore {
  Series price = Series::undefined();
  Series delta = Series::undefined();
  Series gamma = Series::undefined();
  Series vega = Series::undefined();
  Series theta = Series::undefined();
  Series rho = Series::undefined();
};

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_vector_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string("subsp::math::finance::pricing::") + op + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::math::finance::pricing::") + op + ": expected non-scalar Series");
  if (sizes.size() == 1) return x.reshape({sizes[0]});
  PLUSS_CHECK(sizes.size() == 2 && sizes[1] == 1,
              std::string("subsp::math::finance::pricing::") + op + ": expected vector or Nx1 matrix");
  return x.reshape({sizes[0]});
}

void validate_common_(double strike,
                      double rate,
                      double dividend_yield,
                      double volatility,
                      const char* op) {
  PLUSS_CHECK(std::isfinite(strike) && strike > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": strike must be finite and > 0");
  PLUSS_CHECK(std::isfinite(rate), std::string("subsp::math::finance::pricing::") + op + ": rate must be finite");
  PLUSS_CHECK(std::isfinite(dividend_yield),
              std::string("subsp::math::finance::pricing::") + op + ": dividend_yield must be finite");
  PLUSS_CHECK(std::isfinite(volatility) && volatility > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": volatility must be finite and > 0");
}

Series norm_cdf_(const Series& x) {
  return x.mul(-kInvSqrt2).erfc().mul(0.5);
}

Series norm_pdf_(const Series& x) {
  return x.square().mul(-0.5).exp().mul(kInvSqrt2Pi);
}

Series full_like_(const Series& like, double value, Config cfg) {
  return pluss::series::full(like.sizes(), value, cfg);
}

void validate_option_type_(OptionType type, const char* op) {
  PLUSS_CHECK(type == OptionType::Call || type == OptionType::Put,
              std::string("subsp::math::finance::pricing::") + op + ": unsupported option type");
}

BsSeriesCore bs_core_(OptionType type,
                      const Series& spot,
                      const Series& ttm,
                      double strike,
                      double rate,
                      double dividend_yield,
                      double volatility,
                      Config cfg,
                      const char* op) {
  validate_option_type_(type, op);
  validate_common_(strike, rate, dividend_yield, volatility, op);

  const auto out_cfg = f64_cfg_(cfg);
  const Series s = as_vector_(spot, op).to(out_cfg.platform, out_cfg.dtype);
  const Series tau = as_vector_(ttm, op).to(out_cfg.platform, out_cfg.dtype);
  PLUSS_CHECK(s.numel() == tau.numel(),
              std::string("subsp::math::finance::pricing::") + op + ": spot and time_to_maturity size mismatch");
  PLUSS_CHECK(s.isfinite().all().item<double>() == 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": spot contains non-finite values");
  PLUSS_CHECK(s.gt(0.0).all().item<double>() == 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": spot values must be > 0");
  PLUSS_CHECK(tau.isfinite().all().item<double>() == 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": time_to_maturity contains non-finite values");
  PLUSS_CHECK(tau.ge(0.0).all().item<double>() == 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": time_to_maturity values must be >= 0");

  const Series zeros = full_like_(s, 0.0, out_cfg);
  const Series ones = full_like_(s, 1.0, out_cfg);
  const Series half = full_like_(s, 0.5, out_cfg);
  const Series neg_ones = full_like_(s, -1.0, out_cfg);
  const Series neg_half = full_like_(s, -0.5, out_cfg);

  const Series tau_pos = tau.clamp_min(0.0);
  const Series tau_eff = tau_pos.clamp_min(1e-16);
  const Series sqrt_tau = tau_eff.sqrt();
  const Series vol_sqrt_tau = sqrt_tau.mul(volatility);

  const double carry = rate - dividend_yield + 0.5 * volatility * volatility;
  const Series d1 = s.div(strike).log().add(tau_eff.mul(carry)).div(vol_sqrt_tau);
  const Series d2 = d1.sub(vol_sqrt_tau);
  const Series cdf_d1 = norm_cdf_(d1);
  const Series cdf_d2 = norm_cdf_(d2);
  const Series pdf_d1 = norm_pdf_(d1);
  const Series disc_r = tau_pos.mul(-rate).exp();
  const Series disc_q = tau_pos.mul(-dividend_yield).exp();

  const Series call_price = s.mul(disc_q).mul(cdf_d1).sub(full_like_(s, strike, out_cfg).mul(disc_r).mul(cdf_d2));
  const Series put_price =
      full_like_(s, strike, out_cfg).mul(disc_r).mul(norm_cdf_(d2.neg())).sub(s.mul(disc_q).mul(norm_cdf_(d1.neg())));
  const Series call_delta = disc_q.mul(cdf_d1);
  const Series put_delta = disc_q.mul(cdf_d1.sub(1.0));
  const Series gamma = disc_q.mul(pdf_d1).div(s.mul(volatility).mul(sqrt_tau));
  const Series vega = s.mul(disc_q).mul(pdf_d1).mul(sqrt_tau);
  const Series call_theta = s.mul(disc_q)
                                .mul(pdf_d1)
                                .mul(-0.5 * volatility)
                                .div(sqrt_tau)
                                .sub(full_like_(s, strike, out_cfg).mul(disc_r).mul(rate).mul(cdf_d2))
                                .add(s.mul(disc_q).mul(dividend_yield).mul(cdf_d1));
  const Series put_theta = s.mul(disc_q)
                               .mul(pdf_d1)
                               .mul(-0.5 * volatility)
                               .div(sqrt_tau)
                               .add(full_like_(s, strike, out_cfg).mul(disc_r).mul(rate).mul(norm_cdf_(d2.neg())))
                               .sub(s.mul(disc_q).mul(dividend_yield).mul(norm_cdf_(d1.neg())));
  const Series call_rho = full_like_(s, strike, out_cfg).mul(tau_pos).mul(disc_r).mul(cdf_d2);
  const Series put_rho = full_like_(s, strike, out_cfg).mul(tau_pos).mul(disc_r).mul(-1.0).mul(norm_cdf_(d2.neg()));

  Series price = (type == OptionType::Call) ? call_price : put_price;
  Series delta = (type == OptionType::Call) ? call_delta : put_delta;
  Series theta = (type == OptionType::Call) ? call_theta : put_theta;
  Series rho = (type == OptionType::Call) ? call_rho : put_rho;

  const Series maturity_mask = tau_pos.le(0.0);
  const Series intrinsic_call = s.sub(strike).clamp_min(0.0);
  const Series intrinsic_put = full_like_(s, strike, out_cfg).sub(s).clamp_min(0.0);
  const Series intrinsic = (type == OptionType::Call) ? intrinsic_call : intrinsic_put;

  Series delta_call_0 = s.gt(strike).where(ones, zeros);
  delta_call_0 = s.eq(strike).where(half, delta_call_0);
  Series delta_put_0 = s.lt(strike).where(neg_ones, zeros);
  delta_put_0 = s.eq(strike).where(neg_half, delta_put_0);
  const Series delta_0 = (type == OptionType::Call) ? delta_call_0 : delta_put_0;

  price = maturity_mask.where(intrinsic, price);
  delta = maturity_mask.where(delta_0, delta);
  const Series gamma_out = maturity_mask.where(zeros, gamma);
  const Series vega_out = maturity_mask.where(zeros, vega);
  const Series theta_out = maturity_mask.where(zeros, theta);
  const Series rho_out = maturity_mask.where(zeros, rho);

  PLUSS_CHECK(price.isfinite().all().item<double>() == 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": price produced non-finite values");
  PLUSS_CHECK(delta.isfinite().all().item<double>() == 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": delta produced non-finite values");
  PLUSS_CHECK(gamma_out.isfinite().all().item<double>() == 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": gamma produced non-finite values");
  PLUSS_CHECK(vega_out.isfinite().all().item<double>() == 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": vega produced non-finite values");
  PLUSS_CHECK(theta_out.isfinite().all().item<double>() == 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": theta produced non-finite values");
  PLUSS_CHECK(rho_out.isfinite().all().item<double>() == 1.0,
              std::string("subsp::math::finance::pricing::") + op + ": rho produced non-finite values");

  return BsSeriesCore{
      .price = price,
      .delta = delta,
      .gamma = gamma_out,
      .vega = vega_out,
      .theta = theta_out,
      .rho = rho_out,
  };
}

Series scalar_series_(double x, Config cfg) {
  return pluss::series::full({1}, x, cfg);
}

Series maturity_vector_(const Series& spot, double maturity, Config cfg, const char* op) {
  PLUSS_CHECK(std::isfinite(maturity) && maturity >= 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": maturity must be finite and >= 0");
  const Series s = as_vector_(spot, op);
  return pluss::series::full({s.numel()}, maturity, cfg);
}

} // namespace

double black_scholes_price(OptionType type,
                           double spot,
                           double strike,
                           double rate,
                           double dividend_yield,
                           double maturity,
                           double volatility) {
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0,
              "subsp::math::finance::pricing::black_scholes_price: spot must be finite and > 0");
  PLUSS_CHECK(std::isfinite(maturity) && maturity >= 0.0,
              "subsp::math::finance::pricing::black_scholes_price: maturity must be finite and >= 0");
  const auto cfg = f64_cfg_({});
  const auto core = bs_core_(type,
                             scalar_series_(spot, cfg),
                             scalar_series_(maturity, cfg),
                             strike,
                             rate,
                             dividend_yield,
                             volatility,
                             cfg,
                             "black_scholes_price");
  return core.price.item<double>();
}

Series black_scholes_price(OptionType type,
                           const Series& spot,
                           double strike,
                           double rate,
                           double dividend_yield,
                           double maturity,
                           double volatility,
                           Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const auto core = bs_core_(type,
                             spot,
                             maturity_vector_(spot, maturity, out_cfg, "black_scholes_price"),
                             strike,
                             rate,
                             dividend_yield,
                             volatility,
                             out_cfg,
                             "black_scholes_price");
  return core.price.to(out_cfg.platform, out_cfg.dtype);
}

Series black_scholes_price(OptionType type,
                           const Series& spot,
                           const Series& time_to_maturity,
                           double strike,
                           double rate,
                           double dividend_yield,
                           double volatility,
                           Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const auto core =
      bs_core_(type, spot, time_to_maturity, strike, rate, dividend_yield, volatility, out_cfg, "black_scholes_price");
  return core.price.to(out_cfg.platform, out_cfg.dtype);
}

BlackScholesGreeks black_scholes_greeks(OptionType type,
                                        double spot,
                                        double strike,
                                        double rate,
                                        double dividend_yield,
                                        double maturity,
                                        double volatility) {
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0,
              "subsp::math::finance::pricing::black_scholes_greeks: spot must be finite and > 0");
  PLUSS_CHECK(std::isfinite(maturity) && maturity >= 0.0,
              "subsp::math::finance::pricing::black_scholes_greeks: maturity must be finite and >= 0");
  const auto cfg = f64_cfg_({});
  const auto core = bs_core_(type,
                             scalar_series_(spot, cfg),
                             scalar_series_(maturity, cfg),
                             strike,
                             rate,
                             dividend_yield,
                             volatility,
                             cfg,
                             "black_scholes_greeks");
  return BlackScholesGreeks{
      .delta = core.delta.item<double>(),
      .gamma = core.gamma.item<double>(),
      .vega = core.vega.item<double>(),
      .theta = core.theta.item<double>(),
      .rho = core.rho.item<double>(),
  };
}

BlackScholesSeriesGreeks black_scholes_greeks(OptionType type,
                                              const Series& spot,
                                              double strike,
                                              double rate,
                                              double dividend_yield,
                                              double maturity,
                                              double volatility,
                                              Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const auto core = bs_core_(type,
                             spot,
                             maturity_vector_(spot, maturity, out_cfg, "black_scholes_greeks"),
                             strike,
                             rate,
                             dividend_yield,
                             volatility,
                             out_cfg,
                             "black_scholes_greeks");
  return BlackScholesSeriesGreeks{
      .delta = core.delta.to(out_cfg.platform, out_cfg.dtype),
      .gamma = core.gamma.to(out_cfg.platform, out_cfg.dtype),
      .vega = core.vega.to(out_cfg.platform, out_cfg.dtype),
      .theta = core.theta.to(out_cfg.platform, out_cfg.dtype),
      .rho = core.rho.to(out_cfg.platform, out_cfg.dtype),
  };
}

BlackScholesSeriesGreeks black_scholes_greeks(OptionType type,
                                              const Series& spot,
                                              const Series& time_to_maturity,
                                              double strike,
                                              double rate,
                                              double dividend_yield,
                                              double volatility,
                                              Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const auto core = bs_core_(
      type, spot, time_to_maturity, strike, rate, dividend_yield, volatility, out_cfg, "black_scholes_greeks");
  return BlackScholesSeriesGreeks{
      .delta = core.delta.to(out_cfg.platform, out_cfg.dtype),
      .gamma = core.gamma.to(out_cfg.platform, out_cfg.dtype),
      .vega = core.vega.to(out_cfg.platform, out_cfg.dtype),
      .theta = core.theta.to(out_cfg.platform, out_cfg.dtype),
      .rho = core.rho.to(out_cfg.platform, out_cfg.dtype),
  };
}

double black_scholes_delta(OptionType type,
                           double spot,
                           double strike,
                           double rate,
                           double dividend_yield,
                           double maturity,
                           double volatility) {
  return black_scholes_greeks(type, spot, strike, rate, dividend_yield, maturity, volatility).delta;
}

double black_scholes_gamma(double spot,
                           double strike,
                           double rate,
                           double dividend_yield,
                           double maturity,
                           double volatility) {
  return black_scholes_greeks(
             OptionType::Call, spot, strike, rate, dividend_yield, maturity, volatility)
      .gamma;
}

double black_scholes_vega(double spot,
                          double strike,
                          double rate,
                          double dividend_yield,
                          double maturity,
                          double volatility) {
  return black_scholes_greeks(
             OptionType::Call, spot, strike, rate, dividend_yield, maturity, volatility)
      .vega;
}

double black_scholes_theta(OptionType type,
                           double spot,
                           double strike,
                           double rate,
                           double dividend_yield,
                           double maturity,
                           double volatility) {
  return black_scholes_greeks(type, spot, strike, rate, dividend_yield, maturity, volatility).theta;
}

double black_scholes_rho(OptionType type,
                         double spot,
                         double strike,
                         double rate,
                         double dividend_yield,
                         double maturity,
                         double volatility) {
  return black_scholes_greeks(type, spot, strike, rate, dividend_yield, maturity, volatility).rho;
}

} // namespace pluss::subsp::math::finance::pricing
