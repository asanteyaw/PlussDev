#include "pluss/subsp/stat/distr.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/quasirandom_factory.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <numbers>
#include <numeric>
#include <random>
#include <string>
#include <string_view>
#include <vector>

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
  Beta,
  Binomial,
  Cauchy,
  ChiSquare,
  Laplace,
  Logistic,
  Geometric,
  Gamma,
  Hypergeometric,
  InverseGaussian,
  NormalTrunc,
  Poisson,
  StudentT,
  Triangular,
  Pareto,
  Lomax,
  Rayleigh,
  Skellam,
  Wald,
  Weibull,
  Zeta,
  Gig,
  Gh,
  Nig,
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
  if (key == "beta" || key == "powerfunction" || key == "powerfunc") return Family::Beta;
  if (key == "binom" || key == "binomial") return Family::Binomial;
  if (key == "cauchy" || key == "lorentz" || key == "caushy") return Family::Cauchy;
  if (key == "chisquare" || key == "chisq" || key == "chi2") return Family::ChiSquare;
  if (key == "laplace" || key == "doubleexponential" || key == "doubleexp" ||
      key == "bilateralexponential" || key == "bilateralexp") return Family::Laplace;
  if (key == "logistic") return Family::Logistic;
  if (key == "geom" || key == "geometric" || key == "furry") return Family::Geometric;
  if (key == "gamma") return Family::Gamma;
  if (key == "hypergeom" || key == "hypergeometric") return Family::Hypergeometric;
  if (key == "ig" || key == "inversegauss" || key == "invgauss") return Family::InverseGaussian;
  if (key == "normaltrunc" || key == "normaltruncated" || key == "gausstrunc") return Family::NormalTrunc;
  if (key == "poisson" || key == "po") return Family::Poisson;
  if (key == "studentst" || key == "t") return Family::StudentT;
  if (key == "tri" || key == "triangular") return Family::Triangular;
  if (key == "pareto") return Family::Pareto;
  if (key == "pareto2" || key == "lomax") return Family::Lomax;
  if (key == "rayl" || key == "rayleigh") return Family::Rayleigh;
  if (key == "skellam") return Family::Skellam;
  if (key == "wald") return Family::Wald;
  if (key == "weibull" || key == "wbl") return Family::Weibull;
  if (key == "zeta" || key == "zipf") return Family::Zeta;
  if (key == "gig") return Family::Gig;
  if (key == "gh") return Family::Gh;
  if (key == "nig") return Family::Nig;

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
         key == "beta" || key == "powerfunction" || key == "powerfunc" ||
         key == "binom" || key == "binomial" ||
         key == "cauchy" || key == "lorentz" || key == "caushy" ||
         key == "chisquare" || key == "chisq" || key == "chi2" ||
         key == "laplace" || key == "doubleexponential" || key == "doubleexp" ||
         key == "bilateralexponential" || key == "bilateralexp" ||
         key == "logistic" ||
         key == "geom" || key == "geometric" || key == "furry" ||
         key == "gamma" ||
         key == "hypergeom" || key == "hypergeometric" ||
         key == "ig" || key == "inversegauss" || key == "invgauss" ||
         key == "normaltrunc" || key == "normaltruncated" || key == "gausstrunc" ||
         key == "poisson" || key == "po" ||
         key == "studentst" || key == "t" ||
         key == "tri" || key == "triangular" ||
         key == "pareto" ||
         key == "pareto2" || key == "lomax" ||
         key == "rayl" || key == "rayleigh" ||
         key == "skellam" ||
         key == "wald" ||
         key == "weibull" || key == "wbl" ||
         key == "zeta" || key == "zipf" ||
         key == "gig" ||
         key == "gh" ||
         key == "nig";
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

template <class Fn>
Series map_series_f64_(const Series& x, Fn&& fn) {
  const Series cpu = x.to(pluss::pCPU, ScalarType::Float64);
  const auto in = cpu.to_vec<double>();
  std::vector<double> out(in.size());
  std::transform(in.begin(), in.end(), out.begin(), std::forward<Fn>(fn));
  return pluss::make_series(out, x.sizes(), Config{}.with_dtype(ScalarType::Float64)).to(x.platform(), ScalarType::Float64);
}

template <class T>
Series make_sample_series_(const std::vector<T>& values,
                           std::span<const std::int64_t> sizes,
                           Config cfg,
                           ScalarType natural_dtype) {
  const ScalarType out_dtype = natural_cfg_(cfg, natural_dtype).dtype;
  return pluss::make_series(values, sizes, Config{}.with_dtype(natural_dtype)).to(cfg.platform, out_dtype);
}

template <class T, class Fn>
Series sample_host_(std::span<const std::int64_t> sizes,
                    Config cfg,
                    ScalarType natural_dtype,
                    std::uint64_t seed,
                    Fn&& fn) {
  const std::int64_t total = numel_from_sizes_(sizes);
  std::mt19937_64 rng(seed);
  std::vector<T> values(static_cast<std::size_t>(total));
  std::generate(values.begin(), values.end(), [&] { return static_cast<T>(fn(rng)); });
  return make_sample_series_(values, sizes, cfg, natural_dtype);
}

double log_beta_scalar_(double a, double b) {
  return std::lgamma(a) + std::lgamma(b) - std::lgamma(a + b);
}

double beta_scalar_(double a, double b) {
  return std::exp(log_beta_scalar_(a, b));
}

double regularized_gamma_p_scalar_(double a, double x) {
  if (!(a > 0.0)) return std::numeric_limits<double>::quiet_NaN();
  if (x <= 0.0) return 0.0;
  constexpr int itmax = 200;
  constexpr double eps = 3e-14;
  constexpr double fpmin = 1e-300;

  if (x < a + 1.0) {
    double sum = 1.0 / a;
    double del = sum;
    double ap = a;
    for (int n = 1; n <= itmax; ++n) {
      ap += 1.0;
      del *= x / ap;
      sum += del;
      if (std::abs(del) < std::abs(sum) * eps) break;
    }
    return sum * std::exp(-x + a * std::log(x) - std::lgamma(a));
  }

  double b = x + 1.0 - a;
  double c = 1.0 / fpmin;
  double d = 1.0 / b;
  double h = d;
  for (int i = 1; i <= itmax; ++i) {
    const double an = -static_cast<double>(i) * (static_cast<double>(i) - a);
    b += 2.0;
    d = an * d + b;
    if (std::abs(d) < fpmin) d = fpmin;
    c = b + an / c;
    if (std::abs(c) < fpmin) c = fpmin;
    d = 1.0 / d;
    const double del = d * c;
    h *= del;
    if (std::abs(del - 1.0) < eps) break;
  }
  return 1.0 - std::exp(-x + a * std::log(x) - std::lgamma(a)) * h;
}

double betacf_scalar_(double a, double b, double x) {
  constexpr int itmax = 200;
  constexpr double eps = 3e-14;
  constexpr double fpmin = 1e-300;
  const double qab = a + b;
  const double qap = a + 1.0;
  const double qam = a - 1.0;
  double c = 1.0;
  double d = 1.0 - qab * x / qap;
  if (std::abs(d) < fpmin) d = fpmin;
  d = 1.0 / d;
  double h = d;
  for (int m = 1; m <= itmax; ++m) {
    const int m2 = 2 * m;
    double aa = m * (b - m) * x / ((qam + m2) * (a + m2));
    d = 1.0 + aa * d;
    if (std::abs(d) < fpmin) d = fpmin;
    c = 1.0 + aa / c;
    if (std::abs(c) < fpmin) c = fpmin;
    d = 1.0 / d;
    h *= d * c;
    aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
    d = 1.0 + aa * d;
    if (std::abs(d) < fpmin) d = fpmin;
    c = 1.0 + aa / c;
    if (std::abs(c) < fpmin) c = fpmin;
    d = 1.0 / d;
    const double del = d * c;
    h *= del;
    if (std::abs(del - 1.0) < eps) break;
  }
  return h;
}

