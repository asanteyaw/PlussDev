#include "pluss/subsp/math/finance/objects.h"
#include "pluss/subsp/math/finance/module.h"
#include "validation_internal.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::object {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kInvSqrt2 = 0.70710678118654752440;
constexpr double kInvSqrt2Pi = 0.39894228040143267794;
constexpr double kTiny = 1e-12;

Config f64_cfg_(Config cfg, Platform platform) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(platform).with_pinned(cfg.pinned);
}

Series as_series_vec_(const Series& x, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::math::finance::object::price: ") + name + " must be defined");
  const auto sz = x.sizes();
  if (sz.empty()) return x.reshape({1});
  if (sz.size() == 1) return x;
  PLUSS_CHECK(sz.size() == 2 && sz[1] == 1,
              std::string("subsp::math::finance::object::price: ") + name + " must be vector-like");
  return x.reshape({sz[0]});
}

Series broadcast_ref_(const Series& a,
                      const Series& b,
                      const Series& c,
                      const Series& d,
                      const Series& e,
                      const Series& f,
                      Config cfg) {
  Series z = a.to(cfg.platform, cfg.dtype).mul(0.0);
  z = z.add(b.to(cfg.platform, cfg.dtype).mul(0.0));
  z = z.add(c.to(cfg.platform, cfg.dtype).mul(0.0));
  z = z.add(d.to(cfg.platform, cfg.dtype).mul(0.0));
  z = z.add(e.to(cfg.platform, cfg.dtype).mul(0.0));
  z = z.add(f.to(cfg.platform, cfg.dtype).mul(0.0));
  return z;
}

Series expand_like_(const Series& x, const Series& like, Config cfg) {
  return x.to(cfg.platform, cfg.dtype).add(like.mul(0.0));
}

Series option_is_call_mask_(const Vanilla& inst, const Series& ref, Config cfg, const char* op) {
  if (inst.type == pricing::OptionType::Call) {
    return pluss::series::full(ref.sizes(), 1.0, cfg);
  }
  if (inst.type == pricing::OptionType::Put) {
    return pluss::series::full(ref.sizes(), 0.0, cfg);
  }
  PLUSS_CHECK(inst.type == pricing::OptionType::Auto,
              std::string("subsp::math::finance::object::") + op + ": option type must be Call or Put (OptionType::Auto is only valid for an instrument that supplies is_call)");
  PLUSS_CHECK(inst.is_call.defined(),
              std::string("subsp::math::finance::object::") + op +
                  ": instrument.is_call must be defined when instrument.type=OptionType::Auto");
  const Series mask = expand_like_(as_series_vec_(inst.is_call, "instrument.is_call"), ref, cfg);
  PLUSS_CHECK(mask.ge(0.0).all().item<double>() == 1.0 && mask.le(1.0).all().item<double>() == 1.0,
              std::string("subsp::math::finance::object::") + op +
                  ": instrument.is_call must contain values in [0,1]");
  return mask;
}

pricing::OptionType option_type_from_mask_(const Series& is_call_mask_flat, std::int64_t i) {
  const double v = is_call_mask_flat.select(0, i).reshape({1}).select(0, 0).item<double>();
  return (v >= 0.5) ? pricing::OptionType::Call : pricing::OptionType::Put;
}

// Option-by-option Greeks: calls `one(spot, strike, rate, yield, maturity, is_call[, index])` for every option of the strip and collects the results.
template <typename One>
PriceResult group_greeks_(const Vanilla& inst, const MarketState& mkt, Config out_cfg, One&& one) {
  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"), as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"), as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"), pluss::series::full({1}, 0.0, out_cfg), out_cfg);
  const std::int64_t n = ref.numel();
  const auto host = [&](const Series& x, const char* name) {
    const auto h = expand_like_(as_series_vec_(x, name), ref, out_cfg).reshape({n}).to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
    return std::vector<double>(h.begin(), h.end());
  };
  const auto spot = host(mkt.spot, "market.spot"), strike = host(inst.strike, "instrument.strike"), maturity = host(inst.maturity, "instrument.maturity");
  const auto rate = host(mkt.rate, "market.rate"), yield = host(mkt.dividend_yield, "market.dividend_yield");
  const auto mask = host(inst.type == pricing::OptionType::Auto ? inst.is_call : pluss::series::full({1}, inst.type == pricing::OptionType::Call ? 1.0 : 0.0, out_cfg), "instrument.is_call");
  std::vector<double> price(n), delta(n), gamma(n), vega(n), theta(n), rho(n);
  for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i) {
    pricing::detail::ScalarGreeks r;
    if constexpr (std::is_invocable_v<One, double, double, double, double, double, bool, std::size_t>) r = one(spot[i], strike[i], rate[i], yield[i], maturity[i], mask[i] >= 0.5, i);
    else r = one(spot[i], strike[i], rate[i], yield[i], maturity[i], mask[i] >= 0.5);
    price[i] = r.price; delta[i] = r.delta; gamma[i] = r.gamma; vega[i] = r.vega; theta[i] = r.theta; rho[i] = r.rho;
  }
  const auto shape = [&](const std::vector<double>& v) { return pluss::make_series(v, {n}, out_cfg).reshape(ref.sizes()); };
  return PriceResult{.price = shape(price), .delta = shape(delta), .gamma = shape(gamma), .vega = shape(vega), .theta = shape(theta), .rho = shape(rho)};
}

Series norm_cdf_(const Series& x) {
  return x.mul(-kInvSqrt2).erfc().mul(0.5);
}

Series norm_pdf_(const Series& x) {
  return x.square().mul(-0.5).exp().mul(kInvSqrt2Pi);
}

double scalar_from_series_(const Series& x, const char* name, Config cfg) {
  const Series v = as_series_vec_(x, name).to(cfg.platform, cfg.dtype);
  PLUSS_CHECK(v.numel() == 1,
              std::string("subsp::math::finance::object::price: ") + name + " must have exactly one element");
  return v.reshape({1}).select(0, 0).item<double>();
}

double scalar_or_default_(const Series& x, double fallback, const char* name, Config cfg) {
  if (!x.defined()) return fallback;
  return scalar_from_series_(x, name, cfg);
}

volatility::HestonParams heston_params_from_series_(const model::Heston& mdl, Config cfg) {
  const double kappa = scalar_from_series_(mdl.kappa, "model.heston.kappa", cfg);
  const double theta = scalar_from_series_(mdl.theta, "model.heston.theta", cfg);
  const double sigma = scalar_from_series_(mdl.sigma, "model.heston.sigma", cfg);
  const double rho = scalar_from_series_(mdl.rho, "model.heston.rho", cfg);
  const double v0 = scalar_from_series_(mdl.v0, "model.heston.v0", cfg);
  const double mu = scalar_or_default_(mdl.mu, 0.0, "model.heston.mu", cfg);
  const double variance_floor =
      scalar_or_default_(mdl.variance_floor, 1e-12, "model.heston.variance_floor", cfg);
  return volatility::HestonParams{
      .kappa = kappa,
      .theta = theta,
      .sigma = sigma,
      .rho = rho,
      .v0 = v0,
      .mu = mu,
      .variance_floor = variance_floor,
  };
}

volatility::BatesParams bates_params_from_series_(const model::Bates& mdl, Config cfg) {
  const double kappa = scalar_from_series_(mdl.kappa, "model.bates.kappa", cfg);
  const double theta = scalar_from_series_(mdl.theta, "model.bates.theta", cfg);
  const double sigma = scalar_from_series_(mdl.sigma, "model.bates.sigma", cfg);
  const double rho = scalar_from_series_(mdl.rho, "model.bates.rho", cfg);
  const double v0 = scalar_from_series_(mdl.v0, "model.bates.v0", cfg);
  const double mu = scalar_or_default_(mdl.mu, 0.0, "model.bates.mu", cfg);
  const double variance_floor = scalar_or_default_(mdl.variance_floor, 1e-12, "model.bates.variance_floor", cfg);
  const double jump_intensity =
      scalar_or_default_(mdl.jump_intensity, 0.15, "model.bates.jump_intensity", cfg);
  const double jump_mean = scalar_or_default_(mdl.jump_mean, -0.08, "model.bates.jump_mean", cfg);
  const double jump_vol = scalar_or_default_(mdl.jump_vol, 0.20, "model.bates.jump_vol", cfg);
  return volatility::BatesParams{
      .kappa = kappa,
      .theta = theta,
      .sigma = sigma,
      .rho = rho,
      .v0 = v0,
      .mu = mu,
      .variance_floor = variance_floor,
      .jump_intensity = jump_intensity,
      .jump_mean = jump_mean,
      .jump_vol = jump_vol,
  };
}

volatility::SabrParams sabr_params_from_series_(const model::Sabr& mdl, Config cfg) {
  const double alpha0 = scalar_from_series_(mdl.alpha0, "model.sabr.alpha0", cfg);
  const double beta = scalar_from_series_(mdl.beta, "model.sabr.beta", cfg);
  const double nu = scalar_from_series_(mdl.nu, "model.sabr.nu", cfg);
  const double rho = scalar_from_series_(mdl.rho, "model.sabr.rho", cfg);
  const double mu = scalar_or_default_(mdl.mu, 0.0, "model.sabr.mu", cfg);
  const double alpha_floor = scalar_or_default_(mdl.alpha_floor, 1e-12, "model.sabr.alpha_floor", cfg);
  return volatility::SabrParams{
      .alpha0 = alpha0,
      .beta = beta,
      .nu = nu,
      .rho = rho,
      .mu = mu,
      .alpha_floor = alpha_floor,
  };
}

volatility::RoughHestonParams rough_heston_params_from_series_(const model::RoughHeston& mdl, Config cfg) {
  const double kappa = scalar_from_series_(mdl.kappa, "model.rough_heston.kappa", cfg);
  const double theta = scalar_from_series_(mdl.theta, "model.rough_heston.theta", cfg);
  const double sigma = scalar_from_series_(mdl.sigma, "model.rough_heston.sigma", cfg);
  const double alpha = scalar_from_series_(mdl.alpha, "model.rough_heston.alpha", cfg);
  const double rho = scalar_from_series_(mdl.rho, "model.rough_heston.rho", cfg);
  const double v0 = scalar_from_series_(mdl.v0, "model.rough_heston.v0", cfg);
  const double mu = scalar_or_default_(mdl.mu, 0.0, "model.rough_heston.mu", cfg);
  const double variance_floor =
      scalar_or_default_(mdl.variance_floor, 1e-12, "model.rough_heston.variance_floor", cfg);
  return volatility::RoughHestonParams{
      .mu = mu,
      .v0 = v0,
      .theta = theta,
      .kappa = kappa,
      .sigma = sigma,
      .alpha = alpha,
      .rho = rho,
      .variance_floor = variance_floor,
  };
}

volatility::RoughBergomiParams rough_bergomi_params_from_series_(const model::RoughBergomi& mdl, Config cfg) {
  const double xi = scalar_from_series_(mdl.xi, "model.rough_bergomi.xi", cfg);
  const double eta = scalar_from_series_(mdl.eta, "model.rough_bergomi.eta", cfg);
  const double alpha = scalar_from_series_(mdl.alpha, "model.rough_bergomi.alpha", cfg);
  const double rho = scalar_from_series_(mdl.rho, "model.rough_bergomi.rho", cfg);
  const double mu = scalar_or_default_(mdl.mu, 0.0, "model.rough_bergomi.mu", cfg);
  const double variance_floor =
      scalar_or_default_(mdl.variance_floor, 1e-12, "model.rough_bergomi.variance_floor", cfg);
  return volatility::RoughBergomiParams{
      .mu = mu,
      .xi = xi,
      .eta = eta,
      .alpha = alpha,
      .rho = rho,
      .variance_floor = variance_floor,
  };
}

