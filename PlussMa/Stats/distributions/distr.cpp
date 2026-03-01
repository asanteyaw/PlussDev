#include "pluss/subsp/stat/distr.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/quasirandom_factory.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>

namespace pluss::subsp::stat::distr {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kEps = 1e-12;
constexpr double kSqrt2 = 1.4142135623730950488;
constexpr double kLog2Pi = 1.8378770664093454836;

enum class Family : std::uint8_t {
  Normal,
  Lognormal,
  Uniform,
  Exponential,
  Bernoulli,
  Cauchy,
  Laplace,
  Logistic,
  Geometric,
  Rayleigh,
  Weibull,
};

Config float64_cfg_like_(const Series& x) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(x.platform());
}

Config float64_cfg_on_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Config natural_cfg_(Config cfg, ScalarType natural) {
  const ScalarType dtype = (cfg.dtype == ScalarType::Float32) ? natural : cfg.dtype;
  return Config{}.with_dtype(dtype).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

std::int64_t numel_from_sizes_(std::span<const std::int64_t> sizes) {
  std::int64_t out = 1;
  for (const std::int64_t s : sizes) {
    PLUSS_CHECK(s >= 0, "subsp::stat::distr: sizes must be >= 0");
    out *= s;
  }
  return out;
}

Series as_f64_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stat::distr::") + op + ": expected defined Series");
  return x.to(ScalarType::Float64);
}

Series full_like_f64_(const Series& x, double value) {
  return pluss::full(x.sizes(), value, float64_cfg_like_(x));
}

Series zeros_like_f64_(const Series& x) {
  return full_like_f64_(x, 0.0);
}

Series ones_like_f64_(const Series& x) {
  return full_like_f64_(x, 1.0);
}

Series unit_uniform_(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  return pluss::rand(sizes, float64_cfg_on_(cfg), seed).clamp_min(kEps).clamp_max(1.0 - kEps);
}

std::string normalize_name_(std::string_view name) {
  std::string out;
  out.reserve(name.size());
  for (const char ch : name) {
    const unsigned char u = static_cast<unsigned char>(ch);
    if (std::isalnum(u) != 0) {
      out.push_back(static_cast<char>(std::tolower(u)));
    }
  }
  return out;
}

Family resolve_(std::string_view name) {
  const std::string key = normalize_name_(name);

  if (key == "norm" || key == "normal" || key == "gaussian" || key == "gauss") return Family::Normal;
  if (key == "lognorm" || key == "lognormal" || key == "cobbdouglas" || key == "antilognormal") return Family::Lognormal;
  if (key == "uniform" || key == "unif") return Family::Uniform;
  if (key == "exp" || key == "exponential") return Family::Exponential;
  if (key == "bern" || key == "bernoulli") return Family::Bernoulli;
  if (key == "cauchy" || key == "lorentz" || key == "caushy") return Family::Cauchy;
  if (key == "laplace" || key == "doubleexponential" || key == "doubleexp" ||
      key == "bilateralexponential" || key == "bilateralexp") return Family::Laplace;
  if (key == "logistic") return Family::Logistic;
  if (key == "geom" || key == "geometric" || key == "furry") return Family::Geometric;
  if (key == "rayl" || key == "rayleigh") return Family::Rayleigh;
  if (key == "weibull" || key == "wbl") return Family::Weibull;

  PLUSS_CHECK(false,
              "subsp::stat::distr: unsupported distribution '" + std::string(name) + "'");
  return Family::Normal;
}

bool supports_impl_(std::string_view name) noexcept {
  const std::string key = normalize_name_(name);
  return key == "norm" || key == "normal" || key == "gaussian" || key == "gauss" ||
         key == "lognorm" || key == "lognormal" || key == "cobbdouglas" || key == "antilognormal" ||
         key == "uniform" || key == "unif" ||
         key == "exp" || key == "exponential" ||
         key == "bern" || key == "bernoulli" ||
         key == "cauchy" || key == "lorentz" || key == "caushy" ||
         key == "laplace" || key == "doubleexponential" || key == "doubleexp" ||
         key == "bilateralexponential" || key == "bilateralexp" ||
         key == "logistic" ||
         key == "geom" || key == "geometric" || key == "furry" ||
         key == "rayl" || key == "rayleigh" ||
         key == "weibull" || key == "wbl";
}