double regularized_beta_scalar_(double a, double b, double x) {
  if (!(a > 0.0) || !(b > 0.0)) return std::numeric_limits<double>::quiet_NaN();
  if (x <= 0.0) return 0.0;
  if (x >= 1.0) return 1.0;
  const double bt = std::exp(std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b) +
                             a * std::log(x) + b * std::log1p(-x));
  if (x < (a + 1.0) / (a + b + 2.0)) {
    return bt * betacf_scalar_(a, b, x) / a;
  }
  return 1.0 - bt * betacf_scalar_(b, a, 1.0 - x) / b;
}

double zeta_scalar_(double s, double q = 1.0) {
  if (!(s > 1.0) || !(q > 0.0)) return std::numeric_limits<double>::quiet_NaN();
  constexpr int n_terms = 4096;
  double acc = 0.0;
  for (int k = 0; k < n_terms; ++k) {
    acc += std::pow(q + static_cast<double>(k), -s);
  }
  const double n = q + static_cast<double>(n_terms);
  const double n_neg_s = std::pow(n, -s);
  const double tail =
      std::pow(n, 1.0 - s) / (s - 1.0) +
      0.5 * n_neg_s +
      (s / 12.0) * n_neg_s / n -
      (s * (s + 1.0) * (s + 2.0) / 720.0) * n_neg_s / (n * n * n);
  return acc + tail;
}

double bessel_i_series_scalar_(double nu, double x) {
  double acc = 0.0;
  const double half = x * 0.5;
  for (int m = 0; m < 48; ++m) {
    const double den = std::tgamma(m + 1.0) * std::tgamma(m + nu + 1.0);
    const double term = std::pow(half, 2.0 * m + nu) / den;
    acc += term;
    if (std::abs(term) < 1e-16) break;
  }
  return acc;
}

double bessel_k_series_scalar_(double order, double x) {
  const double nu = (std::abs(std::sin(std::numbers::pi_v<double> * order)) < 1e-8) ? (order + 1e-6) : order;
  const double s = std::sin(std::numbers::pi_v<double> * nu);
  return (std::numbers::pi_v<double> * 0.5) *
         (bessel_i_series_scalar_(-nu, x) - bessel_i_series_scalar_(nu, x)) / s;
}

double normal_pdf_scalar_(double x, double mean, double stddev) {
  const double z = (x - mean) / stddev;
  return std::exp(-0.5 * z * z) / (stddev * std::sqrt(2.0 * std::numbers::pi_v<double>));
}

double normal_cdf_scalar_(double x, double mean, double stddev) {
  return 0.5 * (1.0 + std::erf((x - mean) / (stddev * kSqrt2)));
}

double inverse_gaussian_pdf_scalar_(double y, double theta, double chi) {
  if (y <= 0.0) return 0.0;
  return std::sqrt(chi / (2.0 * std::numbers::pi_v<double> * y * y * y)) *
         std::exp(-chi / (2.0 * y) * std::pow(y / theta - 1.0, 2.0));
}

double inverse_gaussian_cdf_scalar_(double y, double theta, double chi) {
  if (y <= 0.0) return 0.0;
  const double root = std::sqrt(chi / y);
  return normal_cdf_scalar_(root * (y / theta - 1.0), 0.0, 1.0) +
         std::exp(2.0 * chi / theta) * normal_cdf_scalar_(root * (-y / theta - 1.0), 0.0, 1.0);
}

double sample_inverse_gaussian_scalar_(double theta, double chi, std::mt19937_64& rng) {
  std::normal_distribution<double> standard_normal(0.0, 1.0);
  std::uniform_real_distribution<double> uniform(0.0, 1.0);
  const double v = std::pow(standard_normal(rng), 2.0);
  const double y = theta + 0.5 * theta / chi *
                               (theta * v - std::sqrt(4.0 * theta * chi * v + theta * theta * v * v));
  return (uniform(rng) >= theta / (theta + y)) ? (theta * theta / y) : y;
}

double integrate_simpson_scalar_(const std::function<double(double)>& fn, double a, double b) {
  constexpr int n = 256;
  if (b <= a) return 0.0;
  const double h = (b - a) / static_cast<double>(n);
  double acc = fn(a) + fn(b);
  for (int i = 1; i < n; ++i) {
    const double x = a + h * static_cast<double>(i);
    acc += fn(x) * ((i % 2 == 0) ? 2.0 : 4.0);
  }
  return acc * h / 3.0;
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

Series beta_pdf_(const Series& x, double lo, double hi, double a, double b) {
  PLUSS_CHECK(hi >= lo, "subsp::stat::distr::beta pdf: expected hi >= lo");
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::beta pdf: expected a,b > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv < lo || xv > hi) return 0.0;
    if (hi == lo) return 0.0;
    const double t = (xv - lo) / (hi - lo);
    if (t <= 0.0 || t >= 1.0) return 0.0;
    return std::exp((a - 1.0) * std::log(t) + (b - 1.0) * std::log1p(-t) -
                    log_beta_scalar_(a, b) - std::log(hi - lo));
  });
}

Series beta_logpdf_(const Series& x, double lo, double hi, double a, double b) {
  PLUSS_CHECK(hi >= lo, "subsp::stat::distr::beta logpdf: expected hi >= lo");
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::beta logpdf: expected a,b > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv < lo || xv > hi || hi == lo) return -std::numeric_limits<double>::infinity();
    const double t = (xv - lo) / (hi - lo);
    if (t <= 0.0 || t >= 1.0) return -std::numeric_limits<double>::infinity();
    return (a - 1.0) * std::log(t) + (b - 1.0) * std::log1p(-t) - log_beta_scalar_(a, b) - std::log(hi - lo);
  });
}

Series beta_cdf_(const Series& x, double lo, double hi, double a, double b) {
  PLUSS_CHECK(hi >= lo, "subsp::stat::distr::beta cdf: expected hi >= lo");
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::beta cdf: expected a,b > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv <= lo) return 0.0;
    if (xv >= hi) return 1.0;
    if (hi == lo) return 1.0;
    return regularized_beta_scalar_(a, b, (xv - lo) / (hi - lo));
  });
}

Series beta_sample_(double lo, double hi, double a, double b, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(hi >= lo, "subsp::stat::distr::beta sample: expected hi >= lo");
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::beta sample: expected a,b > 0");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::gamma_distribution<double> ga(a, 1.0);
    std::gamma_distribution<double> gb(b, 1.0);
    const double x1 = ga(rng);
    const double x2 = gb(rng);
    const double t = x1 / (x1 + x2);
    return lo + (hi - lo) * t;
  });
}

Series gamma_pdf_(const Series& x, double m, double b, double a) {
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::gamma pdf: expected a,b > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv < m) return 0.0;
    const double z = (xv - m) / b;
    if (z <= 0.0) return 0.0;
    return std::exp((a - 1.0) * std::log(z) - z - std::lgamma(a) - std::log(b));
  });
}

Series gamma_logpdf_(const Series& x, double m, double b, double a) {
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::gamma logpdf: expected a,b > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv < m) return -std::numeric_limits<double>::infinity();
    const double z = (xv - m) / b;
    if (z <= 0.0) return -std::numeric_limits<double>::infinity();
    return (a - 1.0) * std::log(z) - z - std::lgamma(a) - std::log(b);
  });
}

Series gamma_cdf_(const Series& x, double m, double b, double a) {
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::gamma cdf: expected a,b > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv <= m) return 0.0;
    return regularized_gamma_p_scalar_(a, (xv - m) / b);
  });
}

Series gamma_sample_(double m, double b, double a, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::gamma sample: expected a,b > 0");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::gamma_distribution<double> dist(a, b);
    return m + dist(rng);
  });
}

double log_choose_scalar_(double n, double k) {
  return std::lgamma(n + 1.0) - std::lgamma(k + 1.0) - std::lgamma(n - k + 1.0);
}

Series poisson_pdf_(const Series& x, double lambda) {
  PLUSS_CHECK(lambda > 0.0, "subsp::stat::distr::poisson pdf: expected lambda > 0");
  return map_series_f64_(x, [=](double xv) {
    const double k = std::floor(xv);
    if (xv != k || k < 0.0) return 0.0;
    return std::exp(-lambda + k * std::log(lambda) - std::lgamma(k + 1.0));
  });
}