PriceResult bs_price_analytic_(const Vanilla& inst,
                               const model::BlackScholes& mdl,
                               const MarketState& mkt,
                               bool with_greeks,
                               Config cfg) {
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"),
                                    as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"),
                                    as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"),
                                    as_series_vec_(mdl.volatility, "model.volatility"),
                                    out_cfg);
  const Series is_call_mask = option_is_call_mask_(inst, ref, out_cfg, "price");
  const Series is_put_mask = is_call_mask.mul(-1.0).add(1.0);

  const Series spot = expand_like_(as_series_vec_(mkt.spot, "market.spot"), ref, out_cfg).clamp_min(kTiny);
  const Series strike = expand_like_(as_series_vec_(inst.strike, "instrument.strike"), ref, out_cfg).clamp_min(kTiny);
  const Series tau = expand_like_(as_series_vec_(inst.maturity, "instrument.maturity"), ref, out_cfg).clamp_min(0.0);
  const Series rate = expand_like_(as_series_vec_(mkt.rate, "market.rate"), ref, out_cfg);
  const Series div = expand_like_(as_series_vec_(mkt.dividend_yield, "market.dividend_yield"), ref, out_cfg);
  const Series vol = expand_like_(as_series_vec_(mdl.volatility, "model.volatility"), ref, out_cfg).clamp_min(kTiny);

  const Series zeros = pluss::series::full(ref.sizes(), 0.0, out_cfg);
  const Series ones = pluss::series::full(ref.sizes(), 1.0, out_cfg);
  const Series half = pluss::series::full(ref.sizes(), 0.5, out_cfg);
  const Series neg_ones = pluss::series::full(ref.sizes(), -1.0, out_cfg);
  const Series neg_half = pluss::series::full(ref.sizes(), -0.5, out_cfg);

  const Series tau_eff = tau.clamp_min(kTiny);
  const Series sqrt_tau = tau_eff.sqrt();
  const Series vol_sqrt_tau = vol.mul(sqrt_tau);
  const Series carry = rate.sub(div).add(vol.square().mul(0.5));
  const Series d1 = spot.div(strike).log().add(carry.mul(tau_eff)).div(vol_sqrt_tau);
  const Series d2 = d1.sub(vol_sqrt_tau);
  const Series disc_r = rate.mul(-1.0).mul(tau).exp();
  const Series disc_q = div.mul(-1.0).mul(tau).exp();
  const Series cdf_d1 = norm_cdf_(d1);
  const Series cdf_d2 = norm_cdf_(d2);
  const Series pdf_d1 = norm_pdf_(d1);

  const Series call_price = spot.mul(disc_q).mul(cdf_d1).sub(strike.mul(disc_r).mul(cdf_d2));
  const Series put_price = strike.mul(disc_r).mul(norm_cdf_(d2.neg())).sub(spot.mul(disc_q).mul(norm_cdf_(d1.neg())));
  Series price = call_price.mul(is_call_mask).add(put_price.mul(is_put_mask));

  const Series maturity_mask = tau.le(0.0);
  const Series intrinsic_call = spot.sub(strike).clamp_min(0.0);
  const Series intrinsic_put = strike.sub(spot).clamp_min(0.0);
  const Series intrinsic = intrinsic_call.mul(is_call_mask).add(intrinsic_put.mul(is_put_mask));
  price = maturity_mask.where(intrinsic, price);

  PriceResult out;
  out.price = price;
  if (!with_greeks) {
    return out;
  }

  const Series call_delta = disc_q.mul(cdf_d1);
  const Series put_delta = disc_q.mul(cdf_d1.sub(1.0));
  const Series gamma = disc_q.mul(pdf_d1).div(spot.mul(vol).mul(sqrt_tau));
  const Series vega = spot.mul(disc_q).mul(pdf_d1).mul(sqrt_tau);
  const Series call_theta = spot.mul(disc_q)
                                .mul(pdf_d1)
                                .mul(-0.5)
                                .mul(vol)
                                .div(sqrt_tau)
                                .sub(strike.mul(disc_r).mul(rate).mul(cdf_d2))
                                .add(spot.mul(disc_q).mul(div).mul(cdf_d1));
  const Series put_theta = spot.mul(disc_q)
                               .mul(pdf_d1)
                               .mul(-0.5)
                               .mul(vol)
                               .div(sqrt_tau)
                               .add(strike.mul(disc_r).mul(rate).mul(norm_cdf_(d2.neg())))
                               .sub(spot.mul(disc_q).mul(div).mul(norm_cdf_(d1.neg())));
  const Series call_rho = strike.mul(tau).mul(disc_r).mul(cdf_d2);
  const Series put_rho = strike.mul(tau).mul(disc_r).mul(-1.0).mul(norm_cdf_(d2.neg()));

  Series delta = call_delta.mul(is_call_mask).add(put_delta.mul(is_put_mask));
  const Series theta = call_theta.mul(is_call_mask).add(put_theta.mul(is_put_mask));
  const Series rho = call_rho.mul(is_call_mask).add(put_rho.mul(is_put_mask));

  Series delta_call_0 = spot.gt(strike).where(ones, zeros);
  delta_call_0 = spot.eq(strike).where(half, delta_call_0);
  Series delta_put_0 = spot.lt(strike).where(neg_ones, zeros);
  delta_put_0 = spot.eq(strike).where(neg_half, delta_put_0);
  const Series delta_0 = delta_call_0.mul(is_call_mask).add(delta_put_0.mul(is_put_mask));
  delta = maturity_mask.where(delta_0, delta);

  out.delta = delta;
  out.gamma = maturity_mask.where(zeros, gamma);
  out.vega = maturity_mask.where(zeros, vega);
  out.theta = maturity_mask.where(zeros, theta);
  out.rho = maturity_mask.where(zeros, rho);
  return out;
}

PriceResult bs_price_mc_(const Vanilla& inst,
                         const model::BlackScholes& mdl,
                         const MarketState& mkt,
                         const pricer::MonteCarlo& eng,
                         Config cfg,
                         bool greeks = false) {
  PLUSS_CHECK(eng.options.paths > 0, "subsp::math::finance::object::price: mc paths must be > 0");

  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"),
                                    as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"),
                                    as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"),
                                    as_series_vec_(mdl.volatility, "model.volatility"),
                                    out_cfg);
  const Series is_call_mask = option_is_call_mask_(inst, ref, out_cfg, "price");
  const Series is_put_mask = is_call_mask.mul(-1.0).add(1.0);

  const Series spot = expand_like_(as_series_vec_(mkt.spot, "market.spot"), ref, out_cfg).clamp_min(kTiny);
  const Series strike = expand_like_(as_series_vec_(inst.strike, "instrument.strike"), ref, out_cfg).clamp_min(kTiny);
  const Series tau = expand_like_(as_series_vec_(inst.maturity, "instrument.maturity"), ref, out_cfg).clamp_min(0.0);
  const Series rate = expand_like_(as_series_vec_(mkt.rate, "market.rate"), ref, out_cfg);
  const Series div = expand_like_(as_series_vec_(mkt.dividend_yield, "market.dividend_yield"), ref, out_cfg);
  const Series vol = expand_like_(as_series_vec_(mdl.volatility, "model.volatility"), ref, out_cfg).clamp_min(kTiny);

  const std::int64_t n = spot.numel();
  const Series drift = rate.sub(div).sub(vol.square().mul(0.5)).mul(tau).reshape({1, n});
  const Series volsqrt = vol.mul(tau.sqrt()).reshape({1, n});
  const Series s0 = spot.reshape({1, n});
  const Series k = strike.reshape({1, n});
  const Series disc = rate.mul(-1.0).mul(tau).exp().reshape({1, n});
  const Series call_mask_row = is_call_mask.reshape({1, n});
  const Series put_mask_row = is_put_mask.reshape({1, n});

  Series price_vec = pluss::series::undefined();
  if (!eng.options.antithetic) {
    const Series z = pluss::series::randn({eng.options.paths, n}, out_cfg, eng.options.seed);
    const Series terminal = drift.add(volsqrt.mul(z)).exp().mul(s0);
    const Series call_payoff = terminal.sub(k).clamp_min(0.0);
    const Series put_payoff = k.sub(terminal).clamp_min(0.0);
    const Series payoff = call_payoff.mul(call_mask_row).add(put_payoff.mul(put_mask_row));
    price_vec = payoff.mul(disc).mean(0, false);
  } else {
    const std::int64_t half_paths = std::max<std::int64_t>(1, eng.options.paths / 2);
    const Series z = pluss::series::randn({half_paths, n}, out_cfg, eng.options.seed);
    const Series terminal_pos = drift.add(volsqrt.mul(z)).exp().mul(s0);
    const Series terminal_neg = drift.sub(volsqrt.mul(z)).exp().mul(s0);
    const Series call_payoff_pos = terminal_pos.sub(k).clamp_min(0.0);
    const Series put_payoff_pos = k.sub(terminal_pos).clamp_min(0.0);
    const Series payoff_pos = call_payoff_pos.mul(call_mask_row).add(put_payoff_pos.mul(put_mask_row));
    const Series call_payoff_neg = terminal_neg.sub(k).clamp_min(0.0);
    const Series put_payoff_neg = k.sub(terminal_neg).clamp_min(0.0);
    const Series payoff_neg = call_payoff_neg.mul(call_mask_row).add(put_payoff_neg.mul(put_mask_row));
    price_vec = payoff_pos.add(payoff_neg).mul(0.5).mul(disc).mean(0, false);
  }

  PriceResult out;
  out.price = price_vec.reshape(ref.sizes());
  if (!greeks) return out;

  // Greeks from the same normals, in closed pathwise form: with S_T = S exp((r - q - v^2/2) T + v sqrt(T) Z) and sign = +1 for a call and -1 for a put,
  //   delta = e^{-rT} E[sign 1{exercise} S_T] / S,      vega = e^{-rT} E[sign 1{exercise} S_T (sqrt(T) Z - v T)],
  //   rho = -T V + e^{-rT} T E[sign 1{exercise} S_T],   dV/dT = -r V + e^{-rT} E[sign 1{exercise} S_T (r - q - v^2/2 + v Z / (2 sqrt(T)))],
  // theta = -dV/dT, and gamma by a second difference of the payoff in the spot scale on the same normals.
  const Series sign = call_mask_row.sub(put_mask_row);
  const Series tau_row = tau.reshape({1, n}), vol_row = vol.reshape({1, n}), rate_row = rate.reshape({1, n}), div_row = div.reshape({1, n});
  const Series sqrt_tau = tau_row.sqrt();
  const auto estimate = [&](const Series& z) {
    // returns {price, delta, gamma, vega, rho, dV/dT} sample means for the normals z ([paths, n])
    const Series terminal = drift.add(volsqrt.mul(z)).exp().mul(s0);
    const Series itm = call_mask_row.mul(terminal.gt(k).to(pluss::ScalarType::Float64)).add(put_mask_row.mul(terminal.lt(k).to(pluss::ScalarType::Float64)));
    const Series flow = sign.mul(itm).mul(terminal);   // sign * 1{exercise} * S_T
    const auto payoff_at = [&](double scale) {
      const Series st = terminal.mul(scale);
      return st.sub(k).clamp_min(0.0).mul(call_mask_row).add(k.sub(st).clamp_min(0.0).mul(put_mask_row)).mul(disc).mean(0, false);
    };
    constexpr double eps = 0.01;
    const Series price0 = payoff_at(1.0);
    std::vector<Series> r;
    r.push_back(price0);
    r.push_back(flow.mul(disc).mean(0, false).div(s0.reshape({n})));
    r.push_back(payoff_at(1.0 + eps).sub(price0.mul(2.0)).add(payoff_at(1.0 - eps)).div(s0.reshape({n}).square().mul(eps * eps)));
    r.push_back(flow.mul(sqrt_tau.mul(z).sub(vol_row.mul(tau_row))).mul(disc).mean(0, false));
    r.push_back(flow.mul(disc).mean(0, false).mul(tau.reshape({n})).sub(price0.mul(tau.reshape({n}))));
    r.push_back(flow.mul(rate_row.sub(div_row).sub(vol_row.square().mul(0.5)).add(vol_row.mul(z).div(sqrt_tau.mul(2.0).clamp_min(1e-12)))).mul(disc).mean(0, false).sub(price0.mul(rate.reshape({n}))));
    return r;
  };
  std::vector<Series> parts;
  if (!eng.options.antithetic) {
    parts = estimate(pluss::series::randn({eng.options.paths, n}, out_cfg, eng.options.seed));
  } else {
    const std::int64_t half_paths = std::max<std::int64_t>(1, eng.options.paths / 2);
    const Series z = pluss::series::randn({half_paths, n}, out_cfg, eng.options.seed);
    const auto a = estimate(z), b = estimate(z.mul(-1.0));
    for (std::size_t m = 0; m < a.size(); ++m) parts.push_back(a[m].add(b[m]).mul(0.5));
  }
  // options at expiry: the price is the payoff, delta is the exercise indicator and the other Greeks vanish
  const Series expired = tau.reshape({n}).le(1e-12);
  const Series zero = pluss::series::zeros({n}, out_cfg);
  const auto live = [&](const Series& x) { return expired.where(zero, x).reshape(ref.sizes()); };
  out.delta = parts[1].reshape(ref.sizes());
  out.gamma = live(parts[2]);
  out.vega = live(parts[3]);
  out.rho = live(parts[4]);
  out.theta = live(parts[5].mul(-1.0));
  return out;
}

void require_european_(const Vanilla& inst, const char* engine) {
  PLUSS_CHECK(inst.exercise == pricing::ExerciseStyle::European,
              std::string("subsp::math::finance::object::price: the ") + engine +
                  " engine prices European exercise only; use the MonteCarlo engine (any model) or the Lattice engine (BlackScholes) for American and Bermudan options");
}

std::vector<double> exercise_dates_(const Vanilla& inst, Config cfg) {
  if (inst.exercise != pricing::ExerciseStyle::Bermudan) return {};
  PLUSS_CHECK(inst.exercise_times.defined() && inst.exercise_times.numel() > 0,
              "subsp::math::finance::object::price: a Bermudan instrument needs instrument.exercise_times");
  return as_series_vec_(inst.exercise_times, "instrument.exercise_times").to(pluss::pCPU, cfg.dtype).host<double>();
}

volatility::SimOptions exercise_sim_options_(const pricing::MonteCarloOptions& o) {
  PLUSS_CHECK(!o.control_variate && !o.importance_sampling,
              "subsp::math::finance::object::price: control variates and importance sampling do not apply to American/Bermudan exercise");
  volatility::SimOptions sim;
  sim.scheme = o.scheme;
  sim.method = o.rand_method;
  sim.randomized_quasi = o.randomized_quasi;
  sim.sobol_scrambling = o.sobol_scrambling;
  sim.antithetic = o.antithetic;
  sim.paths_first = true;
  sim.seed = o.seed;
  sim.offset = o.offset;
  return sim;
}

