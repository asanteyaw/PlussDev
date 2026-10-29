#include "pluss/subsp/math/finance/objects.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <string>

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
                         Config cfg) {
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
  return out;
}

} // namespace

PriceResult price(const Vanilla& inst,
                  const model::BlackScholes& mdl,
                  const MarketState& mkt,
                  const pricer::Analytic& eng,
                  Config cfg) {
  return bs_price_analytic_(inst, mdl, mkt, eng.with_greeks, cfg);
}

PriceResult price(const Vanilla& inst,
                  const model::BlackScholes& mdl,
                  const MarketState& mkt,
                  const pricer::MonteCarlo& eng,
                  Config cfg) {
  return bs_price_mc_(inst, mdl, mkt, eng, cfg);
}

PriceResult price(const Vanilla& inst,
                  const model::Heston& mdl,
                  const MarketState& mkt,
                  const pricer::Fft& eng,
                  Config cfg) {
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
    const auto grid = pricing::heston_price_fft_grid(type, spot, rate, q, maturity, params, eng.options, out_cfg);
    return PriceResult{
        .price = grid.price,
        .strike_grid = grid.strike,
    };
  }

  if (ref.numel() == 1) {
    const double spot = spot_v.reshape({1}).select(0, 0).item<double>();
    const double strike = strike_v.reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.reshape({1}).select(0, 0).item<double>();
    const double q = q_v.reshape({1}).select(0, 0).item<double>();
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const double px = pricing::heston_price_fft(type, spot, strike, rate, q, maturity, params, eng.options, out_cfg);
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
        pricing::heston_price_fft(type, spot, strike, rate, q, maturity, params, eng.options, out_cfg);
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
    const auto grid = pricing::bates_price_fft_grid(type, spot, rate, q, maturity, params, eng.options, out_cfg);
    return PriceResult{
        .price = grid.price,
        .strike_grid = grid.strike,
    };
  }

  if (ref.numel() == 1) {
    const double spot = spot_v.reshape({1}).select(0, 0).item<double>();
    const double strike = strike_v.reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.reshape({1}).select(0, 0).item<double>();
    const double q = q_v.reshape({1}).select(0, 0).item<double>();
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const double px = pricing::bates_price_fft(type, spot, strike, rate, q, maturity, params, eng.options, out_cfg);
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
        pricing::bates_price_fft(type, spot, strike, rate, q, maturity, params, eng.options, out_cfg);
  }
  return PriceResult{
      .price = pluss::make_series(px, {ref.numel()}, out_cfg).reshape(ref.sizes()),
  };
}

PriceResult price(const Vanilla& inst,
                  const model::Heston& mdl,
                  const MarketState& mkt,
                  const pricer::Cos& eng,
                  Config cfg) {
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

  if (ref.numel() == 1) {
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const double px = pricing::heston_price_cos(type,
                                                spot_v.reshape({1}).select(0, 0).item<double>(),
                                                strike_v.reshape({1}).select(0, 0).item<double>(),
                                                rate_v.reshape({1}).select(0, 0).item<double>(),
                                                q_v.reshape({1}).select(0, 0).item<double>(),
                                                maturity_v.reshape({1}).select(0, 0).item<double>(),
                                                params,
                                                eng.options,
                                                out_cfg);
    return PriceResult{.price = pluss::series::full({1}, px, out_cfg)};
  }

  std::vector<double> px(static_cast<std::size_t>(ref.numel()));
  for (std::int64_t i = 0; i < ref.numel(); ++i) {
    const auto type = option_type_from_mask_(is_call_mask_v, i);
    px[static_cast<std::size_t>(i)] =
        pricing::heston_price_cos(type,
                                  spot_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                  strike_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                  rate_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                  q_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                  maturity_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                  params,
                                  eng.options,
                                  out_cfg);
  }
  return PriceResult{
      .price = pluss::make_series(px, {ref.numel()}, out_cfg).reshape(ref.sizes()),
  };
}