Series poisson_logpdf_(const Series& x, double lambda) {
  PLUSS_CHECK(lambda > 0.0, "subsp::stat::distr::poisson logpdf: expected lambda > 0");
  return map_series_f64_(x, [=](double xv) {
    const double k = std::floor(xv);
    if (xv != k || k < 0.0) return -std::numeric_limits<double>::infinity();
    return -lambda + k * std::log(lambda) - std::lgamma(k + 1.0);
  });
}

Series poisson_cdf_(const Series& x, double lambda) {
  PLUSS_CHECK(lambda > 0.0, "subsp::stat::distr::poisson cdf: expected lambda > 0");
  return map_series_f64_(x, [=](double xv) {
    const int k = static_cast<int>(std::floor(xv));
    if (k < 0) return 0.0;
    double pmf = std::exp(-lambda);
    double acc = pmf;
    for (int i = 1; i <= k; ++i) {
      pmf *= lambda / static_cast<double>(i);
      acc += pmf;
    }
    return std::min(1.0, acc);
  });
}

Series poisson_sample_(double lambda, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(lambda > 0.0, "subsp::stat::distr::poisson sample: expected lambda > 0");
  return sample_host_<std::int64_t>(sizes, cfg, ScalarType::Int64, seed, [=](std::mt19937_64& rng) {
    std::poisson_distribution<std::int64_t> dist(lambda);
    return dist(rng);
  });
}

Series binom_pdf_(const Series& x, std::int64_t n, double p) {
  PLUSS_CHECK(n > 0 && p > 0.0 && p < 1.0, "subsp::stat::distr::binomial pdf: invalid parameters");
  return map_series_f64_(x, [=](double xv) {
    const double k = std::floor(xv);
    if (xv != k || k < 0.0 || k > static_cast<double>(n)) return 0.0;
    return std::exp(log_choose_scalar_(static_cast<double>(n), k) + k * std::log(p) +
                    (static_cast<double>(n) - k) * std::log1p(-p));
  });
}

Series binom_logpdf_(const Series& x, std::int64_t n, double p) {
  PLUSS_CHECK(n > 0 && p > 0.0 && p < 1.0, "subsp::stat::distr::binomial logpdf: invalid parameters");
  return map_series_f64_(x, [=](double xv) {
    const double k = std::floor(xv);
    if (xv != k || k < 0.0 || k > static_cast<double>(n)) return -std::numeric_limits<double>::infinity();
    return log_choose_scalar_(static_cast<double>(n), k) + k * std::log(p) +
           (static_cast<double>(n) - k) * std::log1p(-p);
  });
}

Series binom_cdf_(const Series& x, std::int64_t n, double p) {
  PLUSS_CHECK(n > 0 && p > 0.0 && p < 1.0, "subsp::stat::distr::binomial cdf: invalid parameters");
  return map_series_f64_(x, [=](double xv) {
    const int k = static_cast<int>(std::floor(xv));
    if (k < 0) return 0.0;
    if (k >= n) return 1.0;
    double pmf = std::pow(1.0 - p, static_cast<double>(n));
    double acc = pmf;
    for (int i = 1; i <= k; ++i) {
      pmf *= (static_cast<double>(n - i + 1) / static_cast<double>(i)) * (p / (1.0 - p));
      acc += pmf;
    }
    return std::min(1.0, acc);
  });
}

Series binom_sample_(std::int64_t n, double p, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(n > 0 && p > 0.0 && p < 1.0, "subsp::stat::distr::binomial sample: invalid parameters");
  return sample_host_<std::int64_t>(sizes, cfg, ScalarType::Int64, seed, [=](std::mt19937_64& rng) {
    std::binomial_distribution<std::int64_t> dist(n, p);
    return dist(rng);
  });
}

Series chisq_pdf_(const Series& x, std::int64_t r) { return gamma_pdf_(x, 0.0, 2.0, 0.5 * static_cast<double>(r)); }
Series chisq_logpdf_(const Series& x, std::int64_t r) { return gamma_logpdf_(x, 0.0, 2.0, 0.5 * static_cast<double>(r)); }
Series chisq_cdf_(const Series& x, std::int64_t r) { return gamma_cdf_(x, 0.0, 2.0, 0.5 * static_cast<double>(r)); }
Series chisq_sample_(std::int64_t r, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  return gamma_sample_(0.0, 2.0, 0.5 * static_cast<double>(r), sizes, cfg, seed);
}

Series student_t_pdf_(const Series& x, double nu, double chi, double eta) {
  PLUSS_CHECK(nu > 0.0 && eta > 0.0, "subsp::stat::distr::t pdf: invalid parameters");
  const double log_norm = std::lgamma(0.5 * (nu + 1.0)) - std::lgamma(0.5 * nu) -
                          0.5 * std::log(nu * std::numbers::pi_v<double>) - std::log(eta);
  return map_series_f64_(x, [=](double xv) {
    const double z = (xv - chi) / eta;
    return std::exp(log_norm - 0.5 * (nu + 1.0) * std::log1p((z * z) / nu));
  });
}

Series student_t_logpdf_(const Series& x, double nu, double chi, double eta) {
  PLUSS_CHECK(nu > 0.0 && eta > 0.0, "subsp::stat::distr::t logpdf: invalid parameters");
  const double log_norm = std::lgamma(0.5 * (nu + 1.0)) - std::lgamma(0.5 * nu) -
                          0.5 * std::log(nu * std::numbers::pi_v<double>) - std::log(eta);
  return map_series_f64_(x, [=](double xv) {
    const double z = (xv - chi) / eta;
    return log_norm - 0.5 * (nu + 1.0) * std::log1p((z * z) / nu);
  });
}

Series student_t_cdf_(const Series& x, double nu, double chi, double eta) {
  PLUSS_CHECK(nu > 0.0 && eta > 0.0, "subsp::stat::distr::t cdf: invalid parameters");
  return map_series_f64_(x, [=](double xv) {
    const double z = (xv - chi) / eta;
    const double t = (z * z) / (nu + z * z);
    const double ib = regularized_beta_scalar_(0.5, 0.5 * nu, t);
    return 0.5 + ((z >= 0.0) ? 0.5 * ib : -0.5 * ib);
  });
}

Series student_t_sample_(double nu, double chi, double eta, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(nu > 0.0 && eta > 0.0, "subsp::stat::distr::t sample: invalid parameters");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::student_t_distribution<double> dist(nu);
    return chi + eta * dist(rng);
  });
}

Series triangular_pdf_(const Series& x, double a, double c, double b) {
  PLUSS_CHECK(a < c && c < b, "subsp::stat::distr::triangular pdf: expected a < c < b");
  return map_series_f64_(x, [=](double xv) {
    if (xv < a || xv > b) return 0.0;
    if (xv <= c) return 2.0 * (xv - a) / ((b - a) * (c - a));
    return 2.0 * (b - xv) / ((b - a) * (b - c));
  });
}

Series triangular_logpdf_(const Series& x, double a, double c, double b) {
  PLUSS_CHECK(a < c && c < b, "subsp::stat::distr::triangular logpdf: expected a < c < b");
  return map_series_f64_(x, [=](double xv) {
    const double pdf = (xv <= c)
                           ? ((xv < a || xv > b) ? 0.0 : 2.0 * (xv - a) / ((b - a) * (c - a)))
                           : ((xv < a || xv > b) ? 0.0 : 2.0 * (b - xv) / ((b - a) * (b - c)));
    return (pdf <= 0.0) ? -std::numeric_limits<double>::infinity() : std::log(pdf);
  });
}

Series triangular_cdf_(const Series& x, double a, double c, double b) {
  PLUSS_CHECK(a < c && c < b, "subsp::stat::distr::triangular cdf: expected a < c < b");
  return map_series_f64_(x, [=](double xv) {
    if (xv <= a) return 0.0;
    if (xv >= b) return 1.0;
    if (xv <= c) return ((xv - a) * (xv - a)) / ((b - a) * (c - a));
    return 1.0 - ((b - xv) * (b - xv)) / ((b - a) * (b - c));
  });
}