using LsmTracked = pricing::detail::LsmTracked;

volatility::SimOptions tracked_sim_options_(const pricing::MonteCarloOptions& o, const LsmTracked& tracked) {
  auto sim = exercise_sim_options_(o);
  sim.tracked_level = tracked.level;
  sim.tracked_spot = tracked.spot;
  sim.tracked_drift = tracked.drift;
  return sim;
}

// Asset paths [paths, steps + 1] under the risk-neutral drift rate - q, one overload per model.
Series exercise_paths_(const model::BlackScholes& mdl, double spot, double rate, double q, double maturity,
                       const pricing::MonteCarloOptions& o, Config cfg, const LsmTracked& tracked = {}) {
  const volatility::GbmParams p{.mu = rate - q, .sigma = scalar_from_series_(mdl.volatility, "model.volatility", cfg)};
  return volatility::detail::gbm_paths(o.steps, o.paths, spot, maturity / static_cast<double>(o.steps), p, tracked_sim_options_(o, tracked), cfg, o.seed).spot;
}

Series exercise_paths_(const model::Heston& mdl, double spot, double rate, double q, double maturity,
                       const pricing::MonteCarloOptions& o, Config cfg, const LsmTracked& tracked = {}) {
  auto p = heston_params_from_series_(mdl, cfg);
  p.mu = rate - q;
  return volatility::detail::heston_paths(o.steps, o.paths, spot, maturity / static_cast<double>(o.steps), p, tracked_sim_options_(o, tracked), cfg, o.seed).spot;
}

Series exercise_paths_(const model::Bates& mdl, double spot, double rate, double q, double maturity,
                       const pricing::MonteCarloOptions& o, Config cfg, const LsmTracked& tracked = {}) {
  auto p = bates_params_from_series_(mdl, cfg);
  p.mu = rate - q;
  return volatility::detail::bates_paths(o.steps, o.paths, spot, maturity / static_cast<double>(o.steps), p, tracked_sim_options_(o, tracked), cfg, o.seed).spot;
}

Series exercise_paths_(const model::Sabr& mdl, double spot, double rate, double q, double maturity,
                       const pricing::MonteCarloOptions& o, Config cfg, const LsmTracked& tracked = {}) {
  auto p = sabr_params_from_series_(mdl, cfg);
  p.mu = rate - q;
  return volatility::detail::sabr_paths(o.steps, o.paths, spot, maturity / static_cast<double>(o.steps), p, tracked_sim_options_(o, tracked), cfg, o.seed).spot;
}

Series exercise_paths_(const model::RoughHeston& mdl, double spot, double rate, double q, double maturity,
                       const pricing::MonteCarloOptions& o, Config cfg, const LsmTracked& tracked = {}) {
  auto p = rough_heston_params_from_series_(mdl, cfg);
  p.mu = rate - q;
  return volatility::detail::rough_heston_paths(o.steps, o.paths, spot, maturity / static_cast<double>(o.steps), p, tracked_sim_options_(o, tracked), cfg, o.seed).spot;
}

Series exercise_paths_(const model::RoughBergomi& mdl, double spot, double rate, double q, double maturity,
                       const pricing::MonteCarloOptions& o, Config cfg, const LsmTracked& tracked = {}) {
  auto p = rough_bergomi_params_from_series_(mdl, cfg);
  p.mu = rate - q;
  return volatility::detail::rough_bergomi_paths(o.steps, o.paths, spot, maturity / static_cast<double>(o.steps), p, tracked_sim_options_(o, tracked), cfg, o.seed).spot;
}

// Early-exercise COS (Heston, Bates): one price per option, over the broadcast instrument and market.
template <typename Model, typename PriceFn>
PriceResult cos_exercise_(const Vanilla& inst, const Model&, const MarketState& mkt, Config cfg, const PriceFn& price_one) {
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto dates = exercise_dates_(inst, out_cfg);
  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"),
                                    as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"),
                                    as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"),
                                    pluss::series::full({1}, 0.0, out_cfg),
                                    out_cfg);
  const auto n = ref.numel();
  auto column = [&](const Series& x, const char* name) { return expand_like_(as_series_vec_(x, name), ref, out_cfg).reshape({n}).to(pluss::pCPU, out_cfg.dtype).host<double>(); };
  const auto spot = column(mkt.spot, "market.spot");
  const auto strike = column(inst.strike, "instrument.strike");
  const auto maturity = column(inst.maturity, "instrument.maturity");
  const auto rate = column(mkt.rate, "market.rate");
  const auto q = column(mkt.dividend_yield, "market.dividend_yield");
  const auto is_call = option_is_call_mask_(inst, ref, out_cfg, "price").reshape({n}).to(pluss::pCPU, out_cfg.dtype).host<double>();
  std::vector<double> px(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const auto k = static_cast<std::size_t>(i);
    px[k] = price_one(is_call[k] >= 0.5 ? pricing::OptionType::Call : pricing::OptionType::Put, spot[k], strike[k], rate[k], q[k], maturity[k], dates);
  }
  return PriceResult{.price = pluss::make_series(px, {n}, out_cfg).reshape(ref.sizes())};
}

// American/Bermudan by Longstaff-Schwartz: fresh paths per option, over that option's own life.
template <typename Model>
PriceResult exercise_mc_(const Vanilla& inst, const Model& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  PLUSS_CHECK(eng.options.paths > 0 && eng.options.steps > 0,
              "subsp::math::finance::object::price: mc paths and steps must be > 0");
  const auto dates = exercise_dates_(inst, out_cfg);
  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"),
                                    as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"),
                                    as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"),
                                    pluss::series::full({1}, 0.0, out_cfg),
                                    out_cfg);
  const auto n = ref.numel();
  auto column = [&](const Series& x, const char* name) { return expand_like_(as_series_vec_(x, name), ref, out_cfg).reshape({n}).to(pluss::pCPU, out_cfg.dtype).host<double>(); };
  const auto spot = column(mkt.spot, "market.spot");
  const auto strike = column(inst.strike, "instrument.strike");
  const auto maturity = column(inst.maturity, "instrument.maturity");
  const auto rate = column(mkt.rate, "market.rate");
  const auto q = column(mkt.dividend_yield, "market.dividend_yield");
  const auto is_call = option_is_call_mask_(inst, ref, out_cfg, "price").reshape({n}).to(pluss::pCPU, out_cfg.dtype).host<double>();
  std::vector<double> px(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const auto k = static_cast<std::size_t>(i);
    PLUSS_CHECK(maturity[k] > 0.0, "subsp::math::finance::object::price: American/Bermudan maturity must be > 0");
    const Series paths = exercise_paths_(mdl, spot[k], rate[k], q[k], maturity[k], eng.options, out_cfg);
    px[k] = pricing::detail::lsm_price(is_call[k] >= 0.5 ? pricing::OptionType::Call : pricing::OptionType::Put, inst.exercise, strike[k],
                               rate[k], maturity[k], paths, dates, eng.options.basis_degree);
  }
  return PriceResult{.price = pluss::make_series(px, {n}, out_cfg).reshape(ref.sizes())};
}


// the volatility level that vega differentiates: initial variance (Heston family), forward variance (rough Bergomi), constant volatility (Black-Scholes)
const Series &exercise_level_(const model::BlackScholes &m) { return m.volatility; }
const Series &exercise_level_(const model::Heston &m) { return m.v0; }
const Series &exercise_level_(const model::Bates &m) { return m.v0; }
const Series &exercise_level_(const model::RoughHeston &m) { return m.v0; }
const Series &exercise_level_(const model::RoughBergomi &m) { return m.xi; }
const Series &exercise_level_(const model::Sabr &m) { return m.alpha0; }

// Black-Scholes values solve their pricing equation (closed-form theta); SABR paths are not proportional to the spot (tracked spot and drift)
pricing::detail::LsmModel lsm_model_(const model::BlackScholes &, double level, double carry) { return {.homogeneous = true, .carry = carry, .bs_volatility = level}; }
pricing::detail::LsmModel lsm_model_(const model::Sabr &, double, double carry) { return {.homogeneous = false, .carry = carry}; }
template <typename Model>
pricing::detail::LsmModel lsm_model_(const Model &, double, double carry) { return {.homogeneous = true, .carry = carry}; }

// American/Bermudan Longstaff-Schwartz Greeks of one simulation per option: the regression is fitted once and the Greeks follow from the fitted exercise rule
// (pricing::detail::lsm_greeks), with the volatility level of the model tracked for vega.
template <typename Model>
PriceResult exercise_mc_greeks_(const Vanilla& inst, const Model& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  PLUSS_CHECK(eng.options.paths > 0 && eng.options.steps > 0, "subsp::math::finance::object::price: mc paths and steps must be > 0");
  const auto dates = exercise_dates_(inst, out_cfg);
  const double level = exercise_level_(mdl).to(pluss::pCPU, ScalarType::Float64).reshape({exercise_level_(mdl).numel()}).template host<double>().at(0);
  return group_greeks_(inst, mkt, out_cfg, [&](double s, double k, double r, double q, double t, bool call) {
    PLUSS_CHECK(t > 0.0, "subsp::math::finance::object::price: American/Bermudan maturity must be > 0");
    const auto simulate = [&](const LsmTracked& tracked) { return exercise_paths_(mdl, s * tracked.spot_scale, r, q, t, eng.options, out_cfg, tracked); };
    return pricing::detail::lsm_greeks(call ? pricing::OptionType::Call : pricing::OptionType::Put, inst.exercise, s, k, r, t, level, simulate, dates, eng.options.basis_degree, lsm_model_(mdl, level, r - q));
  });
}

} // namespace

PriceResult price(const Vanilla& inst,
                  const model::BlackScholes& mdl,
                  const MarketState& mkt,
                  const pricer::Analytic& eng,
                  Config cfg) {
  require_european_(inst, "Analytic");
  return bs_price_analytic_(inst, mdl, mkt, eng.with_greeks, cfg);
}

PriceResult price(const Vanilla& inst,
                  const model::BlackScholes& mdl,
                  const MarketState& mkt,
                  const pricer::MonteCarlo& eng,
                  Config cfg) {
  if (inst.exercise != pricing::ExerciseStyle::European) return exercise_mc_(inst, mdl, mkt, eng, cfg);
  return bs_price_mc_(inst, mdl, mkt, eng, cfg);
}

PriceResult price(const Vanilla& inst,
                  const model::BlackScholes& mdl,
                  const MarketState& mkt,
                  const pricer::Spectral& eng,
                  Config cfg) {
  PLUSS_CHECK(inst.exercise != pricing::ExerciseStyle::Bermudan,
              "subsp::math::finance::object::price: the Spectral engine prices European and American exercise; use the Lattice or MonteCarlo engine for a Bermudan option");
  if (inst.exercise == pricing::ExerciseStyle::European) return bs_price_analytic_(inst, mdl, mkt, false, cfg);
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"),
                                    as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"),
                                    as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"),
                                    as_series_vec_(mdl.volatility, "model.volatility"),
                                    out_cfg);
  const auto n = ref.numel();
  auto column = [&](const Series& x, const char* name) { return expand_like_(as_series_vec_(x, name), ref, out_cfg).reshape({n}).to(pluss::pCPU, out_cfg.dtype).host<double>(); };
  const auto spot = column(mkt.spot, "market.spot");
  const auto strike = column(inst.strike, "instrument.strike");
  const auto maturity = column(inst.maturity, "instrument.maturity");
  const auto rate = column(mkt.rate, "market.rate");
  const auto q = column(mkt.dividend_yield, "market.dividend_yield");
  const auto vol = column(mdl.volatility, "model.volatility");
  const auto is_call = option_is_call_mask_(inst, ref, out_cfg, "price").reshape({n}).to(pluss::pCPU, out_cfg.dtype).host<double>();
  std::vector<double> px(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const auto k = static_cast<std::size_t>(i);
    px[k] = pricing::detail::spectral_price(is_call[k] >= 0.5 ? pricing::OptionType::Call : pricing::OptionType::Put, spot[k], strike[k], rate[k], q[k],
                                    maturity[k], vol[k], eng.options);
  }
  return PriceResult{.price = pluss::make_series(px, {n}, out_cfg).reshape(ref.sizes())};
}

PriceResult price_exercise_with_greeks(const Vanilla& inst, const model::Heston& mdl, const MarketState& mkt, const pricer::Cos& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise != pricing::ExerciseStyle::European, "subsp::math::finance::object::price_with_greeks: a European Heston option has the closed-form COS Greeks of the plain pricer");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = heston_params_from_series_(mdl, out_cfg);
  const auto dates = exercise_dates_(inst, out_cfg);
  return group_greeks_(inst, mkt, out_cfg, [&](double s, double k, double r, double q, double t, bool call) {
    return pricing::detail::heston_cos_exercise_greeks(call ? pricing::OptionType::Call : pricing::OptionType::Put, inst.exercise, s, k, r, q, t, params, dates, eng.options);
  });
}