void expect_param_count_(std::string_view name,
                         std::span<const double> params,
                         std::initializer_list<std::size_t> accepted) {
  if (std::find(accepted.begin(), accepted.end(), params.size()) != accepted.end()) {
    return;
  }

  std::string counts;
  bool first = true;
  for (const std::size_t n : accepted) {
    if (!first) counts += " or ";
    counts += std::to_string(n);
    first = false;
  }

  PLUSS_CHECK(false,
              "subsp::stat::distr::" + std::string(name) +
                  ": expected " + counts + " parameters, got " + std::to_string(params.size()));
}

double param_or_(std::span<const double> params, std::size_t idx, double default_value) {
  return idx < params.size() ? params[idx] : default_value;
}

Series normal_pdf_(const Series& x, double mean, double stddev) {
  PLUSS_CHECK(stddev > 0.0, "subsp::stat::distr::normal pdf: std must be > 0");
  const Series xx = as_f64_(x, "pdf");
  const Series z = xx.sub(mean).div(stddev);
  return z.square().mul(-0.5).exp().div(stddev * std::sqrt(2.0 * std::numbers::pi_v<double>));
}

Series normal_logpdf_(const Series& x, double mean, double stddev) {
  PLUSS_CHECK(stddev > 0.0, "subsp::stat::distr::normal logpdf: std must be > 0");
  const Series xx = as_f64_(x, "logpdf");
  const Series z = xx.sub(mean).div(stddev);
  return z.square().mul(-0.5).sub(std::log(stddev)).sub(0.5 * kLog2Pi);
}

Series normal_cdf_(const Series& x, double mean, double stddev) {
  PLUSS_CHECK(stddev > 0.0, "subsp::stat::distr::normal cdf: std must be > 0");
  const Series xx = as_f64_(x, "cdf");
  return xx.sub(mean).div(stddev * kSqrt2).erf().add(1.0).mul(0.5);
}

Series normal_sample_(double mean,
                      double stddev,
                      std::span<const std::int64_t> sizes,
                      Config cfg,
                      std::uint64_t seed) {
  PLUSS_CHECK(stddev >= 0.0, "subsp::stat::distr::normal sample: std must be >= 0");
  return pluss::normal(mean, stddev, sizes, cfg, seed);
}

Series lognormal_pdf_(const Series& x, double mu, double sigma) {
  PLUSS_CHECK(sigma > 0.0, "subsp::stat::distr::lognormal pdf: sigma must be > 0");
  const Series xx = as_f64_(x, "pdf");
  const Series safe_x = xx.clamp_min(kEps);
  const Series z = safe_x.log().sub(mu).div(sigma);
  const Series core =
      z.square().mul(-0.5).exp().div(safe_x.mul(sigma * std::sqrt(2.0 * std::numbers::pi_v<double>)));
  return core.where(xx.gt(0.0), zeros_like_f64_(xx));
}

Series lognormal_logpdf_(const Series& x, double mu, double sigma) {
  PLUSS_CHECK(sigma > 0.0, "subsp::stat::distr::lognormal logpdf: sigma must be > 0");
  const Series xx = as_f64_(x, "logpdf");
  const Series safe_x = xx.clamp_min(kEps);
  const Series z = safe_x.log().sub(mu).div(sigma);
  const Series core = z.square().mul(-0.5).sub(safe_x.log()).sub(std::log(sigma)).sub(0.5 * kLog2Pi);
  return core.where(xx.gt(0.0), full_like_f64_(xx, -std::numeric_limits<double>::infinity()));
}

Series lognormal_cdf_(const Series& x, double mu, double sigma) {
  PLUSS_CHECK(sigma > 0.0, "subsp::stat::distr::lognormal cdf: sigma must be > 0");
  const Series xx = as_f64_(x, "cdf");
  const Series safe_x = xx.clamp_min(kEps);
  const Series core = safe_x.log().sub(mu).div(sigma * kSqrt2).erf().add(1.0).mul(0.5);
  return core.where(xx.gt(0.0), zeros_like_f64_(xx));
}

Series lognormal_sample_(double mu,
                         double sigma,
                         std::span<const std::int64_t> sizes,
                         Config cfg,
                         std::uint64_t seed) {
  return normal_sample_(mu, sigma, sizes, cfg, seed).exp();
}

Series uniform_pdf_(const Series& x, double lo, double hi) {
  PLUSS_CHECK(hi > lo, "subsp::stat::distr::uniform pdf: expected hi > lo");
  const Series xx = as_f64_(x, "pdf");
  const Series mask = xx.ge(lo).logical_and(xx.le(hi));
  return mask.to(ScalarType::Float64).mul(1.0 / (hi - lo));
}