Series triangular_sample_(double a, double c, double b, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(a < c && c < b, "subsp::stat::distr::triangular sample: expected a < c < b");
  const double t = (c - a) / (b - a);
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    const double u = uniform(rng);
    return (u <= t) ? (a + std::sqrt(u * (b - a) * (c - a)))
                    : (b - std::sqrt((1.0 - u) * (b - a) * (b - c)));
  });
}

Series pareto_pdf_(const Series& x, double k, double a) {
  PLUSS_CHECK(k > 0.0 && a > 0.0, "subsp::stat::distr::pareto pdf: expected k,a > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv < k) return 0.0;
    return a * std::pow(k, a) / std::pow(xv, a + 1.0);
  });
}

Series pareto_logpdf_(const Series& x, double k, double a) {
  PLUSS_CHECK(k > 0.0 && a > 0.0, "subsp::stat::distr::pareto logpdf: expected k,a > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv < k) return -std::numeric_limits<double>::infinity();
    return std::log(a) + a * std::log(k) - (a + 1.0) * std::log(xv);
  });
}

Series pareto_cdf_(const Series& x, double k, double a) {
  PLUSS_CHECK(k > 0.0 && a > 0.0, "subsp::stat::distr::pareto cdf: expected k,a > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv < k) return 0.0;
    return 1.0 - std::pow(k / xv, a);
  });
}

Series pareto_sample_(double k, double a, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(k > 0.0 && a > 0.0, "subsp::stat::distr::pareto sample: expected k,a > 0");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    return k * std::pow(uniform(rng), -1.0 / a);
  });
}

Series lomax_pdf_(const Series& x, double k, double b) {
  PLUSS_CHECK(k > 0.0 && b > 0.0, "subsp::stat::distr::lomax pdf: expected k,b > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv < 0.0) return 0.0;
    return b * std::pow(k, b) / std::pow(k + xv, b + 1.0);
  });
}

Series lomax_logpdf_(const Series& x, double k, double b) {
  PLUSS_CHECK(k > 0.0 && b > 0.0, "subsp::stat::distr::lomax logpdf: expected k,b > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv < 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(b) + b * std::log(k) - (b + 1.0) * std::log(k + xv);
  });
}

Series lomax_cdf_(const Series& x, double k, double b) {
  PLUSS_CHECK(k > 0.0 && b > 0.0, "subsp::stat::distr::lomax cdf: expected k,b > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv < 0.0) return 0.0;
    return 1.0 - std::pow(k / (k + xv), b);
  });
}

Series lomax_sample_(double k, double b, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(k > 0.0 && b > 0.0, "subsp::stat::distr::lomax sample: expected k,b > 0");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    return k * (std::pow(uniform(rng), -1.0 / b) - 1.0);
  });
}

Series inverse_gaussian_pdf_(const Series& x, double theta, double chi) {
  PLUSS_CHECK(theta > 0.0 && chi > 0.0, "subsp::stat::distr::ig pdf: expected theta,chi > 0");
  return map_series_f64_(x, [=](double xv) { return inverse_gaussian_pdf_scalar_(xv, theta, chi); });
}

Series inverse_gaussian_logpdf_(const Series& x, double theta, double chi) {
  PLUSS_CHECK(theta > 0.0 && chi > 0.0, "subsp::stat::distr::ig logpdf: expected theta,chi > 0");
  return map_series_f64_(x, [=](double xv) {
    const double pdf = inverse_gaussian_pdf_scalar_(xv, theta, chi);
    return (pdf <= 0.0) ? -std::numeric_limits<double>::infinity() : std::log(pdf);
  });
}

Series inverse_gaussian_cdf_(const Series& x, double theta, double chi) {
  PLUSS_CHECK(theta > 0.0 && chi > 0.0, "subsp::stat::distr::ig cdf: expected theta,chi > 0");
  return map_series_f64_(x, [=](double xv) { return inverse_gaussian_cdf_scalar_(xv, theta, chi); });
}

Series inverse_gaussian_sample_(double theta, double chi, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(theta > 0.0 && chi > 0.0, "subsp::stat::distr::ig sample: expected theta,chi > 0");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    return sample_inverse_gaussian_scalar_(theta, chi, rng);
  });
}

Series normaltrunc_pdf_(const Series& x, double a, double b, double mu, double sigma) {
  PLUSS_CHECK(b >= a && sigma > 0.0, "subsp::stat::distr::normaltrunc pdf: invalid parameters");
  const double denom = normal_cdf_scalar_(b, mu, sigma) - normal_cdf_scalar_(a, mu, sigma);
  return map_series_f64_(x, [=](double xv) {
    if (xv < a || xv > b) return 0.0;
    return normal_pdf_scalar_(xv, mu, sigma) / denom;
  });
}

Series normaltrunc_logpdf_(const Series& x, double a, double b, double mu, double sigma) {
  PLUSS_CHECK(b >= a && sigma > 0.0, "subsp::stat::distr::normaltrunc logpdf: invalid parameters");
  const double log_denom = std::log(normal_cdf_scalar_(b, mu, sigma) - normal_cdf_scalar_(a, mu, sigma));
  return map_series_f64_(x, [=](double xv) {
    if (xv < a || xv > b) return -std::numeric_limits<double>::infinity();
    const double z = (xv - mu) / sigma;
    return -0.5 * z * z - std::log(sigma) - 0.5 * std::log(2.0 * std::numbers::pi_v<double>) - log_denom;
  });
}

Series normaltrunc_cdf_(const Series& x, double a, double b, double mu, double sigma) {
  PLUSS_CHECK(b >= a && sigma > 0.0, "subsp::stat::distr::normaltrunc cdf: invalid parameters");
  const double left = normal_cdf_scalar_(a, mu, sigma);
  const double denom = normal_cdf_scalar_(b, mu, sigma) - left;
  return map_series_f64_(x, [=](double xv) {
    if (xv <= a) return 0.0;
    if (xv >= b) return 1.0;
    return (normal_cdf_scalar_(xv, mu, sigma) - left) / denom;
  });
}

Series normaltrunc_sample_(double a, double b, double mu, double sigma, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(b >= a && sigma > 0.0, "subsp::stat::distr::normaltrunc sample: invalid parameters");
  const double left = normal_cdf_scalar_(a, mu, sigma);
  const double right = normal_cdf_scalar_(b, mu, sigma);
  const Series u = unit_uniform_(sizes, cfg, seed).mul(right - left).add(left);
  return u.mul(2.0).sub(1.0).erfinv().mul(kSqrt2 * sigma).add(mu).to(cfg.dtype);
}

double hypergeom_logpmf_scalar_(std::int64_t y, std::int64_t N, std::int64_t M, std::int64_t n) {
  const std::int64_t lo = std::max<std::int64_t>(0, n - N + M);
  const std::int64_t hi = std::min<std::int64_t>(n, M);
  if (y < lo || y > hi) return -std::numeric_limits<double>::infinity();
  return std::lgamma(M + 1.0) - std::lgamma(M - y + 1.0) - std::lgamma(y + 1.0) +
         std::lgamma(N - M + 1.0) - std::lgamma(N - M - n + y + 1.0) - std::lgamma(n - y + 1.0) +
         std::lgamma(n + 1.0) + std::lgamma(N - n + 1.0) - std::lgamma(N + 1.0);
}

Series hypergeom_pdf_(const Series& x, std::int64_t N, std::int64_t M, std::int64_t n) {
  return map_series_f64_(x, [=](double xv) {
    const auto y = static_cast<std::int64_t>(std::floor(xv));
    if (xv != static_cast<double>(y)) return 0.0;
    const double lp = hypergeom_logpmf_scalar_(y, N, M, n);
    return std::isfinite(lp) ? std::exp(lp) : 0.0;
  });
}