PriceResult price_exercise_with_greeks(const Vanilla& inst, const model::Bates& mdl, const MarketState& mkt, const pricer::Cos& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise != pricing::ExerciseStyle::European, "subsp::math::finance::object::price_with_greeks: a European Bates option has the closed-form COS Greeks of the plain pricer");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = bates_params_from_series_(mdl, out_cfg);
  const auto dates = exercise_dates_(inst, out_cfg);
  return group_greeks_(inst, mkt, out_cfg, [&](double s, double k, double r, double q, double t, bool call) {
    return pricing::detail::bates_cos_exercise_greeks(call ? pricing::OptionType::Call : pricing::OptionType::Put, inst.exercise, s, k, r, q, t, params, dates, eng.options);
  });
}

PriceResult price_exercise_with_greeks(const Vanilla& inst, const model::BlackScholes& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise != pricing::ExerciseStyle::European, "subsp::math::finance::object::price_exercise_with_greeks: a European option has the shared-path Greeks of the plain pricer");
  return exercise_mc_greeks_(inst, mdl, mkt, eng, cfg);
}

PriceResult price_exercise_with_greeks(const Vanilla& inst, const model::Heston& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise != pricing::ExerciseStyle::European, "subsp::math::finance::object::price_exercise_with_greeks: a European option has the shared-path Greeks of the plain pricer");
  return exercise_mc_greeks_(inst, mdl, mkt, eng, cfg);
}

PriceResult price_exercise_with_greeks(const Vanilla& inst, const model::Bates& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise != pricing::ExerciseStyle::European, "subsp::math::finance::object::price_exercise_with_greeks: a European option has the shared-path Greeks of the plain pricer");
  return exercise_mc_greeks_(inst, mdl, mkt, eng, cfg);
}

PriceResult price_exercise_with_greeks(const Vanilla& inst, const model::Sabr& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise != pricing::ExerciseStyle::European, "subsp::math::finance::object::price_exercise_with_greeks: a European option has the shared-path Greeks of the plain pricer");
  return exercise_mc_greeks_(inst, mdl, mkt, eng, cfg);
}

PriceResult price_exercise_with_greeks(const Vanilla& inst, const model::RoughHeston& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise != pricing::ExerciseStyle::European, "subsp::math::finance::object::price_exercise_with_greeks: a European option has the shared-path Greeks of the plain pricer");
  return exercise_mc_greeks_(inst, mdl, mkt, eng, cfg);
}

PriceResult price_exercise_with_greeks(const Vanilla& inst, const model::RoughBergomi& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise != pricing::ExerciseStyle::European, "subsp::math::finance::object::price_exercise_with_greeks: a European option has the shared-path Greeks of the plain pricer");
  return exercise_mc_greeks_(inst, mdl, mkt, eng, cfg);
}

PriceResult price_with_greeks(const Vanilla& inst, const model::BlackScholes& mdl, const MarketState& mkt, const pricer::Cos& eng, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto dates = exercise_dates_(inst, out_cfg);
  const auto vol = as_series_vec_(mdl.volatility, "model.volatility").to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
  return group_greeks_(inst, mkt, out_cfg, [&](double s, double k, double r, double q, double t, bool call, std::size_t i) {
    return pricing::detail::black_scholes_cos_greeks(call ? pricing::OptionType::Call : pricing::OptionType::Put, inst.exercise, s, k, r, q, t, vol[vol.size() == 1 ? 0 : i], dates, eng.options);
  });
}

PriceResult price_with_greeks(const Vanilla& inst, const model::BlackScholes& mdl, const MarketState& mkt, const pricer::Spectral& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise != pricing::ExerciseStyle::Bermudan, "subsp::math::finance::object::price_with_greeks: the Spectral engine prices European and American exercise");
  if (inst.exercise == pricing::ExerciseStyle::European) return bs_price_analytic_(inst, mdl, mkt, true, cfg);   // a European option has the closed form
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto vol = as_series_vec_(mdl.volatility, "model.volatility").to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
  return group_greeks_(inst, mkt, out_cfg, [&](double s, double k, double r, double q, double t, bool call, std::size_t i) {
    return pricing::detail::spectral_greeks(call ? pricing::OptionType::Call : pricing::OptionType::Put, s, k, r, q, t, vol[vol.size() == 1 ? 0 : i], eng.options);
  });
}

PriceResult price_with_greeks(const Vanilla& inst, const model::BlackScholes& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise == pricing::ExerciseStyle::European, "subsp::math::finance::object::price_with_greeks: only European exercise has pathwise Monte Carlo Greeks");
  return bs_price_mc_(inst, mdl, mkt, eng, cfg, true);
}

PriceResult price(const Vanilla& inst,
                  const model::Heston& mdl,
                  const MarketState& mkt,
                  const pricer::Pde& eng,
                  Config cfg) {
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = heston_params_from_series_(mdl, out_cfg);
  const auto dates = exercise_dates_(inst, out_cfg);
  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"),
                                    as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"),
                                    as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"),
                                    pluss::series::full({1}, 0.0, out_cfg),
                                    out_cfg);
  const auto n = ref.numel();
  auto column = [&](const Series& x, const char* name) { return expand_like_(as_series_vec_(x, name), ref, out_cfg).reshape({n}).to(pluss::pCPU, out_cfg.dtype).host<double>(); };
  const auto spot = column(mkt.spot, "market.spot");
  const auto strike = column(inst.strike, "instrument.strike");
  const auto maturity = column(inst.maturity, "instrument.maturity");
  const auto rate = column(mkt.rate, "market.rate");
  const auto q = column(mkt.dividend_yield, "market.dividend_yield");
  const auto is_call = option_is_call_mask_(inst, ref, out_cfg, "price").reshape({n}).to(pluss::pCPU, out_cfg.dtype).host<double>();
  std::vector<double> px(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const auto k = static_cast<std::size_t>(i);
    px[k] = pricing::detail::heston_price_pde(is_call[k] >= 0.5 ? pricing::OptionType::Call : pricing::OptionType::Put, inst.exercise, spot[k], strike[k],
                                      rate[k], q[k], maturity[k], params, dates, eng.options);
  }
  return PriceResult{.price = pluss::make_series(px, {n}, out_cfg).reshape(ref.sizes())};
}

PriceResult price_with_greeks(const Vanilla& inst, const model::Heston& mdl, const MarketState& mkt, const pricer::Pde& eng, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = heston_params_from_series_(mdl, out_cfg);
  const auto dates = exercise_dates_(inst, out_cfg);
  return group_greeks_(inst, mkt, out_cfg, [&](double s, double k, double r, double q, double t, bool call) {
    return pricing::detail::heston_pde_greeks(call ? pricing::OptionType::Call : pricing::OptionType::Put, inst.exercise, s, k, r, q, t, params, dates, eng.options);
  });
}

PriceResult price_with_greeks(const Vanilla& inst, const model::BlackScholes& mdl, const MarketState& mkt, const pricer::Lattice& eng, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto dates = exercise_dates_(inst, out_cfg);
  const Series vol_series = as_series_vec_(mdl.volatility, "model.volatility");
  const auto vol = vol_series.to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
  return group_greeks_(inst, mkt, out_cfg, [&](double s, double k, double r, double q, double t, bool call, std::size_t i) {
    return pricing::detail::lattice_greeks(call ? pricing::OptionType::Call : pricing::OptionType::Put, inst.exercise, s, k, r, q, t, vol[vol.size() == 1 ? 0 : i], eng.steps, dates, out_cfg);
  });
}

PriceResult price(const Vanilla& inst,
                  const model::BlackScholes& mdl,
                  const MarketState& mkt,
                  const pricer::Cos& eng,
                  Config cfg) {
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto dates = exercise_dates_(inst, out_cfg);
  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"),
                                    as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"),
                                    as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"),
                                    as_series_vec_(mdl.volatility, "model.volatility"),
                                    out_cfg);
  const auto n = ref.numel();
  auto column = [&](const Series& x, const char* name) { return expand_like_(as_series_vec_(x, name), ref, out_cfg).reshape({n}).to(pluss::pCPU, out_cfg.dtype).host<double>(); };
  const auto spot = column(mkt.spot, "market.spot");
  const auto strike = column(inst.strike, "instrument.strike");
  const auto maturity = column(inst.maturity, "instrument.maturity");
  const auto rate = column(mkt.rate, "market.rate");
  const auto q = column(mkt.dividend_yield, "market.dividend_yield");
  const auto vol = column(mdl.volatility, "model.volatility");
  const auto is_call = option_is_call_mask_(inst, ref, out_cfg, "price").reshape({n}).to(pluss::pCPU, out_cfg.dtype).host<double>();
  std::vector<double> px(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const auto k = static_cast<std::size_t>(i);
    px[k] = pricing::detail::black_scholes_price_cos(is_call[k] >= 0.5 ? pricing::OptionType::Call : pricing::OptionType::Put, inst.exercise, spot[k], strike[k],
                                             rate[k], q[k], maturity[k], vol[k], dates, eng.options);
  }
  return PriceResult{.price = pluss::make_series(px, {n}, out_cfg).reshape(ref.sizes())};
}

PriceResult price(const Vanilla& inst,
                  const model::BlackScholes& mdl,
                  const MarketState& mkt,
                  const pricer::Lattice& eng,
                  Config cfg) {
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto dates = exercise_dates_(inst, out_cfg);
  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"),
                                    as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"),
                                    as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"),
                                    as_series_vec_(mdl.volatility, "model.volatility"),
                                    out_cfg);
  const auto n = ref.numel();
  auto column = [&](const Series& x, const char* name) { return expand_like_(as_series_vec_(x, name), ref, out_cfg).reshape({n}).to(pluss::pCPU, out_cfg.dtype).host<double>(); };
  const auto spot = column(mkt.spot, "market.spot");
  const auto strike = column(inst.strike, "instrument.strike");
  const auto maturity = column(inst.maturity, "instrument.maturity");
  const auto rate = column(mkt.rate, "market.rate");
  const auto q = column(mkt.dividend_yield, "market.dividend_yield");
  const auto vol = column(mdl.volatility, "model.volatility");
  const auto is_call = option_is_call_mask_(inst, ref, out_cfg, "price").reshape({n}).to(pluss::pCPU, out_cfg.dtype).host<double>();
  std::vector<double> px(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const auto k = static_cast<std::size_t>(i);
    px[k] = pricing::detail::lattice_price(is_call[k] >= 0.5 ? pricing::OptionType::Call : pricing::OptionType::Put, inst.exercise, spot[k],
                                   strike[k], rate[k], q[k], maturity[k], vol[k], eng.steps, dates, out_cfg);
  }
  return PriceResult{.price = pluss::make_series(px, {n}, out_cfg).reshape(ref.sizes())};
}

// Groups the options of a strip by (spot, rate, yield, maturity), calls `solve` once per group with the strikes and call flags, and scatters the results back.
template <typename Solve>
PriceResult group_strip_(const Vanilla& inst, const MarketState& mkt, bool greeks, Config out_cfg, Solve&& solve) {
  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"), as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"), as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"), pluss::series::full({1}, 0.0, out_cfg), out_cfg);
  const std::int64_t n = ref.numel();
  const auto host = [&](const Series& x, const char* name) {
    const auto h = expand_like_(as_series_vec_(x, name), ref, out_cfg).reshape({n}).to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
    return std::vector<double>(h.begin(), h.end());
  };
  const auto spot = host(mkt.spot, "market.spot"), strike = host(inst.strike, "instrument.strike"), maturity = host(inst.maturity, "instrument.maturity");
  const auto rate = host(mkt.rate, "market.rate"), yield = host(mkt.dividend_yield, "market.dividend_yield");
  const auto mask = host(inst.type == pricing::OptionType::Auto ? inst.is_call : pluss::series::full({1}, inst.type == pricing::OptionType::Call ? 1.0 : 0.0, out_cfg), "instrument.is_call");
  struct Key { double s, r, q, t; bool operator<(const Key& o) const { return std::tie(s, r, q, t) < std::tie(o.s, o.r, o.q, o.t); } };
  std::map<Key, std::vector<std::size_t>> groups;
  for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i) groups[Key{spot[i], rate[i], yield[i], maturity[i]}].push_back(i);
  std::vector<double> price(n), delta(n), gamma(n), vega(n), theta(n), rho(n);
  for (const auto& [key, members] : groups) {
    std::vector<double> strikes;
    std::vector<bool> calls;
    for (std::size_t i : members) { strikes.push_back(strike[i]); calls.push_back(mask[i] >= 0.5); }
    const auto r = solve(key.s, key.r, key.q, key.t, strikes, calls);
    for (std::size_t j = 0; j < members.size(); ++j) {
      const std::size_t i = members[j];
      price[i] = r.price[j]; delta[i] = r.delta[j]; gamma[i] = r.gamma[j]; vega[i] = r.vega[j]; theta[i] = r.theta[j]; rho[i] = r.rho[j];
    }
  }
  const auto shape = [&](const std::vector<double>& v) { return pluss::make_series(v, {n}, out_cfg).reshape(ref.sizes()); };
  PriceResult out{.price = shape(price)};
  if (greeks) { out.delta = shape(delta); out.gamma = shape(gamma); out.vega = shape(vega); out.theta = shape(theta); out.rho = shape(rho); }
  return out;
}

PriceResult fft_sv_strip_(const Vanilla& inst, const MarketState& mkt, const volatility::BatesParams& params, bool bates, const pricing::HestonFftOptions& options, bool greeks, Config out_cfg) {
  return group_strip_(inst, mkt, greeks, out_cfg, [&](double s, double r, double q, double t, const std::vector<double>& strikes, const std::vector<bool>& calls) {
    return pricing::detail::sv_fft_strip(params, bates, s, r, q, t, strikes, calls, options, greeks, out_cfg);
  });
}

