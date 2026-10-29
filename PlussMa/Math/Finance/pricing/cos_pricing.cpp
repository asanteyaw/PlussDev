#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <thread>
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

void validate_cos_options_(const CosOptions& opt, const char* op) {
  PLUSS_CHECK(opt.num_terms >= 2,
              std::string("subsp::math::finance::pricing::") + op + ": num_terms must be >= 2");
  PLUSS_CHECK(std::isfinite(opt.truncation) && opt.truncation > 0.0,
              std::string("subsp::math::finance::pricing::") + op + ": truncation must be finite and > 0");
  // Below about 1e-6 the second difference of the characteristic function that gives the variance is lost to rounding, the interval of
  // the expansion is then wrong, and so is the price; refuse such a step rather than return it.
  PLUSS_CHECK(std::isfinite(opt.cumulant_step) && opt.cumulant_step >= 1e-6,
              std::string("subsp::math::finance::pricing::") + op + ": cumulant_step must be finite and >= 1e-6");
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
  return pluss::series::full({}, value, f64_cfg_(cfg));
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

// Truncation interval [c1 - L sqrt(c2 + sqrt(c4)), c1 + L sqrt(c2 + sqrt(c4))]. The mean and variance come from
// a narrow five-point stencil on log(phi); the fourth cumulant uses a stencil scaled to the distribution
// (u ~ 1/sd) on log|phi|, because a fixed tiny step divides rounding noise by h^4 and makes c4 meaningless.
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

  const double c1 = std::imag(d1);
  const double c2 = std::max(kEps, -std::real(d2));

  const double h4 = 0.5 / std::sqrt(c2);
  const auto log_abs = [&](double u) {
    const auto z = char_fn(std::complex<double>(u, 0.0));
    return std::log(std::max(std::abs(z), kEps));
  };
  const double g0 = log_abs(0.0), g1 = log_abs(h4) + log_abs(-h4), g2 = log_abs(2.0 * h4) + log_abs(-2.0 * h4);
  const double c4 = std::max(0.0, (g2 - 4.0 * g1 + 6.0 * g0) / (h4 * h4 * h4 * h4));
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
  const Series um2 = pluss::series::full(like.sizes(), -2.0 * h, f64_cfg_(cfg));
  const Series um1 = pluss::series::full(like.sizes(), -h, f64_cfg_(cfg));
  const Series u0 = pluss::series::full(like.sizes(), 0.0, f64_cfg_(cfg));
  const Series up1 = pluss::series::full(like.sizes(), h, f64_cfg_(cfg));
  const Series up2 = pluss::series::full(like.sizes(), 2.0 * h, f64_cfg_(cfg));

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
  const Series c1 = d1.imag;
  const Series c2 = d2.real.mul(-1.0).clamp_min(kEps);

  const Series h4 = pluss::series::full(like.sizes(), 0.5, f64_cfg_(cfg)).div(c2.sqrt());
  const auto log_abs = [&](double scale) {
    const ComplexSeries z = checked_cf_(char_fn, h4.mul(scale), cfg, op);
    return z.real.hypot(z.imag).clamp_min(kEps).log();
  };
  const Series g0 = c_log_(f0).real;
  const Series g1 = log_abs(-1.0).add(log_abs(1.0));
  const Series g2 = log_abs(-2.0).add(log_abs(2.0));
  const Series c4 = g2.sub(g1.mul(4.0)).add(g0.mul(6.0)).div(h4.square().square()).clamp_min(0.0);
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

// Fused CPU kernel for the vectorized COS call price. For each option column j it accumulates
//   sum_k w_k Re[phi_k e^{-i u_k a}] * 2/(b-a) * (chi_k - K psi_k)
// in one pass, with u_k = k pi / (b - a). The trigonometric factors cos/sin(k theta) come from a rotation
// recurrence reseeded every 32 steps (error below 32 eps), and the endpoint terms are closed forms
// (cos k pi = (-1)^k), so the whole sum needs no n x m temporaries and no per-term transcendental calls.
// Phi may have one column (a strike chain sharing one characteristic function) or one per option.
struct FusedCos_ {
  const double *phi_re, *phi_im;       // [n, phi_cols] row-major
  std::int64_t n, phi_cols;
  const double *a, *b, *lower, *strike, *discount;  // per option, or a single value for a, b
  std::int64_t a_stride, b_stride;
};

double fused_cos_column_(const FusedCos_ &p, std::int64_t j) {
  const double a = p.a[j * p.a_stride], b = p.b[j * p.b_stride];
  const double width = std::max(b - a, kEps);
  const double lower = p.lower[j], strike = p.strike[j];
  const double theta_a = kPi * a / width, theta_l = kPi * (lower - a) / width;
  const double exp_b = std::exp(b), exp_l = std::exp(lower);
  const std::int64_t cols = p.phi_cols, pj = cols == 1 ? 0 : j;
  const double ca = std::cos(theta_a), sa = std::sin(theta_a), cl = std::cos(theta_l), sl = std::sin(theta_l);
  double c_a = 1.0, s_a = 0.0, c_l = 1.0, s_l = 0.0, total = 0.0;
  for (std::int64_t k = 0; k < p.n; ++k) {
    if ((k & 31) == 0 && k > 0) {  // reseed against drift
      c_a = std::cos(static_cast<double>(k) * theta_a); s_a = std::sin(static_cast<double>(k) * theta_a);
      c_l = std::cos(static_cast<double>(k) * theta_l); s_l = std::sin(static_cast<double>(k) * theta_l);
    }
    const double u = static_cast<double>(k) * kPi / width;
    double chi, psi, weight = 1.0;
    if (k == 0) {
      chi = exp_b - exp_l; psi = b - lower; weight = 0.5;
    } else {
      const double sign = (k & 1) ? -1.0 : 1.0;
      chi = (exp_b * sign - exp_l * c_l - u * exp_l * s_l) / (1.0 + u * u);
      psi = -s_l / u;
    }
    const double kernel = p.phi_re[k * cols + pj] * c_a + p.phi_im[k * cols + pj] * s_a;
    total += weight * kernel * (chi - strike * psi);
    const double nca = c_a * ca - s_a * sa, nsa = s_a * ca + c_a * sa;
    c_a = nca; s_a = nsa;
    const double ncl = c_l * cl - s_l * sl, nsl = s_l * cl + c_l * sl;
    c_l = ncl; s_l = nsl;
  }
  return std::max(0.0, total * 2.0 / width * p.discount[j]);
}

std::vector<double> host_f64_(const Series &s) {
  const Series cpu = s.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous();
  return cpu.reshape({cpu.numel()}).host<double>();
}

// true when the fused CPU kernel applies: CPU data and nothing recording a derivative
bool fused_cos_applicable_(const Series &phi_re, const Series &phi_im, const Series &spot, const Series &strike,
                           const Series &tau) {
  for (const Series *x : {&phi_re, &phi_im, &spot, &strike, &tau}) {
    if (!x->is_cpu() || x->tracks_slope()) return false;
  }
  return true;
}

Series fused_cos_call_(const ComplexSeries &phi, const Series &a, const Series &b, const Series &lower,
                       const Series &strike, const Series &discount, std::int64_t n, Config cfg) {
  const std::int64_t m = strike.numel();
  const std::vector<double> re = host_f64_(phi.real), im = host_f64_(phi.imag), av = host_f64_(a), bv = host_f64_(b),
                            lv = host_f64_(lower), kv = host_f64_(strike), dv = host_f64_(discount);
  FusedCos_ p{re.data(), im.data(), n, phi.real.sizes()[1], av.data(), bv.data(), lv.data(), kv.data(), dv.data(),
              a.numel() == 1 ? 0 : 1, b.numel() == 1 ? 0 : 1};
  std::vector<double> out(static_cast<std::size_t>(m));
  const std::int64_t work = n * m;
  const unsigned threads = work < 200000 ? 1u : std::max(1u, std::min(8u, std::thread::hardware_concurrency()));
  if (threads == 1) {
    for (std::int64_t j = 0; j < m; ++j) out[static_cast<std::size_t>(j)] = fused_cos_column_(p, j);
  } else {
    std::vector<std::thread> pool;
    for (unsigned t = 0; t < threads; ++t) {
      pool.emplace_back([&, t] {
        for (std::int64_t j = t; j < m; j += threads) out[static_cast<std::size_t>(j)] = fused_cos_column_(p, j);
      });
    }
    for (std::thread &th : pool) th.join();
  }
  return pluss::make_series(out, {1, m}, f64_cfg_(cfg));
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

  const double disc_r = std::exp(-rate * maturity);
  const double call = std::max(0.0, std::real(acc) * disc_r);
  if (type == OptionType::Call) return call;
  const double disc_q = std::exp(-dividend_yield * maturity);
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

  // The three inputs are paired element by element; an input of length one is repeated against the others.
  const std::int64_t length = std::max({spot.numel(), strike.numel(), maturity.numel()});
  PLUSS_CHECK((spot.numel() == 1 || spot.numel() == length) && (strike.numel() == 1 || strike.numel() == length) &&
                  (maturity.numel() == 1 || maturity.numel() == length),
              "subsp::math::finance::pricing::option_price_cos: spot, strike and maturity must have the same length (or length 1)");

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
  const Series k = pluss::series::arange(0, n, out_cfg).to(pluss::ScalarType::Float64).reshape({n, 1});
  const auto [a, b] = cos_truncation_interval_series_(spot2, char_fn, options, out_cfg, "option_price_cos");
  const Series width = b.sub(a).clamp_min(kEps);
  const Series u = k.mul(kPi).div(width);
  const Series lower = strike2.log().maximum(a);

  const ComplexSeries phi = checked_cf_(char_fn, u, out_cfg, "option_price_cos");
  const Series discount = tau2.mul(-rate).exp();
  Series call = pluss::series::undefined();
  if (fused_cos_applicable_(phi.real, phi.imag, spot2, strike2, tau2) && phi.real.dim() == 2) {
    call = fused_cos_call_(phi, a, b, lower, strike2, discount.add(strike2.mul(0.0)), n, out_cfg);
  } else {
    // At the upper end u (b - a) = k pi, so cos = (-1)^k and sin = 0: only the lower-limit trig is needed, and
    // Re(phi e^{-i u a}) = Re(phi) cos(u a) + Im(phi) sin(u a) needs no complex product.
    const Series sign = k.mul(0.5).floor().mul(2.0).sub(k).mul(2.0).add(1.0);  // (-1)^k
    const Series kernel = phi.real.mul(u.mul(a).cos()).add(phi.imag.mul(u.mul(a).sin()));
    const Series ul = u.mul(lower.sub(a));
    const Series exp_b = b.exp();
    const Series exp_l = lower.exp();
    const Series ul_cos = ul.cos();
    const Series ul_sin = ul.sin();
    const Series chi = exp_b.mul(sign).sub(exp_l.mul(ul_cos)).sub(u.mul(exp_l).mul(ul_sin)).div(u.square().add(1.0));
    const Series psi_general = ul_sin.mul(-1.0).div(u.clamp_min(kEps));
    const Series psi_zero = b.sub(lower);
    const Series zero_mask = k.eq(pluss::series::full({n, 1}, 0.0, out_cfg));
    const Series psi = zero_mask.where(psi_zero, psi_general);
    const Series wk = zero_mask.where(pluss::series::full({n, 1}, 0.5, out_cfg), pluss::series::full({n, 1}, 1.0, out_cfg));
    const Series hk = chi.sub(strike2.mul(psi)).mul(2.0).div(width);
      call = kernel.mul(hk).mul(wk).sum(0, false).mul(tau2.mul(-rate).exp()).clamp_min(0.0);
  }
  if (type == OptionType::Call) {
    return call.flatten(0, -1);
  }
  return call.add(strike2.mul(tau2.mul(-rate).exp()).sub(spot2.mul(tau2.mul(-dividend_yield).exp())))
      .clamp_min(0.0)
      .flatten(0, -1);
}

} // namespace pluss::subsp::math::finance::pricing