Series uniform_logpdf_(const Series& x, double lo, double hi) {
  PLUSS_CHECK(hi > lo, "subsp::stat::distr::uniform logpdf: expected hi > lo");
  const Series xx = as_f64_(x, "logpdf");
  const Series mask = xx.ge(lo).logical_and(xx.le(hi));
  return full_like_f64_(xx, -std::log(hi - lo))
      .where(mask, full_like_f64_(xx, -std::numeric_limits<double>::infinity()));
}

Series uniform_cdf_(const Series& x, double lo, double hi) {
  PLUSS_CHECK(hi > lo, "subsp::stat::distr::uniform cdf: expected hi > lo");
  const Series xx = as_f64_(x, "cdf");
  return xx.sub(lo).div(hi - lo).clamp_min(0.0).clamp_max(1.0);
}

Series uniform_sample_(double lo,
                       double hi,
                       std::span<const std::int64_t> sizes,
                       Config cfg,
                       std::uint64_t seed) {
  PLUSS_CHECK(hi > lo, "subsp::stat::distr::uniform sample: expected hi > lo");
  return pluss::rand(sizes, cfg, seed).mul(hi - lo).add(lo);
}

Series exponential_pdf_(const Series& x, double rate) {
  PLUSS_CHECK(rate > 0.0, "subsp::stat::distr::exponential pdf: rate must be > 0");
  const Series xx = as_f64_(x, "pdf");
  const Series core = xx.mul(-rate).exp().mul(rate);
  return core.where(xx.ge(0.0), zeros_like_f64_(xx));
}

Series exponential_logpdf_(const Series& x, double rate) {
  PLUSS_CHECK(rate > 0.0, "subsp::stat::distr::exponential logpdf: rate must be > 0");
  const Series xx = as_f64_(x, "logpdf");
  const Series core = xx.mul(-rate).add(std::log(rate));
  return core.where(xx.ge(0.0), full_like_f64_(xx, -std::numeric_limits<double>::infinity()));
}

Series exponential_cdf_(const Series& x, double rate) {
  PLUSS_CHECK(rate > 0.0, "subsp::stat::distr::exponential cdf: rate must be > 0");
  const Series xx = as_f64_(x, "cdf");
  const Series core = xx.mul(-rate).exp().mul(-1.0).add(1.0);
  return core.where(xx.ge(0.0), zeros_like_f64_(xx));
}

Series exponential_sample_(double rate,
                           std::span<const std::int64_t> sizes,
                           Config cfg,
                           std::uint64_t seed) {
  PLUSS_CHECK(rate > 0.0, "subsp::stat::distr::exponential sample: rate must be > 0");
  return unit_uniform_(sizes, cfg, seed).log().mul(-1.0 / rate).to(cfg.dtype);
}

Series bernoulli_pdf_(const Series& x, double p) {
  PLUSS_CHECK(p >= 0.0 && p <= 1.0, "subsp::stat::distr::bernoulli pdf: expected 0 <= p <= 1");
  const Series xx = as_f64_(x, "pdf");
  return xx.eq(1.0).to(ScalarType::Float64).mul(p)
      .add(xx.eq(0.0).to(ScalarType::Float64).mul(1.0 - p));
}

Series bernoulli_logpdf_(const Series& x, double p) {
  PLUSS_CHECK(p >= 0.0 && p <= 1.0, "subsp::stat::distr::bernoulli logpdf: expected 0 <= p <= 1");
  const Series probs = bernoulli_pdf_(x, p);
  return probs.log().where(probs.gt(0.0), full_like_f64_(probs, -std::numeric_limits<double>::infinity()));
}

Series bernoulli_cdf_(const Series& x, double p) {
  PLUSS_CHECK(p >= 0.0 && p <= 1.0, "subsp::stat::distr::bernoulli cdf: expected 0 <= p <= 1");
  const Series xx = as_f64_(x, "cdf");
  const Series zero = zeros_like_f64_(xx);
  const Series one = ones_like_f64_(xx);
  const Series mid_or_one = full_like_f64_(xx, 1.0 - p).where(xx.lt(1.0), one);
  return zero.where(xx.lt(0.0), mid_or_one);
}