Series hypergeom_logpdf_(const Series& x, std::int64_t N, std::int64_t M, std::int64_t n) {
  return map_series_f64_(x, [=](double xv) {
    const auto y = static_cast<std::int64_t>(std::floor(xv));
    return (xv != static_cast<double>(y)) ? -std::numeric_limits<double>::infinity() : hypergeom_logpmf_scalar_(y, N, M, n);
  });
}

Series hypergeom_cdf_(const Series& x, std::int64_t N, std::int64_t M, std::int64_t n) {
  return map_series_f64_(x, [=](double xv) {
    const auto k = static_cast<std::int64_t>(std::floor(xv));
    const auto lo = std::max<std::int64_t>(0, n - N + M);
    if (k < lo) return 0.0;
    const auto hi = std::min<std::int64_t>(n, M);
    if (k >= hi) return 1.0;
    double acc = 0.0;
    for (std::int64_t y = lo; y <= k; ++y) acc += std::exp(hypergeom_logpmf_scalar_(y, N, M, n));
    return std::min(1.0, acc);
  });
}

Series hypergeom_sample_(std::int64_t N, std::int64_t M, std::int64_t n, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(N > 1 && M > 0 && M < N && n > 0 && n < N, "subsp::stat::distr::hypergeom sample: invalid parameters");
  return sample_host_<std::int64_t>(sizes, cfg, ScalarType::Int64, seed, [=](std::mt19937_64& rng) {
    std::int64_t remaining_success = M;
    std::int64_t remaining_total = N;
    std::int64_t out = 0;
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    for (std::int64_t draw = 0; draw < n; ++draw) {
      const double p = static_cast<double>(remaining_success) / static_cast<double>(remaining_total);
      if (uniform(rng) < p) {
        ++out;
        --remaining_success;
      }
      --remaining_total;
    }
    return out;
  });
}

double skellam_pmf_scalar_(std::int64_t n, double lambda1, double lambda2) {
  const double arg = 2.0 * std::sqrt(lambda1 * lambda2);
  const double log_term = -lambda1 - lambda2 + 0.5 * static_cast<double>(n) * std::log(lambda1 / lambda2);
  return std::exp(log_term) * bessel_i_series_scalar_(static_cast<double>(std::abs(n)), arg);
}

Series skellam_pdf_(const Series& x, double lambda1, double lambda2) {
  return map_series_f64_(x, [=](double xv) {
    const auto n = static_cast<std::int64_t>(std::floor(xv));
    return (xv != static_cast<double>(n)) ? 0.0 : skellam_pmf_scalar_(n, lambda1, lambda2);
  });
}

Series skellam_logpdf_(const Series& x, double lambda1, double lambda2) {
  return map_series_f64_(x, [=](double xv) {
    const auto n = static_cast<std::int64_t>(std::floor(xv));
    if (xv != static_cast<double>(n)) return -std::numeric_limits<double>::infinity();
    const double pmf = skellam_pmf_scalar_(n, lambda1, lambda2);
    return (pmf <= 0.0) ? -std::numeric_limits<double>::infinity() : std::log(pmf);
  });
}

Series skellam_cdf_(const Series& x, double lambda1, double lambda2) {
  return map_series_f64_(x, [=](double xv) {
    const auto k = static_cast<std::int64_t>(std::floor(xv));
    const double mean = lambda1 - lambda2;
    const double sd = std::sqrt(lambda1 + lambda2);
    const std::int64_t lo = static_cast<std::int64_t>(std::floor(mean - 12.0 * sd - 20.0));
    const std::int64_t hi = static_cast<std::int64_t>(std::ceil(mean + 12.0 * sd + 20.0));
    if (k < lo) return 0.0;
    if (k >= hi) return 1.0;
    double acc = 0.0;
    for (std::int64_t n = lo; n <= k; ++n) acc += skellam_pmf_scalar_(n, lambda1, lambda2);
    return std::clamp(acc, 0.0, 1.0);
  });
}

Series skellam_sample_(double lambda1, double lambda2, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(lambda1 > 0.0 && lambda2 > 0.0, "subsp::stat::distr::skellam sample: invalid parameters");
  return sample_host_<std::int64_t>(sizes, cfg, ScalarType::Int64, seed, [=](std::mt19937_64& rng) {
    std::poisson_distribution<std::int64_t> d1(lambda1);
    std::poisson_distribution<std::int64_t> d2(lambda2);
    return d1(rng) - d2(rng);
  });
}

Series zeta_pdf_(const Series& x, double a) {
  PLUSS_CHECK(a > 1.0, "subsp::stat::distr::zeta pdf: expected a > 1");
  const double zeta_a = zeta_scalar_(a);
  return map_series_f64_(x, [=](double xv) {
    const auto n = static_cast<std::int64_t>(std::floor(xv));
    if (xv != static_cast<double>(n) || n < 1) return 0.0;
    return 1.0 / (std::pow(static_cast<double>(n), a) * zeta_a);
  });
}

Series zeta_logpdf_(const Series& x, double a) {
  PLUSS_CHECK(a > 1.0, "subsp::stat::distr::zeta logpdf: expected a > 1");
  const double log_zeta_a = std::log(zeta_scalar_(a));
  return map_series_f64_(x, [=](double xv) {
    const auto n = static_cast<std::int64_t>(std::floor(xv));
    if (xv != static_cast<double>(n) || n < 1) return -std::numeric_limits<double>::infinity();
    return -a * std::log(static_cast<double>(n)) - log_zeta_a;
  });
}

Series zeta_cdf_(const Series& x, double a) {
  PLUSS_CHECK(a > 1.0, "subsp::stat::distr::zeta cdf: expected a > 1");
  const double zeta_a = zeta_scalar_(a);
  return map_series_f64_(x, [=](double xv) {
    const auto k = static_cast<std::int64_t>(std::floor(xv));
    if (k < 1) return 0.0;
    double acc = 0.0;
    for (std::int64_t n = 1; n <= k; ++n) acc += std::pow(static_cast<double>(n), -a);
    return std::min(1.0, acc / zeta_a);
  });
}

Series zeta_sample_(double a, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(a > 1.0, "subsp::stat::distr::zeta sample: expected a > 1");
  return sample_host_<std::int64_t>(sizes, cfg, ScalarType::Int64, seed, [=](std::mt19937_64& rng) {
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    const double b = std::pow(2.0, a - 1.0);
    const double am1 = a - 1.0;
    const double bm1 = b - 1.0;
    while (true) {
      const double u1 = uniform(rng);
      const std::int64_t out = static_cast<std::int64_t>(std::floor(std::pow(u1, -1.0 / am1)));
      const double u2 = uniform(rng);
      const double t = std::pow(1.0 + 1.0 / static_cast<double>(out), a - 1.0);
      if (u2 * static_cast<double>(out) * (t - 1.0) / bm1 <= t / b) return out;
    }
  });
}

double gig_logpdf_scalar_(double y, double lambda, double chi, double psi) {
  if (y <= 0.0) return -std::numeric_limits<double>::infinity();
  if (chi == 0.0) {
    return (lambda - 1.0) * std::log(y) - 0.5 * psi * y - lambda * std::log(2.0 / psi) - std::lgamma(lambda);
  }
  if (psi == 0.0) {
    return (-lambda - 1.0) * std::log(y) - 0.5 * chi / y + lambda * std::log(2.0 / chi) - std::lgamma(-lambda);
  }
  const double z = std::sqrt(chi * psi);
  return 0.5 * lambda * std::log(psi / chi) + (lambda - 1.0) * std::log(y) -
         std::log(2.0) - std::log(bessel_k_series_scalar_(lambda, z)) -
         0.5 * (chi / y + psi * y);
}