volatility::BatesParams bates_from_heston_(const volatility::HestonParams& h) {
  return volatility::BatesParams{.kappa = h.kappa, .theta = h.theta, .sigma = h.sigma, .rho = h.rho, .v0 = h.v0, .mu = h.mu, .variance_floor = h.variance_floor, .jump_intensity = 0.0};
}

PriceResult price(const Vanilla& inst,
                  const model::Heston& mdl,
                  const MarketState& mkt,
                  const pricer::Fft& eng,
                  Config cfg) {
  require_european_(inst, "Fft");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = heston_params_from_series_(mdl, out_cfg);

  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"),
                                    as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"),
                                    as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"),
                                    pluss::series::full({1}, 0.0, out_cfg),
                                    out_cfg);
  const Series spot_v = expand_like_(as_series_vec_(mkt.spot, "market.spot"), ref, out_cfg).reshape({ref.numel()});
  const Series strike_v =
      expand_like_(as_series_vec_(inst.strike, "instrument.strike"), ref, out_cfg).reshape({ref.numel()});
  const Series maturity_v =
      expand_like_(as_series_vec_(inst.maturity, "instrument.maturity"), ref, out_cfg).reshape({ref.numel()});
  const Series rate_v = expand_like_(as_series_vec_(mkt.rate, "market.rate"), ref, out_cfg).reshape({ref.numel()});
  const Series q_v =
      expand_like_(as_series_vec_(mkt.dividend_yield, "market.dividend_yield"), ref, out_cfg).reshape({ref.numel()});
  const Series is_call_mask_v = option_is_call_mask_(inst, ref, out_cfg, "price").reshape({ref.numel()});

  if (eng.return_grid) {
    PLUSS_CHECK(ref.numel() == 1,
                "subsp::math::finance::object::price: heston FFT grid output requires scalar market/instrument input");
    const double spot = spot_v.reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.reshape({1}).select(0, 0).item<double>();
    const double q = q_v.reshape({1}).select(0, 0).item<double>();
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const auto grid = pricing::detail::heston_price_fft_grid(type, spot, rate, q, maturity, params, eng.options, out_cfg);
    return PriceResult{
        .price = grid.price,
        .strike_grid = grid.strike,
    };
  }

  if (eng.options.mode == pricing::HestonCfMode::LittleTrap) {
    // one transform per (spot, rate, yield, maturity) serves every strike of the strip
    return fft_sv_strip_(inst, mkt, bates_from_heston_(params), false, eng.options, false, out_cfg);
  }

  if (ref.numel() == 1) {
    const double spot = spot_v.reshape({1}).select(0, 0).item<double>();
    const double strike = strike_v.reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.reshape({1}).select(0, 0).item<double>();
    const double q = q_v.reshape({1}).select(0, 0).item<double>();
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const double px = pricing::detail::heston_price_fft(type, spot, strike, rate, q, maturity, params, eng.options, out_cfg);
    return PriceResult{
        .price = pluss::series::full({1}, px, out_cfg),
    };
  }

  std::vector<double> px(static_cast<std::size_t>(ref.numel()));
  for (std::int64_t i = 0; i < ref.numel(); ++i) {
    const double spot = spot_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double strike = strike_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double q = q_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const auto type = option_type_from_mask_(is_call_mask_v, i);
    px[static_cast<std::size_t>(i)] =
        pricing::detail::heston_price_fft(type, spot, strike, rate, q, maturity, params, eng.options, out_cfg);
  }
  return PriceResult{
      .price = pluss::make_series(px, {ref.numel()}, out_cfg).reshape(ref.sizes()),
  };
}

PriceResult price(const Vanilla& inst,
                  const model::Bates& mdl,
                  const MarketState& mkt,
                  const pricer::Fft& eng,
                  Config cfg) {
  require_european_(inst, "Fft");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = bates_params_from_series_(mdl, out_cfg);

  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"),
                                    as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"),
                                    as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"),
                                    pluss::series::full({1}, 0.0, out_cfg),
                                    out_cfg);
  const Series spot_v = expand_like_(as_series_vec_(mkt.spot, "market.spot"), ref, out_cfg).reshape({ref.numel()});
  const Series strike_v =
      expand_like_(as_series_vec_(inst.strike, "instrument.strike"), ref, out_cfg).reshape({ref.numel()});
  const Series maturity_v =
      expand_like_(as_series_vec_(inst.maturity, "instrument.maturity"), ref, out_cfg).reshape({ref.numel()});
  const Series rate_v = expand_like_(as_series_vec_(mkt.rate, "market.rate"), ref, out_cfg).reshape({ref.numel()});
  const Series q_v =
      expand_like_(as_series_vec_(mkt.dividend_yield, "market.dividend_yield"), ref, out_cfg).reshape({ref.numel()});
  const Series is_call_mask_v = option_is_call_mask_(inst, ref, out_cfg, "price").reshape({ref.numel()});

  if (eng.return_grid) {
    PLUSS_CHECK(ref.numel() == 1,
                "subsp::math::finance::object::price: bates FFT grid output requires scalar market/instrument input");
    const double spot = spot_v.reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.reshape({1}).select(0, 0).item<double>();
    const double q = q_v.reshape({1}).select(0, 0).item<double>();
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const auto grid = pricing::detail::bates_price_fft_grid(type, spot, rate, q, maturity, params, eng.options, out_cfg);
    return PriceResult{
        .price = grid.price,
        .strike_grid = grid.strike,
    };
  }

  if (eng.options.mode == pricing::HestonCfMode::LittleTrap) {
    return fft_sv_strip_(inst, mkt, params, true, eng.options, false, out_cfg);
  }

  if (ref.numel() == 1) {
    const double spot = spot_v.reshape({1}).select(0, 0).item<double>();
    const double strike = strike_v.reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.reshape({1}).select(0, 0).item<double>();
    const double q = q_v.reshape({1}).select(0, 0).item<double>();
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const double px = pricing::detail::bates_price_fft(type, spot, strike, rate, q, maturity, params, eng.options, out_cfg);
    return PriceResult{
        .price = pluss::series::full({1}, px, out_cfg),
    };
  }

  std::vector<double> px(static_cast<std::size_t>(ref.numel()));
  for (std::int64_t i = 0; i < ref.numel(); ++i) {
    const double spot = spot_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double strike = strike_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double q = q_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const auto type = option_type_from_mask_(is_call_mask_v, i);
    px[static_cast<std::size_t>(i)] =
        pricing::detail::bates_price_fft(type, spot, strike, rate, q, maturity, params, eng.options, out_cfg);
  }
  return PriceResult{
      .price = pluss::make_series(px, {ref.numel()}, out_cfg).reshape(ref.sizes()),
  };
}

PriceResult price_with_greeks(const Vanilla& inst, const model::Heston& mdl, const MarketState& mkt, const pricer::Fft& eng, Config cfg) {
  require_european_(inst, "Fft");
  PLUSS_CHECK(eng.options.mode == pricing::HestonCfMode::LittleTrap && !eng.return_grid,
              "subsp::math::finance::object::price_with_greeks: the FFT Greeks need the LittleTrap form and no grid output");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  return fft_sv_strip_(inst, mkt, bates_from_heston_(heston_params_from_series_(mdl, out_cfg)), false, eng.options, true, out_cfg);
}

PriceResult price_with_greeks(const Vanilla& inst, const model::Bates& mdl, const MarketState& mkt, const pricer::Fft& eng, Config cfg) {
  require_european_(inst, "Fft");
  PLUSS_CHECK(eng.options.mode == pricing::HestonCfMode::LittleTrap && !eng.return_grid,
              "subsp::math::finance::object::price_with_greeks: the FFT Greeks need the LittleTrap form and no grid output");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  return fft_sv_strip_(inst, mkt, bates_params_from_series_(mdl, out_cfg), true, eng.options, true, out_cfg);
}

// COS prices of a whole strip in one vectorised call. Every market and model parameter is a Series with one entry (shared) or one entry per option, so
// a strip may mix spots, rates and even model parameters; calls and puts of a mixed strip are priced in two calls.
struct CosStripInputs_ {
  Series ref, spot, strike, maturity, rate, yield, mask;
  pricing::detail::SvParamsSeries p;
  std::int64_t n = 0;
};

CosStripInputs_ cos_strip_inputs_(const Vanilla& inst,
                       const MarketState& mkt,
                       const pricing::detail::SvParamsSeries& params,
                       bool bates,
                       Config out_cfg) {
  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"), as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"), as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"), pluss::series::full({1}, 0.0, out_cfg), out_cfg);
  const std::int64_t n = ref.numel();
  const auto flat = [&](const Series& x, const char* name) {
    const Series v = as_series_vec_(x, name);
    PLUSS_CHECK(v.numel() == 1 || v.numel() == n,
                std::string("subsp::math::finance::object::price: ") + name + " must have one element or one per option");
    return expand_like_(v, ref, out_cfg).reshape({n});
  };
  const Series spot = flat(mkt.spot, "market.spot"), strike = flat(inst.strike, "instrument.strike"), maturity = flat(inst.maturity, "instrument.maturity");
  const Series rate = flat(mkt.rate, "market.rate"), yield = flat(mkt.dividend_yield, "market.dividend_yield");
  pricing::detail::SvParamsSeries p;
  p.kappa = flat(params.kappa, "model.kappa");
  p.theta = flat(params.theta, "model.theta");
  p.sigma = flat(params.sigma, "model.sigma");
  p.rho = flat(params.rho, "model.rho");
  p.v0 = flat(params.v0, "model.v0");
  if (bates) {
    p.jump_intensity = flat(params.jump_intensity, "model.jump_intensity");
    p.jump_mean = flat(params.jump_mean, "model.jump_mean");
    p.jump_vol = flat(params.jump_vol, "model.jump_vol");
  }
  const Series mask = option_is_call_mask_(inst, ref, out_cfg, "price").reshape({n});
  return CosStripInputs_{ref, spot, strike, maturity, rate, yield, mask, p, n};
}

PriceResult cos_strip_greeks_(const Vanilla& inst,
                              const MarketState& mkt,
                              const pricing::detail::SvParamsSeries& params,
                              bool bates,
                              const pricing::CosOptions& options,
                              Config out_cfg) {
  const CosStripInputs_ in = cos_strip_inputs_(inst, mkt, params, bates, out_cfg);
  const auto g = pricing::detail::sv_cos_greeks_v(in.mask, in.spot, in.strike, in.rate, in.yield, in.maturity, in.p, options, out_cfg);
  const auto shape = [&](const Series& x) { return x.reshape(in.ref.sizes()); };
  return PriceResult{.price = shape(g.price), .delta = shape(g.delta), .gamma = shape(g.gamma), .vega = shape(g.vega), .theta = shape(g.theta), .rho = shape(g.rho)};
}

PriceResult cos_strip_(const Vanilla& inst,
                       const MarketState& mkt,
                       const pricing::detail::SvParamsSeries& params,
                       bool bates,
                       const pricing::CosOptions& options,
                       Config out_cfg) {
  const CosStripInputs_ in = cos_strip_inputs_(inst, mkt, params, bates, out_cfg);
  const Series ref = in.ref, spot = in.spot, strike = in.strike, maturity = in.maturity, rate = in.rate, yield = in.yield, mask = in.mask;
  const pricing::detail::SvParamsSeries& p = in.p;
  const std::int64_t n = in.n;
  const auto price_with = [&](pricing::OptionType type, const Series& s, const Series& k, const Series& r, const Series& q, const Series& t,
                              const pricing::detail::SvParamsSeries& pp) {
    return bates ? pricing::detail::bates_price_cos_v(type, s, k, r, q, t, pp, options, out_cfg)
                 : pricing::detail::heston_price_cos_v(type, s, k, r, q, t, pp, options, out_cfg);
  };
  const std::vector<double> flags = mask.to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
  std::vector<std::int64_t> calls, puts;
  for (std::int64_t i = 0; i < n; ++i) (flags[static_cast<std::size_t>(i)] >= 0.5 ? calls : puts).push_back(i);
  if (puts.empty() || calls.empty()) {
    const Series px = price_with(puts.empty() ? pricing::OptionType::Call : pricing::OptionType::Put, spot, strike, rate, yield, maturity, p);
    return PriceResult{.price = px.reshape(ref.sizes())};
  }
  std::vector<double> out(static_cast<std::size_t>(n));
  const Config icfg = Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(out_cfg.platform);
  for (const auto& [type, idx] : {std::pair<pricing::OptionType, const std::vector<std::int64_t>&>{pricing::OptionType::Call, calls}, {pricing::OptionType::Put, puts}}) {
    const Series at = pluss::make_series(idx, {static_cast<std::int64_t>(idx.size())}, icfg);
    const auto pick = [&](const Series& x) { return x.index_select(0, at); };
    pricing::detail::SvParamsSeries sub;
    sub.kappa = pick(p.kappa); sub.theta = pick(p.theta); sub.sigma = pick(p.sigma); sub.rho = pick(p.rho); sub.v0 = pick(p.v0);
    if (bates) { sub.jump_intensity = pick(p.jump_intensity); sub.jump_mean = pick(p.jump_mean); sub.jump_vol = pick(p.jump_vol); }
    const auto v = price_with(type, pick(spot), pick(strike), pick(rate), pick(yield), pick(maturity), sub).to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
    for (std::size_t j = 0; j < idx.size(); ++j) out[static_cast<std::size_t>(idx[j])] = v[j];
  }
  return PriceResult{.price = pluss::make_series(out, {n}, out_cfg).reshape(ref.sizes())};
}