Series bernoulli_sample_(double p,
                         std::span<const std::int64_t> sizes,
                         Config cfg,
                         std::uint64_t seed) {
  PLUSS_CHECK(p >= 0.0 && p <= 1.0, "subsp::stat::distr::bernoulli sample: expected 0 <= p <= 1");
  return pluss::bernoulli(sizes, p, natural_cfg_(cfg, ScalarType::Bool), seed);
}

Series cauchy_pdf_(const Series& x, double location, double scale) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::cauchy pdf: scale must be > 0");
  const Series xx = as_f64_(x, "pdf");
  const Series z2 = xx.sub(location).div(scale).square();
  return z2.add(1.0).mul(std::numbers::pi_v<double> * scale).reciprocal();
}

Series cauchy_logpdf_(const Series& x, double location, double scale) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::cauchy logpdf: scale must be > 0");
  const Series xx = as_f64_(x, "logpdf");
  const Series z2 = xx.sub(location).div(scale).square();
  return z2.add(1.0).log().mul(-1.0).sub(std::log(std::numbers::pi_v<double> * scale));
}

Series cauchy_cdf_(const Series& x, double location, double scale) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::cauchy cdf: scale must be > 0");
  const Series xx = as_f64_(x, "cdf");
  return xx.sub(location).div(scale).atan().div(std::numbers::pi_v<double>).add(0.5);
}

Series cauchy_sample_(double location,
                      double scale,
                      std::span<const std::int64_t> sizes,
                      Config cfg,
                      std::uint64_t seed) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::cauchy sample: scale must be > 0");
  return unit_uniform_(sizes, cfg, seed).sub(0.5).mul(std::numbers::pi_v<double>).tan().mul(scale).add(location).to(cfg.dtype);
}

Series laplace_pdf_(const Series& x, double location, double scale) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::laplace pdf: scale must be > 0");
  const Series xx = as_f64_(x, "pdf");
  return xx.sub(location).abs().div(scale).mul(-1.0).exp().mul(0.5 / scale);
}

Series laplace_logpdf_(const Series& x, double location, double scale) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::laplace logpdf: scale must be > 0");
  const Series xx = as_f64_(x, "logpdf");
  return xx.sub(location).abs().div(scale).mul(-1.0).sub(std::log(2.0 * scale));
}

Series laplace_cdf_(const Series& x, double location, double scale) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::laplace cdf: scale must be > 0");
  const Series xx = as_f64_(x, "cdf");
  const Series centered = xx.sub(location);
  const Series left = centered.div(scale).exp().mul(0.5);
  const Series right = centered.div(scale).mul(-1.0).exp().mul(-0.5).add(1.0);
  return left.where(centered.lt(0.0), right);
}

Series laplace_sample_(double location,
                       double scale,
                       std::span<const std::int64_t> sizes,
                       Config cfg,
                       std::uint64_t seed) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::laplace sample: scale must be > 0");
  const Series centered = unit_uniform_(sizes, cfg, seed).sub(0.5);
  const Series sign = centered.ge(0.0).to(ScalarType::Float64).mul(2.0).sub(1.0);
  return sign.mul(-scale)
      .mul(centered.abs().mul(-2.0).add(1.0).log())
      .add(location)
      .to(cfg.dtype);
}

Series logistic_pdf_(const Series& x, double location, double scale) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::logistic pdf: scale must be > 0");
  const Series xx = as_f64_(x, "pdf");
  const Series sig = xx.sub(location).div(scale).sigmoid();
  return sig.mul(sig.mul(-1.0).add(1.0)).div(scale);
}

Series logistic_logpdf_(const Series& x, double location, double scale) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::logistic logpdf: scale must be > 0");
  const Series xx = as_f64_(x, "logpdf");
  const Series z = xx.sub(location).div(scale);
  return z.neg().softplus().mul(-2.0).sub(z).sub(std::log(scale));
}

Series logistic_cdf_(const Series& x, double location, double scale) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::logistic cdf: scale must be > 0");
  return as_f64_(x, "cdf").sub(location).div(scale).sigmoid();
}

Series logistic_sample_(double location,
                        double scale,
                        std::span<const std::int64_t> sizes,
                        Config cfg,
                        std::uint64_t seed) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::logistic sample: scale must be > 0");
  const Series u = unit_uniform_(sizes, cfg, seed);
  return u.log().sub(u.mul(-1.0).add(1.0).log()).mul(scale).add(location).to(cfg.dtype);
}