double sample_gig_scalar_(double lambda, double chi, double psi, std::mt19937_64& rng) {
  if (chi == 0.0) {
    std::gamma_distribution<double> dist(lambda, 2.0 / psi);
    return dist(rng);
  }
  if (psi == 0.0) {
    std::gamma_distribution<double> dist(-lambda, 2.0 / chi);
    return 1.0 / dist(rng);
  }
  if (std::abs(lambda + 0.5) < 1e-12) {
    return sample_inverse_gaussian_scalar_(std::sqrt(chi / psi), chi, rng);
  }

  const double h = lambda;
  const double b = std::sqrt(chi * psi);
  const double m = (h - 1.0 + std::sqrt((h - 1.0) * (h - 1.0) + b * b)) / b;
  const double log_1_over_pm = -(h - 1.0) * 0.5 * std::log(m) + b * 0.25 * (m + 1.0 / m);
  const double r = (6.0 * m + 2.0 * h * m - b * m * m + b) / (4.0 * m * m);
  const double s = (1.0 + h - b * m) / (2.0 * m * m);
  const double p = (3.0 * s - r * r) / 3.0;
  const double q = (2.0 * r * r * r) / 27.0 - (r * s) / 27.0 - b / (4.0 * m * m);
  const double eta = std::sqrt(-(p * p * p) / 27.0);
  const double acos_arg = std::clamp(-q / (2.0 * eta), -1.0, 1.0);
  const double root = 2.0 * std::exp(std::log(eta) / 3.0);
  const double y1 = root * std::cos(std::acos(acos_arg) / 3.0) - r / 3.0;
  const double y2 = root * std::cos(std::acos(acos_arg) / 3.0 + 2.0 * std::numbers::pi_v<double> / 3.0) - r / 3.0;
  std::uniform_real_distribution<double> uniform(0.0, 1.0);

  double out = 0.0;
  if (((h <= 1.0) && (b <= 1.0)) || std::abs(q / eta) > 2.0 || y1 < 0.0 || y2 > 0.0) {
    const double ym = (-h - 1.0 + std::sqrt((h + 1.0) * (h + 1.0) + b * b)) / b;
    const double a = std::exp(-0.5 * h * std::log(m * ym) + 0.5 * std::log(m / ym) +
                              b * 0.25 * (m + 1.0 / m - ym - 1.0 / ym));
    while (true) {
      const double u = uniform(rng);
      const double v = uniform(rng);
      out = a * (v / u);
      if (std::log(u) <= (h - 1.0) * 0.5 * std::log(out) - b * 0.25 * (out + 1.0 / out) + log_1_over_pm) {
        break;
      }
    }
  } else {
    const double vplus = std::exp(log_1_over_pm + std::log(1.0 / y1) + (h - 1.0) * 0.5 * std::log(1.0 / y1 + m) -
                                  b * 0.25 * (1.0 / y1 + m + 1.0 / (1.0 / y1 + m)));
    const double vminus = -std::exp(log_1_over_pm + std::log(-1.0 / y2) + (h - 1.0) * 0.5 * std::log(1.0 / y2 + m) -
                                    b * 0.25 * (1.0 / y2 + m + 1.0 / (1.0 / y2 + m)));
    while (true) {
      const double u = uniform(rng);
      const double v = vminus + (vplus - vminus) * uniform(rng);
      const double z = v / u;
      if (z < -m) continue;
      out = z + m;
      if (std::log(u) <= (log_1_over_pm + (h - 1.0) * 0.5 * std::log(out) - b * 0.25 * (out + 1.0 / out))) {
        break;
      }
    }
  }
  return std::sqrt(chi / psi) * out;
}

Series gig_pdf_(const Series& x, double lambda, double chi, double psi) {
  return map_series_f64_(x, [=](double xv) {
    const double lp = gig_logpdf_scalar_(xv, lambda, chi, psi);
    return std::isfinite(lp) ? std::exp(lp) : 0.0;
  });
}

Series gig_logpdf_(const Series& x, double lambda, double chi, double psi) {
  return map_series_f64_(x, [=](double xv) { return gig_logpdf_scalar_(xv, lambda, chi, psi); });
}

Series gig_cdf_(const Series& x, double lambda, double chi, double psi) {
  return map_series_f64_(x, [=](double xv) {
    if (xv <= 0.0) return 0.0;
    const auto pdf = [=](double y) { return std::exp(gig_logpdf_scalar_(y, lambda, chi, psi)); };
    return std::clamp(integrate_simpson_scalar_(pdf, 1e-8, xv), 0.0, 1.0);
  });
}

Series gig_sample_(double lambda, double chi, double psi, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    return sample_gig_scalar_(lambda, chi, psi, rng);
  });
}

double gh_logpdf_scalar_(double y, double lambda, double alpha, double beta, double mu, double delta) {
  const double gamma = std::sqrt(alpha * alpha - beta * beta);
  const double zeta = delta * gamma;
  const double r = std::sqrt(delta * delta + (y - mu) * (y - mu));
  return 0.5 * lambda * std::log(alpha * alpha - beta * beta) -
         0.5 * std::log(2.0 * std::numbers::pi_v<double>) -
         (lambda - 0.5) * std::log(alpha) -
         lambda * std::log(delta) -
         std::log(bessel_k_series_scalar_(lambda, zeta)) +
         0.5 * (lambda - 0.5) * std::log(delta * delta + (y - mu) * (y - mu)) +
         std::log(bessel_k_series_scalar_(lambda - 0.5, alpha * r)) +
         beta * (y - mu);
}

Series gh_pdf_(const Series& x, double lambda, double alpha, double beta, double mu, double delta) {
  return map_series_f64_(x, [=](double xv) {
    const double lp = gh_logpdf_scalar_(xv, lambda, alpha, beta, mu, delta);
    return std::isfinite(lp) ? std::exp(lp) : 0.0;
  });
}

Series gh_logpdf_(const Series& x, double lambda, double alpha, double beta, double mu, double delta) {
  return map_series_f64_(x, [=](double xv) { return gh_logpdf_scalar_(xv, lambda, alpha, beta, mu, delta); });
}

Series gh_cdf_(const Series& x, double lambda, double alpha, double beta, double mu, double delta) {
  return map_series_f64_(x, [=](double xv) {
    const double scale = std::max(delta, 10.0 / std::max(1e-6, alpha - std::abs(beta)));
    const double lo = mu - 20.0 * scale;
    const auto pdf = [=](double y) { return std::exp(gh_logpdf_scalar_(y, lambda, alpha, beta, mu, delta)); };
    if (xv <= lo) return 0.0;
    const double v = integrate_simpson_scalar_(pdf, lo, xv);
    return std::clamp(v, 0.0, 1.0);
  });
}

Series gh_sample_(double lambda, double alpha, double beta, double mu, double delta, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(alpha > 0.0 && delta > 0.0 && alpha > std::abs(beta),
              "subsp::stat::distr::gh sample: invalid parameters");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::normal_distribution<double> normal(0.0, 1.0);
    const double w = sample_gig_scalar_(lambda, delta * delta, alpha * alpha - beta * beta, rng);
    return mu + beta * w + std::sqrt(w) * normal(rng);
  });
}

Series nig_pdf_(const Series& x, double alpha, double beta, double mu, double delta) {
  return gh_pdf_(x, -0.5, alpha, beta, mu, delta);
}

Series nig_logpdf_(const Series& x, double alpha, double beta, double mu, double delta) {
  return gh_logpdf_(x, -0.5, alpha, beta, mu, delta);
}

Series nig_cdf_(const Series& x, double alpha, double beta, double mu, double delta) {
  return gh_cdf_(x, -0.5, alpha, beta, mu, delta);
}