PriceResult price(const Vanilla& inst,
                  const model::Bates& mdl,
                  const MarketState& mkt,
                  const pricer::Cos& eng,
                  Config cfg) {
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

  if (ref.numel() == 1) {
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const double px = pricing::bates_price_cos(type,
                                               spot_v.reshape({1}).select(0, 0).item<double>(),
                                               strike_v.reshape({1}).select(0, 0).item<double>(),
                                               rate_v.reshape({1}).select(0, 0).item<double>(),
                                               q_v.reshape({1}).select(0, 0).item<double>(),
                                               maturity_v.reshape({1}).select(0, 0).item<double>(),
                                               params,
                                               eng.options,
                                               out_cfg);
    return PriceResult{.price = pluss::series::full({1}, px, out_cfg)};
  }

  std::vector<double> px(static_cast<std::size_t>(ref.numel()));
  for (std::int64_t i = 0; i < ref.numel(); ++i) {
    const auto type = option_type_from_mask_(is_call_mask_v, i);
    px[static_cast<std::size_t>(i)] =
        pricing::bates_price_cos(type,
                                 spot_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                 strike_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                 rate_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                 q_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                 maturity_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                 params,
                                 eng.options,
                                 out_cfg);
  }
  return PriceResult{
      .price = pluss::make_series(px, {ref.numel()}, out_cfg).reshape(ref.sizes()),
  };
}

PriceResult price(const Vanilla& inst,
                  const model::Heston& mdl,
                  const MarketState& mkt,
                  const pricer::MonteCarlo& eng,
                  Config cfg) {
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

  if (ref.numel() == 1) {
    const double spot = spot_v.reshape({1}).select(0, 0).item<double>();
    const double strike = strike_v.reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.reshape({1}).select(0, 0).item<double>();
    const double q = q_v.reshape({1}).select(0, 0).item<double>();
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const double px = pricing::heston_price_mc(type, spot, strike, rate, q, maturity, params, eng.options, out_cfg);
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
        pricing::heston_price_mc(type, spot, strike, rate, q, maturity, params, eng.options, out_cfg);
  }
  return PriceResult{
      .price = pluss::make_series(px, {ref.numel()}, out_cfg).reshape(ref.sizes()),
  };
}

PriceResult price(const Vanilla& inst,
                  const model::Bates& mdl,
                  const MarketState& mkt,
                  const pricer::MonteCarlo& eng,
                  Config cfg) {
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

  if (ref.numel() == 1) {
    const double spot = spot_v.reshape({1}).select(0, 0).item<double>();
    const double strike = strike_v.reshape({1}).select(0, 0).item<double>();
    const double maturity = maturity_v.reshape({1}).select(0, 0).item<double>();
    const double rate = rate_v.reshape({1}).select(0, 0).item<double>();
    const double q = q_v.reshape({1}).select(0, 0).item<double>();
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const double px = pricing::bates_price_mc(type, spot, strike, rate, q, maturity, params, eng.options, out_cfg);
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
        pricing::bates_price_mc(type, spot, strike, rate, q, maturity, params, eng.options, out_cfg);
  }
  return PriceResult{
      .price = pluss::make_series(px, {ref.numel()}, out_cfg).reshape(ref.sizes()),
  };
}