Series geometric_pdf_(const Series& x, double p) {
  PLUSS_CHECK(p > 0.0 && p <= 1.0, "subsp::stat::distr::geometric pdf: expected 0 < p <= 1");
  const Series xx = as_f64_(x, "pdf");
  const Series k = xx.floor();
  const Series valid = xx.eq(k).logical_and(xx.ge(1.0));
  const Series core = k.sub(1.0).mul(std::log1p(-p)).exp().mul(p);
  return core.where(valid, zeros_like_f64_(xx));
}

Series geometric_logpdf_(const Series& x, double p) {
  PLUSS_CHECK(p > 0.0 && p <= 1.0, "subsp::stat::distr::geometric logpdf: expected 0 < p <= 1");
  const Series xx = as_f64_(x, "logpdf");
  const Series k = xx.floor();
  const Series valid = xx.eq(k).logical_and(xx.ge(1.0));
  const Series core = k.sub(1.0).mul(std::log1p(-p)).add(std::log(p));
  return core.where(valid, full_like_f64_(xx, -std::numeric_limits<double>::infinity()));
}

Series geometric_cdf_(const Series& x, double p) {
  PLUSS_CHECK(p > 0.0 && p <= 1.0, "subsp::stat::distr::geometric cdf: expected 0 < p <= 1");
  const Series xx = as_f64_(x, "cdf");
  const Series floored = xx.floor().clamp_min(0.0);
  const Series core = floored.mul(std::log1p(-p)).exp().mul(-1.0).add(1.0);
  return zeros_like_f64_(xx).where(xx.lt(1.0), core);
}

Series geometric_sample_(double p,
                         std::span<const std::int64_t> sizes,
                         Config cfg,
                         std::uint64_t seed) {
  PLUSS_CHECK(p > 0.0 && p <= 1.0, "subsp::stat::distr::geometric sample: expected 0 < p <= 1");
  return unit_uniform_(sizes, cfg, seed)
      .mul(-1.0)
      .add(1.0)
      .log()
      .div(std::log1p(-p))
      .floor()
      .add(1.0)
      .to(natural_cfg_(cfg, ScalarType::Int64).dtype);
}

Series rayleigh_pdf_(const Series& x, double sigma) {
  PLUSS_CHECK(sigma > 0.0, "subsp::stat::distr::rayleigh pdf: sigma must be > 0");
  const Series xx = as_f64_(x, "pdf");
  const double sigma2 = sigma * sigma;
  const Series core = xx.div(sigma2).mul(xx.square().div(2.0 * sigma2).mul(-1.0).exp());
  return core.where(xx.ge(0.0), zeros_like_f64_(xx));
}

Series rayleigh_logpdf_(const Series& x, double sigma) {
  PLUSS_CHECK(sigma > 0.0, "subsp::stat::distr::rayleigh logpdf: sigma must be > 0");
  const Series xx = as_f64_(x, "logpdf");
  const double sigma2 = sigma * sigma;
  const Series safe = xx.clamp_min(kEps);
  const Series core = safe.log().sub(std::log(sigma2)).sub(safe.square().div(2.0 * sigma2));
  return core.where(xx.ge(0.0), full_like_f64_(xx, -std::numeric_limits<double>::infinity()));
}

Series rayleigh_cdf_(const Series& x, double sigma) {
  PLUSS_CHECK(sigma > 0.0, "subsp::stat::distr::rayleigh cdf: sigma must be > 0");
  const Series xx = as_f64_(x, "cdf");
  const double sigma2 = sigma * sigma;
  const Series core = xx.square().div(2.0 * sigma2).mul(-1.0).exp().mul(-1.0).add(1.0);
  return core.where(xx.ge(0.0), zeros_like_f64_(xx));
}

Series rayleigh_sample_(double sigma,
                        std::span<const std::int64_t> sizes,
                        Config cfg,
                        std::uint64_t seed) {
  PLUSS_CHECK(sigma > 0.0, "subsp::stat::distr::rayleigh sample: sigma must be > 0");
  return unit_uniform_(sizes, cfg, seed).log().mul(-2.0).sqrt().mul(sigma).to(cfg.dtype);
}

Series weibull_pdf_(const Series& x, double shape, double scale) {
  PLUSS_CHECK(shape > 0.0 && scale > 0.0,
              "subsp::stat::distr::weibull pdf: shape and scale must be > 0");
  const Series xx = as_f64_(x, "pdf");
  const Series safe = xx.clamp_min(kEps);
  const Series z = safe.div(scale);
  const Series core = z.pow(shape - 1.0).mul(shape / scale).mul(z.pow(shape).mul(-1.0).exp());
  return core.where(xx.ge(0.0), zeros_like_f64_(xx));
}