PriceResult price(const Vanilla& inst,
                  const model::Heston& mdl,
                  const MarketState& mkt,
                  const pricer::Cos& eng,
                  Config cfg) {
  if (inst.exercise != pricing::ExerciseStyle::European) {
    const auto exercise_params = heston_params_from_series_(mdl, f64_cfg_(cfg, mkt.spot.platform()));
    return cos_exercise_(inst, mdl, mkt, cfg, [&](pricing::OptionType type, double spot, double strike, double rate, double q, double maturity, const std::vector<double>& dates) {
      return pricing::detail::heston_price_cos_exercise(type, inst.exercise, spot, strike, rate, q, maturity, exercise_params, dates, eng.options);
    });
  }
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  return cos_strip_(inst, mkt, pricing::detail::SvParamsSeries{.kappa = mdl.kappa, .theta = mdl.theta, .sigma = mdl.sigma, .rho = mdl.rho, .v0 = mdl.v0}, false,
                    eng.options, out_cfg);
}

PriceResult price_with_greeks(const Vanilla& inst, const model::Heston& mdl, const MarketState& mkt, const pricer::Cos& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise == pricing::ExerciseStyle::European, "subsp::math::finance::object::price_with_greeks: only European exercise has closed-form COS Greeks");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  return cos_strip_greeks_(inst, mkt, pricing::detail::SvParamsSeries{.kappa = mdl.kappa, .theta = mdl.theta, .sigma = mdl.sigma, .rho = mdl.rho, .v0 = mdl.v0}, false,
                           eng.options, out_cfg);
}

PriceResult price_with_greeks(const Vanilla& inst, const model::Bates& mdl, const MarketState& mkt, const pricer::Cos& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise == pricing::ExerciseStyle::European, "subsp::math::finance::object::price_with_greeks: only European exercise has closed-form COS Greeks");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto with_default = [&](const Series& x, double fallback) { return x.defined() ? x : pluss::series::full({1}, fallback, out_cfg); };
  return cos_strip_greeks_(inst, mkt,
                           pricing::detail::SvParamsSeries{.kappa = mdl.kappa, .theta = mdl.theta, .sigma = mdl.sigma, .rho = mdl.rho, .v0 = mdl.v0,
                                                           .jump_intensity = with_default(mdl.jump_intensity, 0.15), .jump_mean = with_default(mdl.jump_mean, -0.08),
                                                           .jump_vol = with_default(mdl.jump_vol, 0.20)},
                           true, eng.options, out_cfg);
}

PriceResult price(const Vanilla& inst,
                  const model::Bates& mdl,
                  const MarketState& mkt,
                  const pricer::Cos& eng,
                  Config cfg) {
  if (inst.exercise != pricing::ExerciseStyle::European) {
    const auto exercise_params = bates_params_from_series_(mdl, f64_cfg_(cfg, mkt.spot.platform()));
    return cos_exercise_(inst, mdl, mkt, cfg, [&](pricing::OptionType type, double spot, double strike, double rate, double q, double maturity, const std::vector<double>& dates) {
      return pricing::detail::bates_price_cos_exercise(type, inst.exercise, spot, strike, rate, q, maturity, exercise_params, dates, eng.options);
    });
  }
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto with_default = [&](const Series& x, double fallback) { return x.defined() ? x : pluss::series::full({1}, fallback, out_cfg); };
  return cos_strip_(inst, mkt,
                    pricing::detail::SvParamsSeries{.kappa = mdl.kappa, .theta = mdl.theta, .sigma = mdl.sigma, .rho = mdl.rho, .v0 = mdl.v0,
                                                    .jump_intensity = with_default(mdl.jump_intensity, 0.15), .jump_mean = with_default(mdl.jump_mean, -0.08),
                                                    .jump_vol = with_default(mdl.jump_vol, 0.20)},
                    true, eng.options, out_cfg);
}

// Monte Carlo strip of Heston / Bates options: options on the same (spot, rate, yield, maturity) share one simulation, so a strip of strikes costs one
// simulation instead of one per strike, and with `greeks` the Greeks come from the shared paths (see pricing::detail::sv_mc_strip).
PriceResult mc_sv_strip_(const Vanilla& inst,
                         const MarketState& mkt,
                         const volatility::BatesParams& params,
                         bool bates,
                         const pricing::MonteCarloOptions& options,
                         bool greeks,
                         Config out_cfg) {
  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"), as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"), as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"), pluss::series::full({1}, 0.0, out_cfg), out_cfg);
  const std::int64_t n = ref.numel();
  const auto host = [&](const Series& x, const char* name) {
    const auto h = expand_like_(as_series_vec_(x, name), ref, out_cfg).reshape({n}).to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
    return std::vector<double>(h.begin(), h.end());
  };
  const auto spot = host(mkt.spot, "market.spot"), strike = host(inst.strike, "instrument.strike"), maturity = host(inst.maturity, "instrument.maturity");
  const auto rate = host(mkt.rate, "market.rate"), yield = host(mkt.dividend_yield, "market.dividend_yield");
  const auto mask = host(inst.type == pricing::OptionType::Auto ? inst.is_call : pluss::series::full({1}, inst.type == pricing::OptionType::Call ? 1.0 : 0.0, out_cfg), "instrument.is_call");
  struct Key { double s, r, q, t; bool operator<(const Key& o) const { return std::tie(s, r, q, t) < std::tie(o.s, o.r, o.q, o.t); } };
  std::map<Key, std::vector<std::size_t>> groups;
  for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i) groups[Key{spot[i], rate[i], yield[i], maturity[i]}].push_back(i);
  std::vector<double> price(n), delta(n), gamma(n), vega(n), theta(n), rho(n);
  for (const auto& [key, members] : groups) {
    std::vector<double> strikes;
    std::vector<bool> calls;
    for (std::size_t i : members) { strikes.push_back(strike[i]); calls.push_back(mask[i] >= 0.5); }
    const auto r = pricing::detail::sv_mc_strip(params, bates, key.s, key.r, key.q, key.t, strikes, calls, options, greeks, out_cfg);
    for (std::size_t j = 0; j < members.size(); ++j) {
      const std::size_t i = members[j];
      price[i] = r.price[j]; delta[i] = r.delta[j]; gamma[i] = r.gamma[j]; vega[i] = r.vega[j]; theta[i] = r.theta[j]; rho[i] = r.rho[j];
    }
  }
  const auto shape = [&](const std::vector<double>& v) { return pluss::make_series(v, {n}, out_cfg).reshape(ref.sizes()); };
  PriceResult out{.price = shape(price)};
  if (greeks) { out.delta = shape(delta); out.gamma = shape(gamma); out.vega = shape(vega); out.theta = shape(theta); out.rho = shape(rho); }
  return out;
}


PriceResult price(const Vanilla& inst,
                  const model::Heston& mdl,
                  const MarketState& mkt,
                  const pricer::MonteCarlo& eng,
                  Config cfg) {
  if (inst.exercise != pricing::ExerciseStyle::European) return exercise_mc_(inst, mdl, mkt, eng, cfg);
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  return mc_sv_strip_(inst, mkt, bates_from_heston_(heston_params_from_series_(mdl, out_cfg)), false, eng.options, false, out_cfg);
}

PriceResult price(const Vanilla& inst,
                  const model::Bates& mdl,
                  const MarketState& mkt,
                  const pricer::MonteCarlo& eng,
                  Config cfg) {
  if (inst.exercise != pricing::ExerciseStyle::European) return exercise_mc_(inst, mdl, mkt, eng, cfg);
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  return mc_sv_strip_(inst, mkt, bates_params_from_series_(mdl, out_cfg), true, eng.options, false, out_cfg);
}

PriceResult price_with_greeks(const Vanilla& inst, const model::Heston& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise == pricing::ExerciseStyle::European, "subsp::math::finance::object::price_with_greeks: only European exercise has shared-path Monte Carlo Greeks");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  return mc_sv_strip_(inst, mkt, bates_from_heston_(heston_params_from_series_(mdl, out_cfg)), false, eng.options, true, out_cfg);
}

PriceResult price_with_greeks(const Vanilla& inst, const model::Bates& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise == pricing::ExerciseStyle::European, "subsp::math::finance::object::price_with_greeks: only European exercise has shared-path Monte Carlo Greeks");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  return mc_sv_strip_(inst, mkt, bates_params_from_series_(mdl, out_cfg), true, eng.options, true, out_cfg);
}

PriceResult price_with_greeks(const Vanilla& inst, const model::Sabr& mdl, const MarketState& mkt, const pricer::Analytic&, Config cfg) {
  require_european_(inst, "Analytic");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = sabr_params_from_series_(mdl, out_cfg);
  return group_greeks_(inst, mkt, out_cfg, [&](double s, double k, double r, double q, double t, bool call) {
    return pricing::detail::sabr_greeks(call ? pricing::OptionType::Call : pricing::OptionType::Put, s, k, r, q, t, params, mdl.vol_type, mdl.approx);
  });
}

PriceResult price(const Vanilla& inst,
                  const model::Sabr& mdl,
                  const MarketState& mkt,
                  const pricer::Analytic&,
                  Config cfg) {
  require_european_(inst, "Analytic");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = sabr_params_from_series_(mdl, out_cfg);

  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"),
                                    as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"),
                                    as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"),
                                    pluss::series::full({1}, 0.0, out_cfg),
                                    out_cfg);
  const Series spot_v = expand_like_(as_series_vec_(mkt.spot, "market.spot"), ref, out_cfg).reshape({ref.numel()});
  const Series strike_v =
      expand_like_(as_series_vec_(inst.strike, "instrument.strike"), ref, out_cfg).reshape({ref.numel()});
  const Series maturity_v =
      expand_like_(as_series_vec_(inst.maturity, "instrument.maturity"), ref, out_cfg).reshape({ref.numel()});
  const Series rate_v = expand_like_(as_series_vec_(mkt.rate, "market.rate"), ref, out_cfg).reshape({ref.numel()});
  const Series q_v =
      expand_like_(as_series_vec_(mkt.dividend_yield, "market.dividend_yield"), ref, out_cfg).reshape({ref.numel()});
  const Series is_call_mask_v = option_is_call_mask_(inst, ref, out_cfg, "price").reshape({ref.numel()});

  if (ref.numel() == 1) {
    const double spot = spot_v.reshape({1}).select(0, 0).item<double>();
    const double strike = strike_v.reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.reshape({1}).select(0, 0).item<double>();
    const double q = q_v.reshape({1}).select(0, 0).item<double>();
    const double forward = spot * std::exp((rate - q) * maturity);
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const double px = pricing::detail::sabr_price_approx(type, forward, strike, rate, maturity, params, mdl.vol_type, mdl.approx);
    return PriceResult{
        .price = pluss::series::full({1}, px, out_cfg),
    };
  }

  std::vector<double> px(static_cast<std::size_t>(ref.numel()));
  for (std::int64_t i = 0; i < ref.numel(); ++i) {
    const double spot = spot_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double strike = strike_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double q = q_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double forward = spot * std::exp((rate - q) * maturity);
    const auto type = option_type_from_mask_(is_call_mask_v, i);
    px[static_cast<std::size_t>(i)] =
        pricing::detail::sabr_price_approx(type, forward, strike, rate, maturity, params, mdl.vol_type, mdl.approx);
  }
  return PriceResult{
      .price = pluss::make_series(px, {ref.numel()}, out_cfg).reshape(ref.sizes()),
  };
}

PriceResult price(const Vanilla& inst,
                  const model::Sabr& mdl,
                  const MarketState& mkt,
                  const pricer::MonteCarlo& eng,
                  Config cfg) {
  if (inst.exercise != pricing::ExerciseStyle::European) return exercise_mc_(inst, mdl, mkt, eng, cfg);
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = sabr_params_from_series_(mdl, out_cfg);

  const Series ref = broadcast_ref_(as_series_vec_(mkt.spot, "market.spot"),
                                    as_series_vec_(inst.strike, "instrument.strike"),
                                    as_series_vec_(inst.maturity, "instrument.maturity"),
                                    as_series_vec_(mkt.rate, "market.rate"),
                                    as_series_vec_(mkt.dividend_yield, "market.dividend_yield"),
                                    pluss::series::full({1}, 0.0, out_cfg),
                                    out_cfg);
  const Series spot_v = expand_like_(as_series_vec_(mkt.spot, "market.spot"), ref, out_cfg).reshape({ref.numel()});
  const Series strike_v =
      expand_like_(as_series_vec_(inst.strike, "instrument.strike"), ref, out_cfg).reshape({ref.numel()});
  const Series maturity_v =
      expand_like_(as_series_vec_(inst.maturity, "instrument.maturity"), ref, out_cfg).reshape({ref.numel()});
  const Series rate_v = expand_like_(as_series_vec_(mkt.rate, "market.rate"), ref, out_cfg).reshape({ref.numel()});
  const Series q_v =
      expand_like_(as_series_vec_(mkt.dividend_yield, "market.dividend_yield"), ref, out_cfg).reshape({ref.numel()});
  const Series is_call_mask_v = option_is_call_mask_(inst, ref, out_cfg, "price").reshape({ref.numel()});

  if (ref.numel() == 1) {
    const double spot = spot_v.reshape({1}).select(0, 0).item<double>();
    const double strike = strike_v.reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.reshape({1}).select(0, 0).item<double>();
    const double q = q_v.reshape({1}).select(0, 0).item<double>();
    const double forward = spot * std::exp((rate - q) * maturity);
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const double px = pricing::detail::sabr_price_mc(type, forward, strike, rate, maturity, params, eng.options, out_cfg);
    return PriceResult{
        .price = pluss::series::full({1}, px, out_cfg),
    };
  }

  std::vector<double> px(static_cast<std::size_t>(ref.numel()));
  for (std::int64_t i = 0; i < ref.numel(); ++i) {
    const double spot = spot_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double strike = strike_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double q = q_v.select(0, i).reshape({1}).select(0, 0).item<double>();
    const double forward = spot * std::exp((rate - q) * maturity);
    const auto type = option_type_from_mask_(is_call_mask_v, i);
    px[static_cast<std::size_t>(i)] = pricing::detail::sabr_price_mc(type, forward, strike, rate, maturity, params, eng.options, out_cfg);
  }
  return PriceResult{
      .price = pluss::make_series(px, {ref.numel()}, out_cfg).reshape(ref.sizes()),
  };
}