PriceResult price(const Vanilla& inst,
                  const model::Sabr& mdl,
                  const MarketState& mkt,
                  const pricer::Analytic&,
                  Config cfg) {
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
    const double px = pricing::sabr_price_approx(type, forward, strike, rate, maturity, params, mdl.vol_type, mdl.approx);
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
        pricing::sabr_price_approx(type, forward, strike, rate, maturity, params, mdl.vol_type, mdl.approx);
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
    const double px = pricing::sabr_price_mc(type, forward, strike, rate, maturity, params, eng.options, out_cfg);
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
    px[static_cast<std::size_t>(i)] = pricing::sabr_price_mc(type, forward, strike, rate, maturity, params, eng.options, out_cfg);
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
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = rough_heston_params_from_series_(mdl, out_cfg);

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
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const double px = pricing::rough_heston_price_mc(type,
                                                     spot_v.reshape({1}).select(0, 0).item<double>(),
                                                     strike_v.reshape({1}).select(0, 0).item<double>(),
                                                     rate_v.reshape({1}).select(0, 0).item<double>(),
                                                     q_v.reshape({1}).select(0, 0).item<double>(),
                                                     maturity_v.reshape({1}).select(0, 0).item<double>(),
                                                     params,
                                                     eng.options,
                                                     out_cfg);
    return PriceResult{.price = pluss::series::full({1}, px, out_cfg)};
  }

  std::vector<double> px(static_cast<std::size_t>(ref.numel()));
  for (std::int64_t i = 0; i < ref.numel(); ++i) {
    const auto type = option_type_from_mask_(is_call_mask_v, i);
    px[static_cast<std::size_t>(i)] =
        pricing::rough_heston_price_mc(type,
                                       spot_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                       strike_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                       rate_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                       q_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                       maturity_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                       params,
                                       eng.options,
                                       out_cfg);
  }
  return PriceResult{
      .price = pluss::make_series(px, {ref.numel()}, out_cfg).reshape(ref.sizes()),
  };
}

PriceResult price(const Vanilla& inst,
                  const model::RoughBergomi& mdl,
                  const MarketState& mkt,
                  const pricer::MonteCarlo& eng,
                  Config cfg) {
  const auto out_cfg = f64_cfg_(cfg, mkt.spot.platform());
  const auto params = rough_bergomi_params_from_series_(mdl, out_cfg);

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
    const auto type = option_type_from_mask_(is_call_mask_v, 0);
    const double px = pricing::rough_bergomi_price_mc(type,
                                                      spot_v.reshape({1}).select(0, 0).item<double>(),
                                                      strike_v.reshape({1}).select(0, 0).item<double>(),
                                                      rate_v.reshape({1}).select(0, 0).item<double>(),
                                                      q_v.reshape({1}).select(0, 0).item<double>(),
                                                      maturity_v.reshape({1}).select(0, 0).item<double>(),
                                                      params,
                                                      eng.options,
                                                      out_cfg);
    return PriceResult{.price = pluss::series::full({1}, px, out_cfg)};
  }

  std::vector<double> px(static_cast<std::size_t>(ref.numel()));
  for (std::int64_t i = 0; i < ref.numel(); ++i) {
    const auto type = option_type_from_mask_(is_call_mask_v, i);
    px[static_cast<std::size_t>(i)] =
        pricing::rough_bergomi_price_mc(type,
                                        spot_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                        strike_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                        rate_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                        q_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                        maturity_v.select(0, i).reshape({1}).select(0, 0).item<double>(),
                                        params,
                                        eng.options,
                                        out_cfg);
  }
  return PriceResult{
      .price = pluss::make_series(px, {ref.numel()}, out_cfg).reshape(ref.sizes()),
  };
}

} // namespace pluss::subsp::math::finance::object

namespace pluss::subsp::math::finance::pricer {
namespace {

using pluss::codem::Config;
using pluss::codem::Series;

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
  };
}