Series weibull_logpdf_(const Series& x, double shape, double scale) {
  PLUSS_CHECK(shape > 0.0 && scale > 0.0,
              "subsp::stat::distr::weibull logpdf: shape and scale must be > 0");
  const Series xx = as_f64_(x, "logpdf");
  const Series safe = xx.clamp_min(kEps);
  const Series z = safe.div(scale);
  const Series core = z.log().mul(shape - 1.0).add(std::log(shape / scale)).sub(z.pow(shape));
  return core.where(xx.ge(0.0), full_like_f64_(xx, -std::numeric_limits<double>::infinity()));
}

Series weibull_cdf_(const Series& x, double shape, double scale) {
  PLUSS_CHECK(shape > 0.0 && scale > 0.0,
              "subsp::stat::distr::weibull cdf: shape and scale must be > 0");
  const Series xx = as_f64_(x, "cdf");
  const Series core = xx.clamp_min(0.0).div(scale).pow(shape).mul(-1.0).exp().mul(-1.0).add(1.0);
  return core.where(xx.ge(0.0), zeros_like_f64_(xx));
}

Series weibull_sample_(double shape,
                       double scale,
                       std::span<const std::int64_t> sizes,
                       Config cfg,
                       std::uint64_t seed) {
  PLUSS_CHECK(shape > 0.0 && scale > 0.0,
              "subsp::stat::distr::weibull sample: shape and scale must be > 0");
  return unit_uniform_(sizes, cfg, seed).log().mul(-1.0).pow(1.0 / shape).mul(scale).to(cfg.dtype);
}

} // namespace

bool supports(std::string_view name) noexcept {
  return supports_impl_(name);
}

Series sample(std::string_view name,
              std::span<const double> params,
              std::span<const std::int64_t> sizes,
              Config cfg,
              std::uint64_t seed) {
  switch (resolve_(name)) {
    case Family::Normal:
      expect_param_count_("normal sample", params, {0, 2});
      return normal_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, 1.0), sizes, cfg, seed);
    case Family::Lognormal:
      expect_param_count_("lognormal sample", params, {0, 2});
      return lognormal_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, 1.0), sizes, cfg, seed);
    case Family::Uniform:
      expect_param_count_("uniform sample", params, {2});
      return uniform_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::Exponential:
      expect_param_count_("exponential sample", params, {0, 1});
      return exponential_sample_(param_or_(params, 0, 1.0), sizes, cfg, seed);
    case Family::Bernoulli:
      expect_param_count_("bernoulli sample", params, {0, 1});
      return bernoulli_sample_(param_or_(params, 0, 0.5), sizes, cfg, seed);
    case Family::Cauchy:
      expect_param_count_("cauchy sample", params, {0, 2});
      return cauchy_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, 1.0), sizes, cfg, seed);
    case Family::Laplace:
      expect_param_count_("laplace sample", params, {0, 2});
      return laplace_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, 1.0), sizes, cfg, seed);
    case Family::Logistic:
      expect_param_count_("logistic sample", params, {0, 2});
      return logistic_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, 1.0), sizes, cfg, seed);
    case Family::Geometric:
      expect_param_count_("geometric sample", params, {0, 1});
      return geometric_sample_(param_or_(params, 0, 0.5), sizes, cfg, seed);
    case Family::Rayleigh:
      expect_param_count_("rayleigh sample", params, {0, 1});
      return rayleigh_sample_(param_or_(params, 0, 1.0), sizes, cfg, seed);
    case Family::Weibull:
      expect_param_count_("weibull sample", params, {2});
      return weibull_sample_(params[0], params[1], sizes, cfg, seed);
  }

  return Series::undefined();
}

Series sample(std::string_view name,
              std::initializer_list<double> params,
              std::span<const std::int64_t> sizes,
              Config cfg,
              std::uint64_t seed) {
  return sample(name, std::span<const double>(params.begin(), params.size()), sizes, cfg, seed);
}

Series sample(std::string_view name,
              std::initializer_list<double> params,
              std::initializer_list<std::int64_t> sizes,
              Config cfg,
              std::uint64_t seed) {
  return sample(name,
                std::span<const double>(params.begin(), params.size()),
                std::span<const std::int64_t>(sizes.begin(), sizes.size()),
                cfg,
                seed);
}