PriceResult price(const Vanilla& inst,
                  const model::RoughHeston& mdl,
                  const MarketState& mkt,
                  const pricer::MonteCarlo& eng,
                  Config cfg) {
  if (inst.exercise != pricing::ExerciseStyle::European) return exercise_mc_(inst, mdl, mkt, eng, cfg);
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = rough_heston_params_from_series_(mdl, out_cfg);
  return group_strip_(inst, mkt, false, out_cfg, [&](double s, double r, double q, double t, const std::vector<double>& strikes, const std::vector<bool>& calls) {
    return pricing::detail::rough_heston_mc_strip(params, s, r, q, t, strikes, calls, eng.options, false, out_cfg);
  });
}

PriceResult price_with_greeks(const Vanilla& inst, const model::Sabr& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise == pricing::ExerciseStyle::European, "subsp::math::finance::object::price_with_greeks: the SABR Monte Carlo Greeks are for European exercise");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = sabr_params_from_series_(mdl, out_cfg);
  return group_greeks_(inst, mkt, out_cfg, [&](double s, double k, double r, double q, double t, bool call) {
    return pricing::detail::sabr_mc_greeks(call ? pricing::OptionType::Call : pricing::OptionType::Put, s, k, r, q, t, params, eng.options, out_cfg);
  });
}

PriceResult price_with_greeks(const Vanilla& inst, const model::RoughHeston& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise == pricing::ExerciseStyle::European, "subsp::math::finance::object::price_with_greeks: only European exercise has shared-path Monte Carlo Greeks");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = rough_heston_params_from_series_(mdl, out_cfg);
  return group_strip_(inst, mkt, true, out_cfg, [&](double s, double r, double q, double t, const std::vector<double>& strikes, const std::vector<bool>& calls) {
    return pricing::detail::rough_heston_mc_strip(params, s, r, q, t, strikes, calls, eng.options, true, out_cfg);
  });
}

PriceResult price(const Vanilla& inst,
                  const model::RoughBergomi& mdl,
                  const MarketState& mkt,
                  const pricer::MonteCarlo& eng,
                  Config cfg) {
  if (inst.exercise != pricing::ExerciseStyle::European) return exercise_mc_(inst, mdl, mkt, eng, cfg);
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = rough_bergomi_params_from_series_(mdl, out_cfg);
  return group_strip_(inst, mkt, false, out_cfg, [&](double s, double r, double q, double t, const std::vector<double>& strikes, const std::vector<bool>& calls) {
    return pricing::detail::rough_bergomi_mc_strip(params, s, r, q, t, strikes, calls, eng.options, false, out_cfg);
  });
}

PriceResult price_with_greeks(const Vanilla& inst, const model::RoughBergomi& mdl, const MarketState& mkt, const pricer::MonteCarlo& eng, Config cfg) {
  PLUSS_CHECK(inst.exercise == pricing::ExerciseStyle::European, "subsp::math::finance::object::price_with_greeks: only European exercise has shared-path Monte Carlo Greeks");
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = rough_bergomi_params_from_series_(mdl, out_cfg);
  return group_strip_(inst, mkt, true, out_cfg, [&](double s, double r, double q, double t, const std::vector<double>& strikes, const std::vector<bool>& calls) {
    return pricing::detail::rough_bergomi_mc_strip(params, s, r, q, t, strikes, calls, eng.options, true, out_cfg);
  });
}

} // namespace pluss::subsp::math::finance::object

namespace pluss::subsp::math::finance::pricer {
namespace {

using pluss::codem::Config;
using pluss::codem::Series;

// Every engine prices only inputs that agree: instrument, market and model are checked on each call, whether or not a Field is involved.
template <typename Model>
void validate_inputs_(const instrument::Vanilla& inst, const market::State& mkt, const Model& mdl) {
  namespace v = internal::validation;
  auto r = v::check(inst);
  r.merge(v::check(mkt));
  r.merge(v::check(module::Model{mdl}));
  r.raise("subsp::math::finance::pricer");
}

pricing::MonteCarloOptions to_mc_options_(const MonteCarlo& mc) {
  return pricing::MonteCarloOptions{
      .paths = mc.paths,
      .steps = mc.steps,
      .scheme = mc.scheme,
      .rand_method = mc.rand_method,
      .randomized_quasi = mc.randomized_quasi,
      .sobol_scrambling = mc.sobol_scrambling,
      .antithetic = mc.antithetic,
      .control_variate = mc.control_variate,
      .importance_sampling = mc.importance_sampling,
      .importance_shift = mc.importance_shift,
      .control_vol = mc.control_vol,
      .offset = mc.offset,
      .seed = mc.seed,
      .basis_degree = mc.basis_degree,
  };
}

pricing::CosOptions to_cos_options_(const COS& cos) {
  return pricing::CosOptions{
      .num_terms = cos.num_terms,
      .truncation = cos.truncation,
      .cumulant_step = cos.cumulant_step,
      .exercise_truncation = cos.exercise_truncation,
      .exercise_terms = cos.exercise_terms,
      .extrapolation = cos.extrapolation,
      .variance_nodes = cos.variance_nodes,
      .variance_tolerance = cos.variance_tolerance,
  };
}

Result to_result_(const object::PriceResult& out) {
  return Result{
      .price = out.price,
      .greeks =
          Greeks{
              .delta = out.delta,
              .gamma = out.gamma,
              .vega = out.vega,
              .theta = out.theta,
              .rho = out.rho,
          },
      .strike_grid = out.strike_grid,
  };
}

// The volatility-level parameter that vega bumps: the initial variance for the Heston family, the initial volatility for SABR and Black-Scholes, the
// forward-variance level for rough Bergomi.
Series &vol_state_(model::Heston &m) { return m.v0; }
Series &vol_state_(model::Bates &m) { return m.v0; }
Series &vol_state_(model::RoughHeston &m) { return m.v0; }
Series &vol_state_(model::RoughBergomi &m) { return m.xi; }
Series &vol_state_(model::Sabr &m) { return m.alpha0; }
Series &vol_state_(model::BlackScholes &m) { return m.volatility; }

// Early-exercise values are piecewise linear in the spot on a fixed lattice or path set, so delta and gamma need a bump of about a node spacing.
constexpr double kExerciseSpotStep = 0.02;

template <typename Model, typename PriceFn>
Greeks numeric_transform_greeks_(const instrument::Vanilla& inst,
                                 const Model& mdl,
                                 const market::State& mkt,
                                 const Series& base_price,
                                 const PriceFn& price_fn,
                                 double spot_rel = 1e-4) {
  const double kSpotRel = spot_rel;
  constexpr double kSpotAbs = 1e-6;
  constexpr double kRateAbs = 1e-4;
  constexpr double kTauRel = 1e-4;
  constexpr double kTauAbs = 1e-5;
  const double kVRel = spot_rel;
  constexpr double kVAbs = 1e-6;
  constexpr double kFloor = 1e-12;

  const Series spot_step = mkt.spot.abs().mul(kSpotRel).clamp_min(kSpotAbs);
  market::State mkt_su = mkt;
  market::State mkt_sd = mkt;
  mkt_su.spot = mkt.spot.add(spot_step);
  mkt_sd.spot = mkt.spot.sub(spot_step).clamp_min(kFloor);
  const Series p_su = price_fn(inst, mdl, mkt_su);
  const Series p_sd = price_fn(inst, mdl, mkt_sd);
  const Series dspot = mkt_su.spot.sub(mkt_sd.spot).clamp_min(kFloor);

  const Series delta = p_su.sub(p_sd).div(dspot);
  const Series gamma = p_su.sub(base_price.mul(2.0)).add(p_sd).mul(4.0).div(dspot.square().clamp_min(kFloor));

  const Series rate_step = mkt.rate.mul(0.0).add(kRateAbs);
  market::State mkt_ru = mkt;
  market::State mkt_rd = mkt;
  mkt_ru.rate = mkt.rate.add(rate_step);
  mkt_rd.rate = mkt.rate.sub(rate_step);
  const Series p_ru = price_fn(inst, mdl, mkt_ru);
  const Series p_rd = price_fn(inst, mdl, mkt_rd);
  const Series rho = p_ru.sub(p_rd).div(mkt_ru.rate.sub(mkt_rd.rate).clamp_min(kFloor));

  const Series tau_step = inst.maturity.abs().mul(kTauRel).clamp_min(kTauAbs);
  instrument::Vanilla inst_tu = inst;
  instrument::Vanilla inst_td = inst;
  inst_tu.maturity = inst.maturity.add(tau_step);
  inst_td.maturity = inst.maturity.sub(tau_step).clamp_min(kFloor);
  const Series p_tu = price_fn(inst_tu, mdl, mkt);
  const Series p_td = price_fn(inst_td, mdl, mkt);
  const Series theta = p_td.sub(p_tu).div(inst_tu.maturity.sub(inst_td.maturity).clamp_min(kFloor));

  Model mdl_vu = mdl;
  Model mdl_vd = mdl;
  Series &level_u = vol_state_(mdl_vu);
  Series &level_d = vol_state_(mdl_vd);
  const Series level = level_u;
  const Series v_step = level.abs().mul(kVRel).clamp_min(kVAbs);
  level_u = level.add(v_step);
  level_d = level.sub(v_step).clamp_min(kFloor);
  const Series p_vu = price_fn(inst, mdl_vu, mkt);
  const Series p_vd = price_fn(inst, mdl_vd, mkt);
  const Series vega = p_vu.sub(p_vd).div(level_u.sub(level_d).clamp_min(kFloor));

  return Greeks{
      .delta = delta,
      .gamma = gamma,
      .vega = vega,
      .theta = theta,
      .rho = rho,
  };
}

} // namespace

Result Analytic::price(const instrument::Vanilla& inst,
                       const model::BlackScholes& mdl,
                       const market::State& mkt,
                       RiskSpec risk,
                       Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const bool ask_greeks = with_greeks && (risk == RiskSpec::All);
  const auto out = object::price(inst, mdl, mkt, object::pricer::Analytic{.with_greeks = ask_greeks}, cfg);
  return to_result_(out);
}

Result Analytic::price(const instrument::Vanilla& inst,
                       const model::Sabr& mdl,
                       const market::State& mkt,
                       RiskSpec risk,
                       Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::Analytic eng{};
  if (risk == RiskSpec::All) return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // Black's formula differentiated, the smile by stencils
  auto res = to_result_(object::price(inst, mdl, mkt, eng, cfg));
  return res;
}

Result Spectral::price(const instrument::Vanilla& inst,
                       const model::BlackScholes& mdl,
                       const market::State& mkt,
                       RiskSpec risk,
                       Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::Spectral eng{.options = {.nodes = nodes, .quadrature = quadrature, .max_iterations = max_iterations, .tolerance = tolerance}};
  if (risk == RiskSpec::All) return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // spot and time derivatives from the solved boundary
  auto res = to_result_(object::price(inst, mdl, mkt, eng, cfg));
  return res;
}

Result Pde::price(const instrument::Vanilla& inst,
                  const model::Heston& mdl,
                  const market::State& mkt,
                  RiskSpec risk,
                  Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::Pde eng{.options = {.spot_nodes = spot_nodes, .variance_nodes = variance_nodes, .time_steps = time_steps}};
  if (risk == RiskSpec::All) return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // Greeks read off the solved surface
  auto res = to_result_(object::price(inst, mdl, mkt, eng, cfg));
  return res;
}

Result COS::price(const instrument::Vanilla& inst,
                  const model::BlackScholes& mdl,
                  const market::State& mkt,
                  RiskSpec risk,
                  Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::Cos eng{.options = to_cos_options_(*this)};
  if (risk == RiskSpec::All) return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // European: closed-form Greeks; early exercise: from the final expansion
  auto res = to_result_(object::price(inst, mdl, mkt, eng, cfg));
  return res;
}

Result Lattice::price(const instrument::Vanilla& inst,
                      const model::BlackScholes& mdl,
                      const market::State& mkt,
                      RiskSpec risk,
                      Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::Lattice eng{.steps = steps};
  if (risk == RiskSpec::All && steps >= 2) return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // delta, gamma, theta from the tree nodes
  auto res = to_result_(object::price(inst, mdl, mkt, eng, cfg));
  if (risk == RiskSpec::All) {
    const auto price_fn = [&](const instrument::Vanilla& i, const model::BlackScholes& m, const market::State& s) { return object::price(i, m, s, eng, cfg).price; };
    res.greeks = numeric_transform_greeks_(inst, mdl, mkt, res.price, price_fn, kExerciseSpotStep);
  }
  return res;
}

Result MonteCarlo::price(const instrument::Vanilla& inst,
                         const model::BlackScholes& mdl,
                         const market::State& mkt,
                         RiskSpec risk,
                         Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::MonteCarlo eng{.options = to_mc_options_(*this)};
  if (risk == RiskSpec::All && inst.exercise == pricing::ExerciseStyle::European) return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // pathwise Greeks from the same normals
  if (risk == RiskSpec::All && inst.exercise != pricing::ExerciseStyle::European) return to_result_(object::price_exercise_with_greeks(inst, mdl, mkt, eng, cfg));   // one fit, pathwise Greeks along its exercise rule
  auto res = to_result_(object::price(inst, mdl, mkt, eng, cfg));
  if (risk == RiskSpec::All) {
    const auto price_fn = [&](const instrument::Vanilla& i, const model::BlackScholes& m, const market::State& s) { return object::price(i, m, s, eng, cfg).price; };
    res.greeks = numeric_transform_greeks_(inst, mdl, mkt, res.price, price_fn, inst.exercise == pricing::ExerciseStyle::European ? 1e-4 : kExerciseSpotStep);
  }
  return res;
}

Result MonteCarlo::price(const instrument::Vanilla& inst,
                         const model::Heston& mdl,
                         const market::State& mkt,
                         RiskSpec risk,
                         Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::MonteCarlo eng{.options = to_mc_options_(*this)};
  if (risk == RiskSpec::All && inst.exercise == pricing::ExerciseStyle::European) {
    return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // Greeks from the paths of the one simulation of the strip
  }
  if (risk == RiskSpec::All && inst.exercise != pricing::ExerciseStyle::European) return to_result_(object::price_exercise_with_greeks(inst, mdl, mkt, eng, cfg));   // one fit, pathwise Greeks along its exercise rule
  auto res = to_result_(object::price(inst, mdl, mkt, eng, cfg));
  if (risk == RiskSpec::All) {
    const auto price_fn = [&](const instrument::Vanilla& i, const model::Heston& m, const market::State& s) { return object::price(i, m, s, eng, cfg).price; };
    res.greeks = numeric_transform_greeks_(inst, mdl, mkt, res.price, price_fn, inst.exercise == pricing::ExerciseStyle::European ? 1e-4 : kExerciseSpotStep);
  }
  return res;
}

Result MonteCarlo::price(const instrument::Vanilla& inst,
                         const model::Bates& mdl,
                         const market::State& mkt,
                         RiskSpec risk,
                         Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::MonteCarlo eng{.options = to_mc_options_(*this)};
  if (risk == RiskSpec::All && inst.exercise == pricing::ExerciseStyle::European) {
    return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // Greeks from the paths of the one simulation of the strip
  }
  if (risk == RiskSpec::All && inst.exercise != pricing::ExerciseStyle::European) return to_result_(object::price_exercise_with_greeks(inst, mdl, mkt, eng, cfg));   // one fit, pathwise Greeks along its exercise rule
  auto res = to_result_(object::price(inst, mdl, mkt, eng, cfg));
  if (risk == RiskSpec::All) {
    const auto price_fn = [&](const instrument::Vanilla& i, const model::Bates& m, const market::State& s) { return object::price(i, m, s, eng, cfg).price; };
    res.greeks = numeric_transform_greeks_(inst, mdl, mkt, res.price, price_fn, inst.exercise == pricing::ExerciseStyle::European ? 1e-4 : kExerciseSpotStep);
  }
  return res;
}

Result MonteCarlo::price(const instrument::Vanilla& inst,
                         const model::Sabr& mdl,
                         const market::State& mkt,
                         RiskSpec risk,
                         Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::MonteCarlo eng{.options = to_mc_options_(*this)};
  if (risk == RiskSpec::All && inst.exercise == pricing::ExerciseStyle::European) return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // delta, vega and the Jacobian from the tracked paths
  if (risk == RiskSpec::All) return to_result_(object::price_exercise_with_greeks(inst, mdl, mkt, eng, cfg));   // one fit; delta, rho and vega from the tracked spot, drift and level
  auto res = to_result_(object::price(inst, mdl, mkt, eng, cfg));
  if (risk == RiskSpec::All) {
    const auto price_fn = [&](const instrument::Vanilla& i, const model::Sabr& m, const market::State& s) { return object::price(i, m, s, eng, cfg).price; };
    res.greeks = numeric_transform_greeks_(inst, mdl, mkt, res.price, price_fn, inst.exercise == pricing::ExerciseStyle::European ? 1e-4 : kExerciseSpotStep);
  }
  return res;
}

Result MonteCarlo::price(const instrument::Vanilla& inst,
                         const model::RoughHeston& mdl,
                         const market::State& mkt,
                         RiskSpec risk,
                         Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::MonteCarlo eng{.options = to_mc_options_(*this)};
  if (risk == RiskSpec::All && inst.exercise == pricing::ExerciseStyle::European) return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));
  if (risk == RiskSpec::All && inst.exercise != pricing::ExerciseStyle::European) return to_result_(object::price_exercise_with_greeks(inst, mdl, mkt, eng, cfg));   // one fit, pathwise Greeks along its exercise rule
  auto res = to_result_(object::price(inst, mdl, mkt, eng, cfg));
  if (risk == RiskSpec::All) {
    const auto price_fn = [&](const instrument::Vanilla& i, const model::RoughHeston& m, const market::State& s) { return object::price(i, m, s, eng, cfg).price; };
    res.greeks = numeric_transform_greeks_(inst, mdl, mkt, res.price, price_fn, inst.exercise == pricing::ExerciseStyle::European ? 1e-4 : kExerciseSpotStep);
  }
  return res;
}