Series nig_sample_(double alpha, double beta, double mu, double delta, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(alpha > 0.0 && delta > 0.0 && alpha > std::abs(beta),
              "subsp::stat::distr::nig sample: invalid parameters");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::normal_distribution<double> normal(0.0, 1.0);
    const double w = sample_inverse_gaussian_scalar_(delta / std::sqrt(alpha * alpha - beta * beta), delta * delta, rng);
    return mu + beta * w + std::sqrt(w) * normal(rng);
  });
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
    case Family::Beta:
      expect_param_count_("beta sample", params, {2, 4});
      return (params.size() == 2)
                 ? beta_sample_(0.0, 1.0, params[0], params[1], sizes, cfg, seed)
                 : beta_sample_(params[0], params[1], params[2], params[3], sizes, cfg, seed);
    case Family::Binomial:
      expect_param_count_("binomial sample", params, {2});
      return binom_sample_(static_cast<std::int64_t>(params[0]), params[1], sizes, cfg, seed);
    case Family::Cauchy:
      expect_param_count_("cauchy sample", params, {0, 2});
      return cauchy_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, 1.0), sizes, cfg, seed);
    case Family::ChiSquare:
      expect_param_count_("chisquare sample", params, {1});
      return chisq_sample_(static_cast<std::int64_t>(params[0]), sizes, cfg, seed);
    case Family::Laplace:
      expect_param_count_("laplace sample", params, {0, 2});
      return laplace_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, 1.0), sizes, cfg, seed);
    case Family::Logistic:
      expect_param_count_("logistic sample", params, {0, 2});
      return logistic_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, 1.0), sizes, cfg, seed);
    case Family::Geometric:
      expect_param_count_("geometric sample", params, {0, 1});
      return geometric_sample_(param_or_(params, 0, 0.5), sizes, cfg, seed);
    case Family::Gamma:
      expect_param_count_("gamma sample", params, {1, 3});
      return (params.size() == 1)
                 ? gamma_sample_(0.0, 1.0, params[0], sizes, cfg, seed)
                 : gamma_sample_(params[0], params[1], params[2], sizes, cfg, seed);
    case Family::Hypergeometric:
      expect_param_count_("hypergeom sample", params, {3});
      return hypergeom_sample_(
          static_cast<std::int64_t>(params[0]), static_cast<std::int64_t>(params[1]), static_cast<std::int64_t>(params[2]), sizes, cfg, seed);
    case Family::InverseGaussian:
      expect_param_count_("ig sample", params, {2});
      return inverse_gaussian_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::NormalTrunc:
      expect_param_count_("normaltrunc sample", params, {4});
      return normaltrunc_sample_(params[0], params[1], params[2], params[3], sizes, cfg, seed);
    case Family::Poisson:
      expect_param_count_("poisson sample", params, {1});
      return poisson_sample_(params[0], sizes, cfg, seed);
    case Family::StudentT:
      expect_param_count_("t sample", params, {1, 3});
      return (params.size() == 1)
                 ? student_t_sample_(params[0], 0.0, 1.0, sizes, cfg, seed)
                 : student_t_sample_(params[0], params[1], params[2], sizes, cfg, seed);
    case Family::Triangular:
      expect_param_count_("triangular sample", params, {3});
      return triangular_sample_(params[0], params[1], params[2], sizes, cfg, seed);
    case Family::Pareto:
      expect_param_count_("pareto sample", params, {2});
      return pareto_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::Lomax:
      expect_param_count_("lomax sample", params, {2});
      return lomax_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::Rayleigh:
      expect_param_count_("rayleigh sample", params, {0, 1});
      return rayleigh_sample_(param_or_(params, 0, 1.0), sizes, cfg, seed);
    case Family::Skellam:
      expect_param_count_("skellam sample", params, {2});
      return skellam_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::Wald:
      expect_param_count_("wald sample", params, {1});
      return inverse_gaussian_sample_(1.0, params[0], sizes, cfg, seed);
    case Family::Weibull:
      expect_param_count_("weibull sample", params, {2});
      return weibull_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::Zeta:
      expect_param_count_("zeta sample", params, {1});
      return zeta_sample_(params[0], sizes, cfg, seed);
    case Family::Gig:
      expect_param_count_("gig sample", params, {3});
      return gig_sample_(params[0], params[1], params[2], sizes, cfg, seed);
    case Family::Gh:
      expect_param_count_("gh sample", params, {5});
      return gh_sample_(params[0], params[1], params[2], params[3], params[4], sizes, cfg, seed);
    case Family::Nig:
      expect_param_count_("nig sample", params, {4});
      return nig_sample_(params[0], params[1], params[2], params[3], sizes, cfg, seed);
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
    case Family::Beta:
      expect_param_count_("beta pdf", params, {2, 4});
      return (params.size() == 2)
                 ? beta_pdf_(x, 0.0, 1.0, params[0], params[1])
                 : beta_pdf_(x, params[0], params[1], params[2], params[3]);
    case Family::Binomial:
      expect_param_count_("binomial pdf", params, {2});
      return binom_pdf_(x, static_cast<std::int64_t>(params[0]), params[1]);
    case Family::Cauchy:
      expect_param_count_("cauchy pdf", params, {0, 2});
      return cauchy_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::ChiSquare:
      expect_param_count_("chisquare pdf", params, {1});
      return chisq_pdf_(x, static_cast<std::int64_t>(params[0]));
    case Family::Laplace:
      expect_param_count_("laplace pdf", params, {0, 2});
      return laplace_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Logistic:
      expect_param_count_("logistic pdf", params, {0, 2});
      return logistic_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Geometric:
      expect_param_count_("geometric pdf", params, {0, 1});
      return geometric_pdf_(x, param_or_(params, 0, 0.5));
    case Family::Gamma:
      expect_param_count_("gamma pdf", params, {1, 3});
      return (params.size() == 1) ? gamma_pdf_(x, 0.0, 1.0, params[0]) : gamma_pdf_(x, params[0], params[1], params[2]);
    case Family::Hypergeometric:
      expect_param_count_("hypergeom pdf", params, {3});
      return hypergeom_pdf_(x, static_cast<std::int64_t>(params[0]), static_cast<std::int64_t>(params[1]), static_cast<std::int64_t>(params[2]));
    case Family::InverseGaussian:
      expect_param_count_("ig pdf", params, {2});
      return inverse_gaussian_pdf_(x, params[0], params[1]);
    case Family::NormalTrunc:
      expect_param_count_("normaltrunc pdf", params, {4});
      return normaltrunc_pdf_(x, params[0], params[1], params[2], params[3]);
    case Family::Poisson:
      expect_param_count_("poisson pdf", params, {1});
      return poisson_pdf_(x, params[0]);
    case Family::StudentT:
      expect_param_count_("t pdf", params, {1, 3});
      return (params.size() == 1) ? student_t_pdf_(x, params[0], 0.0, 1.0) : student_t_pdf_(x, params[0], params[1], params[2]);
    case Family::Triangular:
      expect_param_count_("triangular pdf", params, {3});
      return triangular_pdf_(x, params[0], params[1], params[2]);
    case Family::Pareto:
      expect_param_count_("pareto pdf", params, {2});
      return pareto_pdf_(x, params[0], params[1]);
    case Family::Lomax:
      expect_param_count_("lomax pdf", params, {2});
      return lomax_pdf_(x, params[0], params[1]);
    case Family::Rayleigh:
      expect_param_count_("rayleigh pdf", params, {0, 1});
      return rayleigh_pdf_(x, param_or_(params, 0, 1.0));
    case Family::Skellam:
      expect_param_count_("skellam pdf", params, {2});
      return skellam_pdf_(x, params[0], params[1]);
    case Family::Wald:
      expect_param_count_("wald pdf", params, {1});
      return inverse_gaussian_pdf_(x, 1.0, params[0]);
    case Family::Weibull:
      expect_param_count_("weibull pdf", params, {2});
      return weibull_pdf_(x, params[0], params[1]);
    case Family::Zeta:
      expect_param_count_("zeta pdf", params, {1});
      return zeta_pdf_(x, params[0]);
    case Family::Gig:
      expect_param_count_("gig pdf", params, {3});
      return gig_pdf_(x, params[0], params[1], params[2]);
    case Family::Gh:
      expect_param_count_("gh pdf", params, {5});
      return gh_pdf_(x, params[0], params[1], params[2], params[3], params[4]);
    case Family::Nig:
      expect_param_count_("nig pdf", params, {4});
      return nig_pdf_(x, params[0], params[1], params[2], params[3]);
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
    case Family::Beta:
      expect_param_count_("beta logpdf", params, {2, 4});
      return (params.size() == 2)
                 ? beta_logpdf_(x, 0.0, 1.0, params[0], params[1])
                 : beta_logpdf_(x, params[0], params[1], params[2], params[3]);
    case Family::Binomial:
      expect_param_count_("binomial logpdf", params, {2});
      return binom_logpdf_(x, static_cast<std::int64_t>(params[0]), params[1]);
    case Family::Cauchy:
      expect_param_count_("cauchy logpdf", params, {0, 2});
      return cauchy_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::ChiSquare:
      expect_param_count_("chisquare logpdf", params, {1});
      return chisq_logpdf_(x, static_cast<std::int64_t>(params[0]));
    case Family::Laplace:
      expect_param_count_("laplace logpdf", params, {0, 2});
      return laplace_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Logistic:
      expect_param_count_("logistic logpdf", params, {0, 2});
      return logistic_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Geometric:
      expect_param_count_("geometric logpdf", params, {0, 1});
      return geometric_logpdf_(x, param_or_(params, 0, 0.5));
    case Family::Gamma:
      expect_param_count_("gamma logpdf", params, {1, 3});
      return (params.size() == 1) ? gamma_logpdf_(x, 0.0, 1.0, params[0]) : gamma_logpdf_(x, params[0], params[1], params[2]);
    case Family::Hypergeometric:
      expect_param_count_("hypergeom logpdf", params, {3});
      return hypergeom_logpdf_(x, static_cast<std::int64_t>(params[0]), static_cast<std::int64_t>(params[1]), static_cast<std::int64_t>(params[2]));
    case Family::InverseGaussian:
      expect_param_count_("ig logpdf", params, {2});
      return inverse_gaussian_logpdf_(x, params[0], params[1]);
    case Family::NormalTrunc:
      expect_param_count_("normaltrunc logpdf", params, {4});
      return normaltrunc_logpdf_(x, params[0], params[1], params[2], params[3]);
    case Family::Poisson:
      expect_param_count_("poisson logpdf", params, {1});
      return poisson_logpdf_(x, params[0]);
    case Family::StudentT:
      expect_param_count_("t logpdf", params, {1, 3});
      return (params.size() == 1) ? student_t_logpdf_(x, params[0], 0.0, 1.0) : student_t_logpdf_(x, params[0], params[1], params[2]);
    case Family::Triangular:
      expect_param_count_("triangular logpdf", params, {3});
      return triangular_logpdf_(x, params[0], params[1], params[2]);
    case Family::Pareto:
      expect_param_count_("pareto logpdf", params, {2});
      return pareto_logpdf_(x, params[0], params[1]);
    case Family::Lomax:
      expect_param_count_("lomax logpdf", params, {2});
      return lomax_logpdf_(x, params[0], params[1]);
    case Family::Rayleigh:
      expect_param_count_("rayleigh logpdf", params, {0, 1});
      return rayleigh_logpdf_(x, param_or_(params, 0, 1.0));
    case Family::Skellam:
      expect_param_count_("skellam logpdf", params, {2});
      return skellam_logpdf_(x, params[0], params[1]);
    case Family::Wald:
      expect_param_count_("wald logpdf", params, {1});
      return inverse_gaussian_logpdf_(x, 1.0, params[0]);
    case Family::Weibull:
      expect_param_count_("weibull logpdf", params, {2});
      return weibull_logpdf_(x, params[0], params[1]);
    case Family::Zeta:
      expect_param_count_("zeta logpdf", params, {1});
      return zeta_logpdf_(x, params[0]);
    case Family::Gig:
      expect_param_count_("gig logpdf", params, {3});
      return gig_logpdf_(x, params[0], params[1], params[2]);
    case Family::Gh:
      expect_param_count_("gh logpdf", params, {5});
      return gh_logpdf_(x, params[0], params[1], params[2], params[3], params[4]);
    case Family::Nig:
      expect_param_count_("nig logpdf", params, {4});
      return nig_logpdf_(x, params[0], params[1], params[2], params[3]);
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
    case Family::Beta:
      expect_param_count_("beta cdf", params, {2, 4});
      return (params.size() == 2)
                 ? beta_cdf_(x, 0.0, 1.0, params[0], params[1])
                 : beta_cdf_(x, params[0], params[1], params[2], params[3]);
    case Family::Binomial:
      expect_param_count_("binomial cdf", params, {2});
      return binom_cdf_(x, static_cast<std::int64_t>(params[0]), params[1]);
    case Family::Cauchy:
      expect_param_count_("cauchy cdf", params, {0, 2});
      return cauchy_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::ChiSquare:
      expect_param_count_("chisquare cdf", params, {1});
      return chisq_cdf_(x, static_cast<std::int64_t>(params[0]));
    case Family::Laplace:
      expect_param_count_("laplace cdf", params, {0, 2});
      return laplace_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Logistic:
      expect_param_count_("logistic cdf", params, {0, 2});
      return logistic_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Geometric:
      expect_param_count_("geometric cdf", params, {0, 1});
      return geometric_cdf_(x, param_or_(params, 0, 0.5));
    case Family::Gamma:
      expect_param_count_("gamma cdf", params, {1, 3});
      return (params.size() == 1) ? gamma_cdf_(x, 0.0, 1.0, params[0]) : gamma_cdf_(x, params[0], params[1], params[2]);
    case Family::Hypergeometric:
      expect_param_count_("hypergeom cdf", params, {3});
      return hypergeom_cdf_(x, static_cast<std::int64_t>(params[0]), static_cast<std::int64_t>(params[1]), static_cast<std::int64_t>(params[2]));
    case Family::InverseGaussian:
      expect_param_count_("ig cdf", params, {2});
      return inverse_gaussian_cdf_(x, params[0], params[1]);
    case Family::NormalTrunc:
      expect_param_count_("normaltrunc cdf", params, {4});
      return normaltrunc_cdf_(x, params[0], params[1], params[2], params[3]);
    case Family::Poisson:
      expect_param_count_("poisson cdf", params, {1});
      return poisson_cdf_(x, params[0]);
    case Family::StudentT:
      expect_param_count_("t cdf", params, {1, 3});
      return (params.size() == 1) ? student_t_cdf_(x, params[0], 0.0, 1.0) : student_t_cdf_(x, params[0], params[1], params[2]);
    case Family::Triangular:
      expect_param_count_("triangular cdf", params, {3});
      return triangular_cdf_(x, params[0], params[1], params[2]);
    case Family::Pareto:
      expect_param_count_("pareto cdf", params, {2});
      return pareto_cdf_(x, params[0], params[1]);
    case Family::Lomax:
      expect_param_count_("lomax cdf", params, {2});
      return lomax_cdf_(x, params[0], params[1]);
    case Family::Rayleigh:
      expect_param_count_("rayleigh cdf", params, {0, 1});
      return rayleigh_cdf_(x, param_or_(params, 0, 1.0));
    case Family::Skellam:
      expect_param_count_("skellam cdf", params, {2});
      return skellam_cdf_(x, params[0], params[1]);
    case Family::Wald:
      expect_param_count_("wald cdf", params, {1});
      return inverse_gaussian_cdf_(x, 1.0, params[0]);
    case Family::Weibull:
      expect_param_count_("weibull cdf", params, {2});
      return weibull_cdf_(x, params[0], params[1]);
    case Family::Zeta:
      expect_param_count_("zeta cdf", params, {1});
      return zeta_cdf_(x, params[0]);
    case Family::Gig:
      expect_param_count_("gig cdf", params, {3});
      return gig_cdf_(x, params[0], params[1], params[2]);
    case Family::Gh:
      expect_param_count_("gh cdf", params, {5});
      return gh_cdf_(x, params[0], params[1], params[2], params[3], params[4]);
    case Family::Nig:
      expect_param_count_("nig cdf", params, {4});
      return nig_cdf_(x, params[0], params[1], params[2], params[3]);
  }

  return Series::undefined();
}

Series cdf(std::string_view name, const Series& x, std::initializer_list<double> params) {
  return cdf(name, x, std::span<const double>(params.begin(), params.size()));
}

} // namespace pluss::subsp::stat::distr
