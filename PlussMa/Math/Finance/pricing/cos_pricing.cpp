#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <string>

namespace pluss::subsp::math::finance::pricing {
namespace {

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

} // namespace pluss::subsp::math::finance::pricing