Series sample_like(std::string_view name,
                   std::span<const double> params,
                   const Series& like,
                   std::uint64_t seed) {
  PLUSS_CHECK(like.defined(), "subsp::stat::distr::sample_like: expected defined Series");
  const Config cfg = Config{}.with_dtype(like.scalar_type()).with_platform(like.platform());
  return sample(name, params, like.sizes(), cfg, seed);
}

Series sample_like(std::string_view name,
                   std::initializer_list<double> params,
                   const Series& like,
                   std::uint64_t seed) {
  return sample_like(name, std::span<const double>(params.begin(), params.size()), like, seed);
}

Series mixture_normal(const Series& weights,
                      const Series& means,
                      const Series& stddevs,
                      std::span<const std::int64_t> sizes,
                      Config cfg,
                      std::uint64_t seed,
                      bool use_sobol) {
  PLUSS_CHECK(weights.defined(), "subsp::stat::distr::mixture_normal: weights must be defined");
  PLUSS_CHECK(means.defined(), "subsp::stat::distr::mixture_normal: means must be defined");
  PLUSS_CHECK(stddevs.defined(), "subsp::stat::distr::mixture_normal: stddevs must be defined");

  const Series w = weights.to(pluss::pCPU, ScalarType::Float64).reshape({weights.numel()});
  const Series m_cpu = means.to(pluss::pCPU, ScalarType::Float64).reshape({means.numel()});
  const Series s_cpu = stddevs.to(pluss::pCPU, ScalarType::Float64).reshape({stddevs.numel()});
  const Series m = m_cpu.to(cfg.platform, ScalarType::Float64);
  const Series s = s_cpu.to(cfg.platform, ScalarType::Float64);

  PLUSS_CHECK(
      w.numel() == m.numel() && w.numel() == s.numel(),
      "subsp::stat::distr::mixture_normal: weights, means, stddevs must have same numel");
  PLUSS_CHECK(w.numel() > 0, "subsp::stat::distr::mixture_normal: expected at least one component");
  PLUSS_CHECK(s_cpu.amin(0, false).item<double>() >= 0.0,
              "subsp::stat::distr::mixture_normal: stddevs must be >= 0");

  const double total_weight = w.sum().item<double>();
  PLUSS_CHECK(total_weight > 0.0, "subsp::stat::distr::mixture_normal: weights must sum to > 0");

  const std::vector<double> probs = w.div(total_weight).to_vec<double>();
  const std::int64_t k = w.numel();
  const std::int64_t total = numel_from_sizes_(sizes);

  const Series selectors = pluss::multinomial(probs, total, true, seed)
                               .to(cfg.platform, ScalarType::Int64)
                               .reshape({total, 1});
  const Series component_ids =
      pluss::arange(0, k, Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform)).reshape({1, k});
  const Series selector_mask = component_ids.eq(selectors).to(ScalarType::Float64);

  const Series base = (use_sobol
                           ? pluss::sobol_normal({total, k}, Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform), seed)
                           : pluss::randn({total, k}, Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform), seed));
  const Series components = m.reshape({1, k}).add(s.reshape({1, k}).mul(base));
  return selector_mask.mul(components).sum(1, false).reshape(sizes).to(cfg.dtype);
}

Series mixture_normal(const Series& weights,
                      const Series& means,
                      const Series& stddevs,
                      std::initializer_list<std::int64_t> sizes,
                      Config cfg,
                      std::uint64_t seed,
                      bool use_sobol) {
  return mixture_normal(
      weights,
      means,
      stddevs,
      std::span<const std::int64_t>(sizes.begin(), sizes.size()),
      cfg,
      seed,
      use_sobol);
}

Series pdf(std::string_view name, const Series& x, std::span<const double> params) {
  switch (resolve_(name)) {
    case Family::Normal:
      expect_param_count_("normal pdf", params, {0, 2});
      return normal_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Lognormal:
      expect_param_count_("lognormal pdf", params, {0, 2});
      return lognormal_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Uniform:
      expect_param_count_("uniform pdf", params, {2});
      return uniform_pdf_(x, params[0], params[1]);
    case Family::Exponential:
      expect_param_count_("exponential pdf", params, {0, 1});
      return exponential_pdf_(x, param_or_(params, 0, 1.0));
    case Family::Bernoulli:
      expect_param_count_("bernoulli pdf", params, {0, 1});
      return bernoulli_pdf_(x, param_or_(params, 0, 0.5));
    case Family::Cauchy:
      expect_param_count_("cauchy pdf", params, {0, 2});
      return cauchy_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Laplace:
      expect_param_count_("laplace pdf", params, {0, 2});
      return laplace_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Logistic:
      expect_param_count_("logistic pdf", params, {0, 2});
      return logistic_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Geometric:
      expect_param_count_("geometric pdf", params, {0, 1});
      return geometric_pdf_(x, param_or_(params, 0, 0.5));
    case Family::Rayleigh:
      expect_param_count_("rayleigh pdf", params, {0, 1});
      return rayleigh_pdf_(x, param_or_(params, 0, 1.0));
    case Family::Weibull:
      expect_param_count_("weibull pdf", params, {2});
      return weibull_pdf_(x, params[0], params[1]);
  }

  return Series::undefined();
}