pricing::CosOptions to_cos_options_(const COS& cos) {
  return pricing::CosOptions{
      .num_terms = cos.num_terms,
      .truncation = cos.truncation,
      .cumulant_step = cos.cumulant_step,
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

template <typename Model, typename PriceFn>
Greeks numeric_transform_greeks_(const instrument::Vanilla& inst,
                                 const Model& mdl,
                                 const market::State& mkt,
                                 const Series& base_price,
                                 const PriceFn& price_fn) {
  constexpr double kSpotRel = 1e-4;
  constexpr double kSpotAbs = 1e-6;
  constexpr double kRateAbs = 1e-4;
  constexpr double kTauRel = 1e-4;
  constexpr double kTauAbs = 1e-5;
  constexpr double kVRel = 1e-4;
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

  const Series v_step = mdl.v0.abs().mul(kVRel).clamp_min(kVAbs);
  Model mdl_vu = mdl;
  Model mdl_vd = mdl;
  mdl_vu.v0 = mdl.v0.add(v_step);
  mdl_vd.v0 = mdl.v0.sub(v_step).clamp_min(kFloor);
  const Series p_vu = price_fn(inst, mdl_vu, mkt);
  const Series p_vd = price_fn(inst, mdl_vd, mkt);
  const Series vega = p_vu.sub(p_vd).div(mdl_vu.v0.sub(mdl_vd.v0).clamp_min(kFloor));

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
  const bool ask_greeks = with_greeks && (risk == RiskSpec::All);
  const auto out = object::price(inst, mdl, mkt, object::pricer::Analytic{.with_greeks = ask_greeks}, cfg);
  return to_result_(out);
}

Result Analytic::price(const instrument::Vanilla& inst,
                       const model::Sabr& mdl,
                       const market::State& mkt,
                       RiskSpec,
                       Config cfg) const {
  const auto out = object::price(inst, mdl, mkt, object::pricer::Analytic{}, cfg);
  return to_result_(out);
}

Result MonteCarlo::price(const instrument::Vanilla& inst,
                         const model::BlackScholes& mdl,
                         const market::State& mkt,
                         RiskSpec,
                         Config cfg) const {
  const auto out = object::price(inst, mdl, mkt, object::pricer::MonteCarlo{.options = to_mc_options_(*this)}, cfg);
  return to_result_(out);
}

Result MonteCarlo::price(const instrument::Vanilla& inst,
                         const model::Heston& mdl,
                         const market::State& mkt,
                         RiskSpec,
                         Config cfg) const {
  const auto out = object::price(inst, mdl, mkt, object::pricer::MonteCarlo{.options = to_mc_options_(*this)}, cfg);
  return to_result_(out);
}

Result MonteCarlo::price(const instrument::Vanilla& inst,
                         const model::Bates& mdl,
                         const market::State& mkt,
                         RiskSpec,
                         Config cfg) const {
  const auto out = object::price(inst, mdl, mkt, object::pricer::MonteCarlo{.options = to_mc_options_(*this)}, cfg);
  return to_result_(out);
}

Result MonteCarlo::price(const instrument::Vanilla& inst,
                         const model::Sabr& mdl,
                         const market::State& mkt,
                         RiskSpec,
                         Config cfg) const {
  const auto out = object::price(inst, mdl, mkt, object::pricer::MonteCarlo{.options = to_mc_options_(*this)}, cfg);
  return to_result_(out);
}

Result MonteCarlo::price(const instrument::Vanilla& inst,
                         const model::RoughHeston& mdl,
                         const market::State& mkt,
                         RiskSpec,
                         Config cfg) const {
  const auto out = object::price(inst, mdl, mkt, object::pricer::MonteCarlo{.options = to_mc_options_(*this)}, cfg);
  return to_result_(out);
}

Result MonteCarlo::price(const instrument::Vanilla& inst,
                         const model::RoughBergomi& mdl,
                         const market::State& mkt,
                         RiskSpec,
                         Config cfg) const {
  const auto out = object::price(inst, mdl, mkt, object::pricer::MonteCarlo{.options = to_mc_options_(*this)}, cfg);
  return to_result_(out);
}

Result Fft::price(const instrument::Vanilla& inst,
                  const model::Heston& mdl,
                  const market::State& mkt,
                  RiskSpec risk,
                  Config cfg) const {
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
  const auto out = object::price(
      inst,
      mdl,
      mkt,
      eng,
      cfg);
  auto res = to_result_(out);
  if (risk == RiskSpec::All) {
    const auto price_fn = [&](const instrument::Vanilla& i, const model::Heston& m, const market::State& s) {
      return object::price(i, m, s, eng, cfg).price;
    };
    res.greeks = numeric_transform_greeks_(inst, mdl, mkt, res.price, price_fn);
  }
  return res;
}

Result Fft::price(const instrument::Vanilla& inst,
                  const model::Bates& mdl,
                  const market::State& mkt,
                  RiskSpec risk,
                  Config cfg) const {
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
  const auto out = object::price(
      inst,
      mdl,
      mkt,
      eng,
      cfg);
  auto res = to_result_(out);
  if (risk == RiskSpec::All) {
    const auto price_fn = [&](const instrument::Vanilla& i, const model::Bates& m, const market::State& s) {
      return object::price(i, m, s, eng, cfg).price;
    };
    res.greeks = numeric_transform_greeks_(inst, mdl, mkt, res.price, price_fn);
  }
  return res;
}

Result COS::price(const instrument::Vanilla& inst,
                  const model::Heston& mdl,
                  const market::State& mkt,
                  RiskSpec risk,
                  Config cfg) const {
  const object::pricer::Cos eng{.options = to_cos_options_(*this)};
  const auto out = object::price(inst, mdl, mkt, eng, cfg);
  auto res = to_result_(out);
  if (risk == RiskSpec::All) {
    const auto price_fn = [&](const instrument::Vanilla& i, const model::Heston& m, const market::State& s) {
      return object::price(i, m, s, eng, cfg).price;
    };
    res.greeks = numeric_transform_greeks_(inst, mdl, mkt, res.price, price_fn);
  }
  return res;
}

Result COS::price(const instrument::Vanilla& inst,
                  const model::Bates& mdl,
                  const market::State& mkt,
                  RiskSpec risk,
                  Config cfg) const {
  const object::pricer::Cos eng{.options = to_cos_options_(*this)};
  const auto out = object::price(inst, mdl, mkt, eng, cfg);
  auto res = to_result_(out);
  if (risk == RiskSpec::All) {
    const auto price_fn = [&](const instrument::Vanilla& i, const model::Bates& m, const market::State& s) {
      return object::price(i, m, s, eng, cfg).price;
    };
    res.greeks = numeric_transform_greeks_(inst, mdl, mkt, res.price, price_fn);
  }
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

market::State market::make(double spot, double rate, double dividend_yield, pluss::codem::Config cfg) {
  return market::State{
      .spot = pluss::series::full({1}, spot, cfg),
      .rate = pluss::series::full({1}, rate, cfg),
      .dividend_yield = pluss::series::full({1}, dividend_yield, cfg),
  };
}

model::Heston model::make(double kappa,
                          double theta,
                          double sigma,
                          double rho,
                          double v0,
                          pluss::codem::Config cfg) {
  return model::Heston{
      .kappa = pluss::series::full({1}, kappa, cfg),
      .theta = pluss::series::full({1}, theta, cfg),
      .sigma = pluss::series::full({1}, sigma, cfg),
      .rho = pluss::series::full({1}, rho, cfg),
      .v0 = pluss::series::full({1}, v0, cfg),
  };
}

model::Bates model::make(double kappa,
                         double theta,
                         double sigma,
                         double rho,
                         double v0,
                         double jump_intensity,
                         double jump_mean,
                         double jump_vol,
                         pluss::codem::Config cfg) {
  return model::Bates{
      .kappa = pluss::series::full({1}, kappa, cfg),
      .theta = pluss::series::full({1}, theta, cfg),
      .sigma = pluss::series::full({1}, sigma, cfg),
      .rho = pluss::series::full({1}, rho, cfg),
      .v0 = pluss::series::full({1}, v0, cfg),
      .jump_intensity = pluss::series::full({1}, jump_intensity, cfg),
      .jump_mean = pluss::series::full({1}, jump_mean, cfg),
      .jump_vol = pluss::series::full({1}, jump_vol, cfg),
  };
}

model::Sabr model::make(double alpha0,
                        double beta,
                        double nu,
                        double rho,
                        pricing::SabrVolType vol_type,
                        pricing::SabrApproxOptions approx,
                        pluss::codem::Config cfg) {
  return model::Sabr{
      .alpha0 = pluss::series::full({1}, alpha0, cfg),
      .beta = pluss::series::full({1}, beta, cfg),
      .nu = pluss::series::full({1}, nu, cfg),
      .rho = pluss::series::full({1}, rho, cfg),
      .vol_type = vol_type,
      .approx = approx,
  };
}

} // namespace pluss::subsp::math::finance
