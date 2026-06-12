#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <string>

namespace pluss::subsp::math::finance::pricing {
namespace {

using pluss::Config;
using pluss::Series;

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

void validate_cos_options_(const CosOptions& opt, const char* op) {
  PLUSS_CHECK(opt.num_terms >= 2,
              std::string("subsp::math::finance::pricing::") + op + ": num_terms must be >= 2");
  PLUSS_CHECK(std::isfinite(opt.truncation) && opt.truncation > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": truncation must be finite and > 0");
  PLUSS_CHECK(std::isfinite(opt.cumulant_step) && opt.cumulant_step > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": cumulant_step must be finite and > 0");
}

double intrinsic_(OptionType type, double spot, double strike) {
  if (type == OptionType::Call) {
    return std::max(0.0, spot - strike);
  }
  return std::max(0.0, strike - spot);
}

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_f64_(const Series& x, Config cfg) {
  return x.to(cfg.platform, pluss::ScalarType::Float64).contiguous();
}

Series scalar_(double value, Config cfg) {
  return pluss::full({}, value, f64_cfg_(cfg));
}

ComplexSeries c_make_(const Series& re, const Series& im) {
  return ComplexSeries{re, im};
}

ComplexSeries c_add_(const ComplexSeries& a, const ComplexSeries& b) {
  return c_make_(a.real.add(b.real), a.imag.add(b.imag));
}

ComplexSeries c_sub_(const ComplexSeries& a, const ComplexSeries& b) {
  return c_make_(a.real.sub(b.real), a.imag.sub(b.imag));
}

ComplexSeries c_mul_(const ComplexSeries& a, const ComplexSeries& b) {
  return c_make_(a.real.mul(b.real).sub(a.imag.mul(b.imag)),
                 a.real.mul(b.imag).add(a.imag.mul(b.real)));
}

ComplexSeries c_mul_(const ComplexSeries& a, double b) {
  return c_make_(a.real.mul(b), a.imag.mul(b));
}

ComplexSeries c_div_(const ComplexSeries& a, double b) {
  return c_make_(a.real.div(b), a.imag.div(b));
}

ComplexSeries c_exp_(const ComplexSeries& z) {
  const Series er = z.real.exp();
  return c_make_(er.mul(z.imag.cos()), er.mul(z.imag.sin()));
}

ComplexSeries c_log_(const ComplexSeries& z) {
  return c_make_(z.real.hypot(z.imag).clamp_min(kEps).log(), z.imag.atan2(z.real));
}

ComplexSeries checked_cf_(const CharacteristicSeriesFn& char_fn, const Series& u, Config cfg, const char* op) {
  ComplexSeries z = char_fn(u);
  PLUSS_CHECK(z.real.defined() && z.imag.defined(),
              std::string("subsp::math::finance::pricing::") + op +
                  ": char_fn returned undefined real or imag part");
  z.real = as_f64_(z.real, cfg).contiguous();
  z.imag = as_f64_(z.imag, cfg).contiguous();
  PLUSS_CHECK(z.real.isfinite().all().item<bool>() && z.imag.isfinite().all().item<bool>(),
              std::string("subsp::math::finance::pricing::") + op + ": char_fn returned non-finite values");
  return z;
}

std::complex<double> safe_log_cf_(const CharacteristicFn& char_fn, double u, const char* op) {
  const auto z = char_fn(std::complex<double>(u, 0.0));
  PLUSS_CHECK(std::isfinite(z.real()) && std::isfinite(z.imag()),
              std::string("subsp::math::finance::pricing::") + op + ": char_fn returned non-finite value");
  const double absz = std::abs(z);
  PLUSS_CHECK(std::isfinite(absz) && absz > kEps,
              std::string("subsp::math::finance::pricing::") + op + ": char_fn magnitude too small at cumulant probe");
  return std::log(z);
}

std::pair<double, double> cos_truncation_interval_(double spot,
                                                   const CharacteristicFn& char_fn,
                                                   const CosOptions& options,
                                                   const char* op) {
  const double h = options.cumulant_step;
  const auto fm2 = safe_log_cf_(char_fn, -2.0 * h, op);
  const auto fm1 = safe_log_cf_(char_fn, -h, op);
  const auto f0 = safe_log_cf_(char_fn, 0.0, op);
  const auto fp1 = safe_log_cf_(char_fn, h, op);
  const auto fp2 = safe_log_cf_(char_fn, 2.0 * h, op);

  const auto d1 = (fm2 - 8.0 * fm1 + 8.0 * fp1 - fp2) / (12.0 * h);
  const auto d2 = (-fm2 + 16.0 * fm1 - 30.0 * f0 + 16.0 * fp1 - fp2) / (12.0 * h * h);
  const auto d4 = (fm2 - 4.0 * fm1 + 6.0 * f0 - 4.0 * fp1 + fp2) / (h * h * h * h);

  const double c1 = std::imag(d1);
  const double c2 = std::max(kEps, -std::real(d2));
  const double c4 = std::max(0.0, std::real(d4));
  const double width = options.truncation * std::sqrt(c2 + std::sqrt(c4 + kEps));

  if (!std::isfinite(c1) || !std::isfinite(width) || width <= 0.0) {
    const double log_s = std::log(spot);
    return {log_s - options.truncation, log_s + options.truncation};
  }
  return {c1 - width, c1 + width};
}

std::pair<Series, Series> cos_truncation_interval_series_(const Series& spot,
                                                          const CharacteristicSeriesFn& char_fn,
                                                          const CosOptions& options,
                                                          Config cfg,
                                                          const char* op) {
  const double h = options.cumulant_step;
  const Series like = as_f64_(spot, cfg).flatten(0, -1).reshape({1, spot.numel()});
  const Series um2 = pluss::full(like.sizes(), -2.0 * h, f64_cfg_(cfg));
  const Series um1 = pluss::full(like.sizes(), -h, f64_cfg_(cfg));
  const Series u0 = pluss::full(like.sizes(), 0.0, f64_cfg_(cfg));
  const Series up1 = pluss::full(like.sizes(), h, f64_cfg_(cfg));
  const Series up2 = pluss::full(like.sizes(), 2.0 * h, f64_cfg_(cfg));

  const ComplexSeries fm2 = c_log_(checked_cf_(char_fn, um2, cfg, op));
  const ComplexSeries fm1 = c_log_(checked_cf_(char_fn, um1, cfg, op));
  const ComplexSeries f0 = c_log_(checked_cf_(char_fn, u0, cfg, op));
  const ComplexSeries fp1 = c_log_(checked_cf_(char_fn, up1, cfg, op));
  const ComplexSeries fp2 = c_log_(checked_cf_(char_fn, up2, cfg, op));

  const ComplexSeries d1 = c_div_(c_sub_(c_add_(fm2, c_mul_(fp1, 8.0)), c_add_(c_mul_(fm1, 8.0), fp2)), 12.0 * h);
  const ComplexSeries d2 = c_div_(
      c_add_(c_add_(c_mul_(fm2, -1.0), c_mul_(fm1, 16.0)),
             c_add_(c_mul_(f0, -30.0), c_add_(c_mul_(fp1, 16.0), c_mul_(fp2, -1.0)))),
      12.0 * h * h);
  const ComplexSeries d4 = c_div_(
      c_add_(c_add_(fm2, c_mul_(fm1, -4.0)),
             c_add_(c_mul_(f0, 6.0), c_add_(c_mul_(fp1, -4.0), fp2))),
      h * h * h * h);

  const Series c1 = d1.imag;
  const Series c2 = d2.real.mul(-1.0).clamp_min(kEps);
  const Series c4 = d4.real.clamp_min(0.0);
  const Series width = c2.add(c4.add(kEps).sqrt()).sqrt().mul(options.truncation);
  return {c1.sub(width), c1.add(width)};
}

double cos_chi_(std::int64_t k, double a, double b, double c, double d) {
  if (d <= c) return 0.0;
  if (k == 0) return std::exp(d) - std::exp(c);
  const double u = static_cast<double>(k) * kPi / (b - a);
  const double ea = std::exp(c);
  const double eb = std::exp(d);
  const double ua = u * (c - a);
  const double ub = u * (d - a);
  return (eb * std::cos(ub) - ea * std::cos(ua) + u * (eb * std::sin(ub) - ea * std::sin(ua))) / (1.0 + u * u);
}

double cos_psi_(std::int64_t k, double a, double b, double c, double d) {
  if (d <= c) return 0.0;
  if (k == 0) return d - c;
  const double u = static_cast<double>(k) * kPi / (b - a);
  return (std::sin(u * (d - a)) - std::sin(u * (c - a))) / u;
}

} // namespace

double option_price_cos(OptionType type,
                        double spot,
                        double strike,
                        double rate,
                        double dividend_yield,
                        double maturity,
                        const CharacteristicFn& char_fn,
                        const CosOptions& options,
                        Config cfg) {
  (void)cfg;
  validate_option_(type, "option_price_cos");
  validate_common_(spot, strike, rate, dividend_yield, maturity, "option_price_cos");
  PLUSS_CHECK(static_cast<bool>(char_fn), "subsp::math::finance::pricing::option_price_cos: char_fn must be valid");
  validate_cos_options_(options, "option_price_cos");

  if (maturity <= 0.0) return intrinsic_(type, spot, strike);

  const auto [a, b] = cos_truncation_interval_(spot, char_fn, options, "option_price_cos");
  PLUSS_CHECK(std::isfinite(a) && std::isfinite(b) && b > a + kEps,
              "subsp::math::finance::pricing::option_price_cos: invalid truncation interval");

  const double c = std::log(strike);
  const double l = std::max(c, a);
  const double scale = 2.0 / (b - a);

  std::complex<double> acc(0.0, 0.0);
  for (std::int64_t k = 0; k < options.num_terms; ++k) {
    const double u = static_cast<double>(k) * kPi / (b - a);
    const std::complex<double> phi = char_fn(std::complex<double>(u, 0.0));
    PLUSS_CHECK(std::isfinite(phi.real()) && std::isfinite(phi.imag()),
                "subsp::math::finance::pricing::option_price_cos: char_fn returned non-finite value");

    const double hk = scale * (cos_chi_(k, a, b, l, b) - strike * cos_psi_(k, a, b, l, b));
    const std::complex<double> phase = std::exp(std::complex<double>(0.0, -u * a));
    const double wk = (k == 0) ? 0.5 : 1.0;
    acc += wk * phi * phase * hk;
  }

  const double call = std::max(0.0, std::real(acc));
  if (type == OptionType::Call) return call;
  const double disc_q = std::exp(-dividend_yield * maturity);
  const double disc_r = std::exp(-rate * maturity);
  return std::max(0.0, call + strike * disc_r - spot * disc_q);
}

Series option_price_cos(OptionType type,
                        const Series& spot,
                        const Series& strike,
                        double rate,
                        double dividend_yield,
                        const Series& maturity,
                        const CharacteristicSeriesFn& char_fn,
                        const CosOptions& options,
                        Config cfg) {
  validate_option_(type, "option_price_cos");
  PLUSS_CHECK(static_cast<bool>(char_fn), "subsp::math::finance::pricing::option_price_cos: char_fn must be valid");
  validate_cos_options_(options, "option_price_cos");
  PLUSS_CHECK(std::isfinite(rate), "subsp::math::finance::pricing::option_price_cos: rate must be finite");
  PLUSS_CHECK(std::isfinite(dividend_yield),
              "subsp::math::finance::pricing::option_price_cos: dividend_yield must be finite");
  PLUSS_CHECK(spot.defined() && strike.defined() && maturity.defined(),
              "subsp::math::finance::pricing::option_price_cos: spot, strike and maturity must be defined");

  const Config out_cfg = f64_cfg_(cfg);
  const Series spot2 = as_f64_(spot, out_cfg).flatten(0, -1).reshape({1, spot.numel()});
  const Series strike2 = as_f64_(strike, out_cfg).flatten(0, -1).reshape({1, strike.numel()});
  const Series tau2 = as_f64_(maturity, out_cfg).flatten(0, -1).reshape({1, maturity.numel()});
  PLUSS_CHECK(spot2.gt(0.0).all().item<bool>(),
              "subsp::math::finance::pricing::option_price_cos: spot must be > 0");
  PLUSS_CHECK(strike2.gt(0.0).all().item<bool>(),
              "subsp::math::finance::pricing::option_price_cos: strike must be > 0");
  PLUSS_CHECK(tau2.ge(0.0).all().item<bool>(),
              "subsp::math::finance::pricing::option_price_cos: maturity must be >= 0");

  const std::int64_t n = std::max<std::int64_t>(options.num_terms, 8);
  const Series k = pluss::arange(0, n, out_cfg).to(pluss::ScalarType::Float64).reshape({n, 1});
  const auto [a, b] = cos_truncation_interval_series_(spot2, char_fn, options, out_cfg, "option_price_cos");
  const Series width = b.sub(a).clamp_min(kEps);
  const Series u = k.mul(kPi).div(width);
  const Series lower = strike2.log().maximum(a);

  const ComplexSeries phi = checked_cf_(char_fn, u, out_cfg, "option_price_cos");
  const ComplexSeries phase = c_exp_(c_make_(scalar_(0.0, out_cfg), u.mul(a).mul(-1.0)));
  const ComplexSeries kernel = c_mul_(phi, phase);

  const Series ub = u.mul(b.sub(a));
  const Series ul = u.mul(lower.sub(a));
  const Series exp_b = b.exp();
  const Series exp_l = lower.exp();
  const Series chi = exp_b.mul(ub.cos())
                         .sub(exp_l.mul(ul.cos()))
                         .add(u.mul(exp_b.mul(ub.sin()).sub(exp_l.mul(ul.sin()))))
                         .div(u.square().add(1.0));
  const Series psi_general = ub.sin().sub(ul.sin()).div(u.clamp_min(kEps));
  const Series psi_zero = b.sub(lower);
  const Series zero_mask = k.eq(pluss::full({n, 1}, 0.0, out_cfg));
  const Series psi = zero_mask.where(psi_zero, psi_general);
  const Series wk = zero_mask.where(pluss::full({n, 1}, 0.5, out_cfg), pluss::full({n, 1}, 1.0, out_cfg));
  const Series hk = chi.sub(strike2.mul(psi)).mul(2.0).div(width);
  const Series call = kernel.real.mul(hk).mul(wk).sum(0, false).mul(tau2.mul(-rate).exp()).clamp_min(0.0);
  if (type == OptionType::Call) {
    return call.flatten(0, -1);
  }
  return call.add(strike2.mul(tau2.mul(-rate).exp()).sub(spot2.mul(tau2.mul(-dividend_yield).exp())))
      .clamp_min(0.0)
      .flatten(0, -1);
}

} // namespace pluss::subsp::math::finance::pricing