Result MonteCarlo::price(const instrument::Vanilla& inst,
                         const model::RoughBergomi& mdl,
                         const market::State& mkt,
                         RiskSpec risk,
                         Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::MonteCarlo eng{.options = to_mc_options_(*this)};
  if (risk == RiskSpec::All && inst.exercise == pricing::ExerciseStyle::European) return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));
  if (risk == RiskSpec::All && inst.exercise != pricing::ExerciseStyle::European) return to_result_(object::price_exercise_with_greeks(inst, mdl, mkt, eng, cfg));   // one fit, pathwise Greeks along its exercise rule
  auto res = to_result_(object::price(inst, mdl, mkt, eng, cfg));
  if (risk == RiskSpec::All) {
    const auto price_fn = [&](const instrument::Vanilla& i, const model::RoughBergomi& m, const market::State& s) { return object::price(i, m, s, eng, cfg).price; };
    res.greeks = numeric_transform_greeks_(inst, mdl, mkt, res.price, price_fn, inst.exercise == pricing::ExerciseStyle::European ? 1e-4 : kExerciseSpotStep);
  }
  return res;
}

Result Fft::price(const instrument::Vanilla& inst,
                  const model::Heston& mdl,
                  const market::State& mkt,
                  RiskSpec risk,
                  Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  PLUSS_CHECK(!(risk == RiskSpec::All && return_grid),
              "subsp::math::finance::pricer::Fft::price: RiskSpec::All is not supported with return_grid=true");
  const object::pricer::Fft eng{
      .options =
          pricing::HestonFftOptions{
              .vol_risk_premium = vol_risk_premium,
              .mode = mode,
              .num_fft = num_fft,
              .characteristic_fcn_step = du,
              .log_strike_step = dk,
              .damping_factor = alpha,
              .quadrature = quadrature,
          },
      .return_grid = return_grid,
  };
  if (risk == RiskSpec::All && mode == pricing::HestonCfMode::LittleTrap && !return_grid) {
    return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // price and Greeks from four transforms of one grid
  }
  const auto out = object::price(
      inst,
      mdl,
      mkt,
      eng,
      cfg);
  auto res = to_result_(out);
  if (risk == RiskSpec::All) {
    // the original form of the characteristic function is the same analytic function as the little-trap form (it only loses accuracy at long maturities), so the
    // Greeks are the exact multiplied transforms of the little-trap form
    auto lt = eng;
    lt.options.mode = pricing::HestonCfMode::LittleTrap;
    res.greeks = to_result_(object::price_with_greeks(inst, mdl, mkt, lt, cfg)).greeks;
  }
  return res;
}

Result Fft::price(const instrument::Vanilla& inst,
                  const model::Bates& mdl,
                  const market::State& mkt,
                  RiskSpec risk,
                  Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  PLUSS_CHECK(!(risk == RiskSpec::All && return_grid),
              "subsp::math::finance::pricer::Fft::price: RiskSpec::All is not supported with return_grid=true");
  const object::pricer::Fft eng{
      .options =
          pricing::HestonFftOptions{
              .vol_risk_premium = vol_risk_premium,
              .mode = mode,
              .num_fft = num_fft,
              .characteristic_fcn_step = du,
              .log_strike_step = dk,
              .damping_factor = alpha,
              .quadrature = quadrature,
          },
      .return_grid = return_grid,
  };
  if (risk == RiskSpec::All && mode == pricing::HestonCfMode::LittleTrap && !return_grid) {
    return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // price and Greeks from four transforms of one grid
  }
  const auto out = object::price(
      inst,
      mdl,
      mkt,
      eng,
      cfg);
  auto res = to_result_(out);
  if (risk == RiskSpec::All) {
    // the original form of the characteristic function is the same analytic function as the little-trap form (it only loses accuracy at long maturities), so the
    // Greeks are the exact multiplied transforms of the little-trap form
    auto lt = eng;
    lt.options.mode = pricing::HestonCfMode::LittleTrap;
    res.greeks = to_result_(object::price_with_greeks(inst, mdl, mkt, lt, cfg)).greeks;
  }
  return res;
}

Result COS::price(const instrument::Vanilla& inst,
                  const model::Heston& mdl,
                  const market::State& mkt,
                  RiskSpec risk,
                  Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::Cos eng{.options = to_cos_options_(*this)};
  if (risk == RiskSpec::All && inst.exercise != pricing::ExerciseStyle::European) return to_result_(object::price_exercise_with_greeks(inst, mdl, mkt, eng, cfg));   // delta, gamma, vega from the final expansion
  if (risk == RiskSpec::All && inst.exercise == pricing::ExerciseStyle::European) {
    return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // closed-form Greeks from one pass over the characteristic function
  }
  const auto out = object::price(inst, mdl, mkt, eng, cfg);
  auto res = to_result_(out);
  return res;
}

Result COS::price(const instrument::Vanilla& inst,
                  const model::Bates& mdl,
                  const market::State& mkt,
                  RiskSpec risk,
                  Config cfg) const {
  validate_inputs_(inst, mkt, mdl);
  const object::pricer::Cos eng{.options = to_cos_options_(*this)};
  if (risk == RiskSpec::All && inst.exercise != pricing::ExerciseStyle::European) return to_result_(object::price_exercise_with_greeks(inst, mdl, mkt, eng, cfg));   // delta, gamma, vega from the final expansion
  if (risk == RiskSpec::All && inst.exercise == pricing::ExerciseStyle::European) {
    return to_result_(object::price_with_greeks(inst, mdl, mkt, eng, cfg));   // closed-form Greeks from one pass over the characteristic function
  }
  const auto out = object::price(inst, mdl, mkt, eng, cfg);
  auto res = to_result_(out);
  return res;
}

} // namespace pluss::subsp::math::finance::pricer

namespace pluss::subsp::math::finance {

instrument::Vanilla instrument::make(pricing::OptionType type, double strike, double maturity, pluss::codem::Config cfg) {
  return instrument::Vanilla{
      .type = type,
      .strike = pluss::series::full({1}, strike, cfg),
      .maturity = pluss::series::full({1}, maturity, cfg),
  };
}

instrument::Vanilla instrument::american(pricing::OptionType type, double strike, double maturity, pluss::codem::Config cfg) {
  auto inst = instrument::make(type, strike, maturity, cfg);
  inst.exercise = pricing::ExerciseStyle::American;
  return inst;
}

instrument::Vanilla instrument::bermudan(pricing::OptionType type, double strike, double maturity, std::span<const double> dates,
                                         pluss::codem::Config cfg) {
  auto inst = instrument::make(type, strike, maturity, cfg);
  inst.exercise = pricing::ExerciseStyle::Bermudan;
  inst.exercise_times = pluss::make_series(std::vector<double>(dates.begin(), dates.end()), {static_cast<std::int64_t>(dates.size())}, cfg);
  return inst;
}

market::State market::make(double spot, double rate, double dividend_yield, pluss::codem::Config cfg) {
  return market::State{
      .spot = pluss::series::full({1}, spot, cfg),
      .rate = pluss::series::full({1}, rate, cfg),
      .dividend_yield = pluss::series::full({1}, dividend_yield, cfg),
  };
}

} // namespace pluss::subsp::math::finance