Series pdf(std::string_view name, const Series& x, std::initializer_list<double> params) {
  return pdf(name, x, std::span<const double>(params.begin(), params.size()));
}

Series logpdf(std::string_view name, const Series& x, std::span<const double> params) {
  switch (resolve_(name)) {
    case Family::Normal:
      expect_param_count_("normal logpdf", params, {0, 2});
      return normal_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Lognormal:
      expect_param_count_("lognormal logpdf", params, {0, 2});
      return lognormal_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Uniform:
      expect_param_count_("uniform logpdf", params, {2});
      return uniform_logpdf_(x, params[0], params[1]);
    case Family::Exponential:
      expect_param_count_("exponential logpdf", params, {0, 1});
      return exponential_logpdf_(x, param_or_(params, 0, 1.0));
    case Family::Bernoulli:
      expect_param_count_("bernoulli logpdf", params, {0, 1});
      return bernoulli_logpdf_(x, param_or_(params, 0, 0.5));
    case Family::Cauchy:
      expect_param_count_("cauchy logpdf", params, {0, 2});
      return cauchy_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Laplace:
      expect_param_count_("laplace logpdf", params, {0, 2});
      return laplace_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Logistic:
      expect_param_count_("logistic logpdf", params, {0, 2});
      return logistic_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Geometric:
      expect_param_count_("geometric logpdf", params, {0, 1});
      return geometric_logpdf_(x, param_or_(params, 0, 0.5));
    case Family::Rayleigh:
      expect_param_count_("rayleigh logpdf", params, {0, 1});
      return rayleigh_logpdf_(x, param_or_(params, 0, 1.0));
    case Family::Weibull:
      expect_param_count_("weibull logpdf", params, {2});
      return weibull_logpdf_(x, params[0], params[1]);
  }

  return Series::undefined();
}

Series logpdf(std::string_view name, const Series& x, std::initializer_list<double> params) {
  return logpdf(name, x, std::span<const double>(params.begin(), params.size()));
}

Series cdf(std::string_view name, const Series& x, std::span<const double> params) {
  switch (resolve_(name)) {
    case Family::Normal:
      expect_param_count_("normal cdf", params, {0, 2});
      return normal_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Lognormal:
      expect_param_count_("lognormal cdf", params, {0, 2});
      return lognormal_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Uniform:
      expect_param_count_("uniform cdf", params, {2});
      return uniform_cdf_(x, params[0], params[1]);
    case Family::Exponential:
      expect_param_count_("exponential cdf", params, {0, 1});
      return exponential_cdf_(x, param_or_(params, 0, 1.0));
    case Family::Bernoulli:
      expect_param_count_("bernoulli cdf", params, {0, 1});
      return bernoulli_cdf_(x, param_or_(params, 0, 0.5));
    case Family::Cauchy:
      expect_param_count_("cauchy cdf", params, {0, 2});
      return cauchy_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Laplace:
      expect_param_count_("laplace cdf", params, {0, 2});
      return laplace_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Logistic:
      expect_param_count_("logistic cdf", params, {0, 2});
      return logistic_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Geometric:
      expect_param_count_("geometric cdf", params, {0, 1});
      return geometric_cdf_(x, param_or_(params, 0, 0.5));
    case Family::Rayleigh:
      expect_param_count_("rayleigh cdf", params, {0, 1});
      return rayleigh_cdf_(x, param_or_(params, 0, 1.0));
    case Family::Weibull:
      expect_param_count_("weibull cdf", params, {2});
      return weibull_cdf_(x, params[0], params[1]);
  }

  return Series::undefined();
}

Series cdf(std::string_view name, const Series& x, std::initializer_list<double> params) {
  return cdf(name, x, std::span<const double>(params.begin(), params.size()));
}

} // namespace pluss::subsp::stat::distr
