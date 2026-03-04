#include "pluss/subsp/stat/distr.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/codem.h"
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
  Alpha,
  Anglit,
  Arcsin,
  Bessel,
  Normal,
  Lognormal,
  Uniform,
  Exponential,
  Bernoulli,
  Beta,
  Binomial,
  Bradford,
  Burr,
  Cauchy,
  Chi,
  ChiSquare,
  ChiSquareNoncentral,
  Cosine,
  Erlang,
  ExtremeValue,
  F,
  FNoncentral,
  Laplace,
  Logistic,
  Geometric,
  Gamma,
  Gompertz,
  HalfCosine,
  HyperbolicSecant,
  Hypergeometric,
  InverseGaussian,
  Maxwell,
  Nakagami,
  NegativeBinomial,
  NormalTrunc,
  Planck,
  Poisson,
  Quadratic,
  Rademacher,
  StudentT,
  Triangular,
  Pareto,
  Lomax,
  Rayleigh,
  Rice,
  Semicircle,
  Skellam,
  TukeyLambda,
  UShape,
  VonMises,
  Wald,
  Weibull,
  Yule,
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

  if (key == "alpha") return Family::Alpha;
  if (key == "anglit") return Family::Anglit;
  if (key == "arcsin") return Family::Arcsin;
  if (key == "bessel") return Family::Bessel;
  if (key == "norm" || key == "normal" || key == "gaussian" || key == "gauss") return Family::Normal;
  if (key == "lognorm" || key == "lognormal" || key == "cobbdouglas" || key == "antilognormal") return Family::Lognormal;
  if (key == "uniform" || key == "unif") return Family::Uniform;
  if (key == "exp" || key == "exponential") return Family::Exponential;
  if (key == "bern" || key == "bernoulli") return Family::Bernoulli;
  if (key == "beta" || key == "powerfunction" || key == "powerfunc") return Family::Beta;
  if (key == "binom" || key == "binomial") return Family::Binomial;
  if (key == "bradford") return Family::Bradford;
  if (key == "burr" || key == "fisk") return Family::Burr;
  if (key == "cauchy" || key == "lorentz" || key == "caushy") return Family::Cauchy;
  if (key == "chi") return Family::Chi;
  if (key == "chisquare" || key == "chisq" || key == "chi2") return Family::ChiSquare;
  if (key == "chisqnc" || key == "chisqnoncentral" || key == "chisqnoncentr" || key == "chi2noncentral") return Family::ChiSquareNoncentral;
  if (key == "cosine") return Family::Cosine;
  if (key == "erlang") return Family::Erlang;
  if (key == "extrval" || key == "extremevalue" || key == "extrvalue" || key == "gumbel") return Family::ExtremeValue;
  if (key == "f" || key == "fdistribution" || key == "fdistrib" || key == "fdistr" || key == "fdist" || key == "fdis") return Family::F;
  if (key == "fnc" || key == "fnoncentral" || key == "fnoncentr") return Family::FNoncentral;
  if (key == "laplace" || key == "doubleexponential" || key == "doubleexp" ||
      key == "bilateralexponential" || key == "bilateralexp") return Family::Laplace;
  if (key == "logistic") return Family::Logistic;
  if (key == "geom" || key == "geometric" || key == "furry") return Family::Geometric;
  if (key == "gamma") return Family::Gamma;
  if (key == "gompertz") return Family::Gompertz;
  if (key == "halfcosine" || key == "hcosine" || key == "hcos") return Family::HalfCosine;
  if (key == "hyperbolicsecant" || key == "hsecant" || key == "hsec") return Family::HyperbolicSecant;
  if (key == "hypergeom" || key == "hypergeometric") return Family::Hypergeometric;
  if (key == "ig" || key == "inversegauss" || key == "invgauss") return Family::InverseGaussian;
  if (key == "maxwell") return Family::Maxwell;
  if (key == "nakagamim" || key == "nakagami") return Family::Nakagami;
  if (key == "negativebinomial" || key == "negbinomial" || key == "negbinom" || key == "pascal") return Family::NegativeBinomial;
  if (key == "normaltrunc" || key == "normaltruncated" || key == "gausstrunc") return Family::NormalTrunc;
  if (key == "planck") return Family::Planck;
  if (key == "poisson" || key == "po") return Family::Poisson;
  if (key == "quadratic" || key == "quad" || key == "quadr") return Family::Quadratic;
  if (key == "rademacher") return Family::Rademacher;
  if (key == "studentst" || key == "t") return Family::StudentT;
  if (key == "tri" || key == "triangular") return Family::Triangular;
  if (key == "pareto") return Family::Pareto;
  if (key == "pareto2" || key == "lomax") return Family::Lomax;
  if (key == "rayl" || key == "rayleigh") return Family::Rayleigh;
  if (key == "rice" || key == "rician") return Family::Rice;
  if (key == "semicirc" || key == "semicircle" || key == "wigner") return Family::Semicircle;
  if (key == "skellam") return Family::Skellam;
  if (key == "tukeylambda") return Family::TukeyLambda;
  if (key == "u" || key == "ushape") return Family::UShape;
  if (key == "vonmises") return Family::VonMises;
  if (key == "wald") return Family::Wald;
  if (key == "weibull" || key == "wbl" || key == "frechet") return Family::Weibull;
  if (key == "yule" || key == "yulesimon") return Family::Yule;
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
  return key == "alpha" ||
         key == "anglit" ||
         key == "arcsin" ||
         key == "bessel" ||
         key == "norm" || key == "normal" || key == "gaussian" || key == "gauss" ||
         key == "lognorm" || key == "lognormal" || key == "cobbdouglas" || key == "antilognormal" ||
         key == "uniform" || key == "unif" ||
         key == "exp" || key == "exponential" ||
         key == "bern" || key == "bernoulli" ||
         key == "beta" || key == "powerfunction" || key == "powerfunc" ||
         key == "binom" || key == "binomial" ||
         key == "bradford" ||
         key == "burr" || key == "fisk" ||
         key == "cauchy" || key == "lorentz" || key == "caushy" ||
         key == "chi" ||
         key == "chisquare" || key == "chisq" || key == "chi2" ||
         key == "chisqnc" || key == "chisqnoncentral" || key == "chisqnoncentr" || key == "chi2noncentral" ||
         key == "cosine" ||
         key == "erlang" ||
         key == "extrval" || key == "extremevalue" || key == "extrvalue" || key == "gumbel" ||
         key == "f" || key == "fdistribution" || key == "fdistrib" || key == "fdistr" || key == "fdist" || key == "fdis" ||
         key == "fnc" || key == "fnoncentral" || key == "fnoncentr" ||
         key == "laplace" || key == "doubleexponential" || key == "doubleexp" ||
         key == "bilateralexponential" || key == "bilateralexp" ||
         key == "logistic" ||
         key == "geom" || key == "geometric" || key == "furry" ||
         key == "gamma" ||
         key == "gompertz" ||
         key == "halfcosine" || key == "hcosine" || key == "hcos" ||
         key == "hyperbolicsecant" || key == "hsecant" || key == "hsec" ||
         key == "hypergeom" || key == "hypergeometric" ||
         key == "ig" || key == "inversegauss" || key == "invgauss" ||
         key == "maxwell" ||
         key == "nakagamim" || key == "nakagami-m" || key == "nakagami" ||
         key == "negativebinomial" || key == "negbinomial" || key == "negbinom" || key == "pascal" ||
         key == "normaltrunc" || key == "normaltruncated" || key == "gausstrunc" ||
         key == "planck" ||
         key == "poisson" || key == "po" ||
         key == "quadratic" || key == "quad" || key == "quadr" ||
         key == "rademacher" ||
         key == "studentst" || key == "t" ||
         key == "tri" || key == "triangular" ||
         key == "pareto" ||
         key == "pareto2" || key == "lomax" ||
         key == "rayl" || key == "rayleigh" ||
         key == "rice" || key == "rician" ||
         key == "semicirc" || key == "semicircle" || key == "wigner" ||
         key == "skellam" ||
         key == "tukeylambda" ||
         key == "u" || key == "ushape" ||
         key == "vonmises" ||
         key == "wald" ||
         key == "weibull" || key == "wbl" || key == "frechet" ||
         key == "yule" || key == "yulesimon" ||
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
  if (!(x > 0.0)) {
    return std::numeric_limits<double>::infinity();
  }
  if (x > 32.0) {
    const double nu2 = order * order;
    const double corr = 1.0 + (4.0 * nu2 - 1.0) / (8.0 * x);
    return std::max(std::sqrt(std::numbers::pi_v<double> / (2.0 * x)) * std::exp(-x) * std::max(corr, kEps),
                    std::numeric_limits<double>::min());
  }
  const double nu = (std::abs(std::sin(std::numbers::pi_v<double> * order)) < 1e-8) ? (order + 1e-6) : order;
  const double s = std::sin(std::numbers::pi_v<double> * nu);
  const double raw = (std::numbers::pi_v<double> * 0.5) *
                     (bessel_i_series_scalar_(-nu, x) - bessel_i_series_scalar_(nu, x)) / s;
  if (std::isfinite(raw) && raw > 0.0) {
    return raw;
  }
  const double nu2 = order * order;
  const double corr = 1.0 + (4.0 * nu2 - 1.0) / (8.0 * x);
  return std::max(std::sqrt(std::numbers::pi_v<double> / (2.0 * x)) * std::exp(-x) * std::max(corr, kEps),
                  std::numeric_limits<double>::min());
}

double normal_pdf_scalar_(double x, double mean, double stddev) {
  const double z = (x - mean) / stddev;
  return std::exp(-0.5 * z * z) / (stddev * std::sqrt(2.0 * std::numbers::pi_v<double>));
}

double normal_cdf_scalar_(double x, double mean, double stddev) {
  const double z = (x - mean) / stddev;
  if (z <= -8.0) return 0.0;
  if (z >= 8.0) return 1.0;
  double s = z;
  double t = std::numeric_limits<double>::infinity();
  double b = z;
  const double q = z * z;
  double i = 1.0;
  while (s != t) {
    t = s;
    i += 2.0;
    b *= q / i;
    s += b;
  }
  return 0.5 + s * std::exp(-0.5 * q - 0.91893853320467274178);
}

double normal_logcdf_scalar_(double x, double mean, double stddev) {
  const double z = (x - mean) / stddev;
  if (std::isnan(z)) return z;
  if (z < -10.0) {
    const double inv = 1.0 / z;
    const double inv2 = inv * inv;
    const double corr = 1.0 - inv2 + 3.0 * inv2 * inv2 - 15.0 * inv2 * inv2 * inv2;
    return -0.5 * z * z - std::log(-z) - 0.5 * kLog2Pi + std::log(std::max(corr, kEps));
  }
  if (z > 8.0) {
    const double tail = 0.5 * std::erfc(z / kSqrt2);
    return std::log1p(-tail);
  }
  return std::log(0.5 * std::erfc(-z / kSqrt2));
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

double logsubexp_scalar_(double a, double b) {
  if (b == -std::numeric_limits<double>::infinity()) return a;
  PLUSS_CHECK(a >= b, "subsp::stat::distr::logsubexp: expected a >= b");
  return a + std::log1p(-std::exp(b - a));
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

double erfinv_approx_scalar_(double x) {
  constexpr double a = 0.147;
  const double xc = std::clamp(x, -0.999999999, 0.999999999);
  const double ln = std::log(1.0 - xc * xc);
  const double tt = 2.0 / (std::numbers::pi_v<double> * a) + ln * 0.5;
  const double inside = tt * tt - ln / a;
  return std::copysign(std::sqrt(std::sqrt(std::max(inside, 0.0)) - tt), xc);
}

double normal_inv_cdf_scalar_(double u, double mean = 0.0, double stddev = 1.0) {
  return mean + stddev * kSqrt2 * erfinv_approx_scalar_(2.0 * u - 1.0);
}

double wrap_angle_scalar_(double x) {
  const double two_pi = 2.0 * std::numbers::pi_v<double>;
  double y = std::fmod(x + std::numbers::pi_v<double>, two_pi);
  if (y < 0.0) y += two_pi;
  return y - std::numbers::pi_v<double>;
}

template <class Fn>
double invert_monotone_scalar_(Fn&& fn, double target, double lo, double hi) {
  double left = lo;
  double right = hi;
  for (int i = 0; i < 80; ++i) {
    const double mid = 0.5 * (left + right);
    if (fn(mid) < target) {
      left = mid;
    } else {
      right = mid;
    }
  }
  return 0.5 * (left + right);
}

template <class Fn>
double poisson_mix_scalar_(double lambda_half, Fn&& term_fn) {
  constexpr int kMaxTerms = 4096;
  double weight = std::exp(-lambda_half);
  double acc = weight * term_fn(0);
  for (int j = 1; j < kMaxTerms; ++j) {
    weight *= lambda_half / static_cast<double>(j);
    acc += weight * term_fn(j);
    if (weight < 1e-14 && j > lambda_half + 20.0 * std::sqrt(std::max(lambda_half, 1.0))) break;
  }
  return acc;
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

Series norm_cdf_(const Series& X) {
  const Series xx = as_f64_(X, "cdf");
  const Series zero = zeros_like_f64_(xx);
  const Series one = ones_like_f64_(xx);
  const Series lower = xx.lt(-8.0);
  const Series upper = xx.gt(8.0);
  const Series central = lower.logical_or(upper).logical_not();

  Series s = xx.where(central, zero);
  Series t = full_like_f64_(xx, std::numeric_limits<double>::infinity());
  Series b = s.clone();
  const Series q = s.square();
  Series i = one;

  while (s.ne(t).any().item<bool>()) {
    t = s;
    i = i.add(2.0);
    b = b.mul(q.div(i));
    s = s.add(b);
  }

  const Series central_result = s.mul(q.mul(-0.5).sub(0.91893853320467274178).exp()).add(0.5);
  return one.where(upper, central_result.where(central, zero));
}

Series norm_logcdf_(const Series& x) {
  return map_series_f64_(as_f64_(x, "logcdf"), [](double xv) {
    return normal_logcdf_scalar_(xv, 0.0, 1.0);
  });
}

Series normal_cdf_(const Series& x, double mean, double stddev) {
  PLUSS_CHECK(stddev > 0.0, "subsp::stat::distr::normal cdf: std must be > 0");
  return norm_cdf_(as_f64_(x, "cdf").sub(mean).div(stddev));
}

Series normal_logcdf_(const Series& x, double mean, double stddev) {
  PLUSS_CHECK(stddev > 0.0, "subsp::stat::distr::normal logcdf: std must be > 0");
  return norm_logcdf_(as_f64_(x, "logcdf").sub(mean).div(stddev));
}

// Series normal_cdf_(const Series& x, double mean, double stddev) {
//   PLUSS_CHECK(stddev > 0.0, "subsp::stat::distr::normal cdf: std must be > 0");
//   const Series xx = as_f64_(x, "cdf");
//   return xx.sub(mean).div(stddev * kSqrt2).erf().add(1.0).mul(0.5);
// }

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
  const Series core = norm_cdf_(safe_x.log().sub(mu).div(sigma));
  return core.where(xx.gt(0.0), zeros_like_f64_(xx));
}

Series lognormal_logcdf_(const Series& x, double mu, double sigma) {
  PLUSS_CHECK(sigma > 0.0, "subsp::stat::distr::lognormal logcdf: sigma must be > 0");
  const Series xx = as_f64_(x, "logcdf");
  const Series safe_x = xx.clamp_min(kEps);
  const Series core = normal_logcdf_(safe_x.log(), mu, sigma);
  return core.where(xx.gt(0.0), full_like_f64_(xx, -std::numeric_limits<double>::infinity()));
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

Series uniform_logcdf_(const Series& x, double lo, double hi) {
  PLUSS_CHECK(hi > lo, "subsp::stat::distr::uniform logcdf: expected hi > lo");
  const Series xx = as_f64_(x, "logcdf");
  const Series below = xx.le(lo);
  const Series above = xx.ge(hi);
  const Series central = below.logical_or(above).logical_not();
  const Series core = xx.sub(lo).div(hi - lo).log();
  const Series neg_inf = full_like_f64_(xx, -std::numeric_limits<double>::infinity());
  return neg_inf.where(below, full_like_f64_(xx, 0.0).where(above, core.where(central, neg_inf)));
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

Series cauchy_logcdf_(const Series& x, double location, double scale) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::cauchy logcdf: scale must be > 0");
  return cauchy_cdf_(x, location, scale).log();
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

Series laplace_logcdf_(const Series& x, double location, double scale) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::laplace logcdf: scale must be > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv < location) return -std::log(2.0) + (xv - location) / scale;
    return std::log1p(-0.5 * std::exp((location - xv) / scale));
  });
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

Series logistic_logcdf_(const Series& x, double location, double scale) {
  PLUSS_CHECK(scale > 0.0, "subsp::stat::distr::logistic logcdf: scale must be > 0");
  return map_series_f64_(x, [=](double xv) {
    const double z = (xv - location) / scale;
    if (z >= 0.0) return -std::log1p(std::exp(-z));
    return z - std::log1p(std::exp(z));
  });
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

Series rayleigh_logcdf_(const Series& x, double sigma) {
  PLUSS_CHECK(sigma > 0.0, "subsp::stat::distr::rayleigh logcdf: sigma must be > 0");
  return map_series_f64_(x, [=](double xv) {
    if (xv < 0.0) return -std::numeric_limits<double>::infinity();
    return std::log1p(-std::exp(-(xv * xv) / (2.0 * sigma * sigma)));
  });
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

Series normaltrunc_logcdf_(const Series& x, double a, double b, double mu, double sigma) {
  PLUSS_CHECK(b >= a && sigma > 0.0, "subsp::stat::distr::normaltrunc logcdf: invalid parameters");
  const double log_left = normal_logcdf_scalar_(a, mu, sigma);
  const double log_right = normal_logcdf_scalar_(b, mu, sigma);
  const double log_denom = logsubexp_scalar_(log_right, log_left);
  return map_series_f64_(x, [=](double xv) {
    if (xv <= a) return -std::numeric_limits<double>::infinity();
    if (xv >= b) return 0.0;
    return logsubexp_scalar_(normal_logcdf_scalar_(xv, mu, sigma), log_left) - log_denom;
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

double gig_pdf_scalar_(double y, double lambda, double chi, double psi) {
  const double lp = gig_logpdf_scalar_(y, lambda, chi, psi);
  return std::isfinite(lp) ? std::exp(lp) : 0.0;
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
    return gig_pdf_scalar_(xv, lambda, chi, psi);
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
  const double chi = delta * delta;
  const double psi = alpha * alpha - beta * beta;
  return map_series_f64_(x, [=](double xv) {
    constexpr double eps = 1e-8;
    const auto mixture_integrand = [=](double t) {
      const double tc = std::clamp(t, eps, 1.0 - eps);
      const double w = tc / (1.0 - tc);
      const double jac = 1.0 / ((1.0 - tc) * (1.0 - tc));
      const double conditional_cdf = normal_cdf_scalar_(xv, mu + beta * w, std::sqrt(w));
      return conditional_cdf * gig_pdf_scalar_(w, lambda, chi, psi) * jac;
    };
    const double v = integrate_simpson_scalar_(mixture_integrand, eps, 1.0 - eps);
    if (std::isfinite(v)) {
      return std::clamp(v, 0.0, 1.0);
    }
    return xv < mu ? 0.0 : 1.0;
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

Series alpha_pdf_(const Series& x, double a, double b) {
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::alpha pdf: expected a,b > 0");
  const double norm_a = normal_cdf_scalar_(a, 0.0, 1.0);
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    const double z = a - b / y;
    return b * normal_pdf_scalar_(z, 0.0, 1.0) / (y * y * norm_a);
  });
}

Series alpha_logpdf_(const Series& x, double a, double b) {
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::alpha logpdf: expected a,b > 0");
  const double log_norm_a = std::log(normal_cdf_scalar_(a, 0.0, 1.0));
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return -std::numeric_limits<double>::infinity();
    const double z = a - b / y;
    return std::log(b) - 2.0 * std::log(y) - 0.5 * z * z - 0.5 * std::log(2.0 * std::numbers::pi_v<double>) - log_norm_a;
  });
}

Series alpha_cdf_(const Series& x, double a, double b) {
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::alpha cdf: expected a,b > 0");
  const double norm_a = normal_cdf_scalar_(a, 0.0, 1.0);
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    return normal_cdf_scalar_(a - b / y, 0.0, 1.0) / norm_a;
  });
}

Series alpha_sample_(double a, double b, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::alpha sample: expected a,b > 0");
  const double norm_a = normal_cdf_scalar_(a, 0.0, 1.0);
  return map_series_f64_(unit_uniform_(sizes, cfg, seed), [=](double u) {
    return b / (a - normal_inv_cdf_scalar_(norm_a * u));
  }).to(cfg.platform, natural_cfg_(cfg, ScalarType::Float64).dtype);
}

Series anglit_pdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::anglit pdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    if (y < t - s || y > t + s) return 0.0;
    return std::sin((std::numbers::pi_v<double> * 0.5) * (y - t) / s + std::numbers::pi_v<double> * 0.5) / s;
  });
}

Series anglit_logpdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::anglit logpdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    if (y < t - s || y > t + s) return -std::numeric_limits<double>::infinity();
    return std::log(std::sin((std::numbers::pi_v<double> * 0.5) * (y - t) / s + std::numbers::pi_v<double> * 0.5)) - std::log(s);
  });
}

Series anglit_cdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::anglit cdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    if (y <= t - s) return 0.0;
    if (y >= t + s) return 1.0;
    return std::pow(std::sin((std::numbers::pi_v<double> * 0.25) * (y - t) / s + std::numbers::pi_v<double> * 0.25), 2.0);
  });
}

Series anglit_sample_(double t, double s, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::anglit sample: expected s > 0");
  return map_series_f64_(unit_uniform_(sizes, cfg, seed), [=](double u) {
    return t + s * (4.0 / std::numbers::pi_v<double> * std::asin(std::sqrt(u)) - 1.0);
  }).to(cfg.platform, natural_cfg_(cfg, ScalarType::Float64).dtype);
}

Series arcsin_pdf_(const Series& x) { return beta_pdf_(x, 0.0, 1.0, 0.5, 0.5); }
Series arcsin_logpdf_(const Series& x) { return beta_logpdf_(x, 0.0, 1.0, 0.5, 0.5); }
Series arcsin_cdf_(const Series& x) { return beta_cdf_(x, 0.0, 1.0, 0.5, 0.5); }
Series arcsin_sample_(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  return beta_sample_(0.0, 1.0, 0.5, 0.5, sizes, cfg, seed);
}

double bessel_logpmf_scalar_(std::int64_t y, double nu, double a) {
  if (y < 0) return -std::numeric_limits<double>::infinity();
  const double den = bessel_i_series_scalar_(nu, a);
  if (!(den > 0.0)) return -std::numeric_limits<double>::infinity();
  return (2.0 * static_cast<double>(y) + nu) * std::log(a * 0.5) - std::log(den) - std::lgamma(y + 1.0) - std::lgamma(y + nu + 1.0);
}

Series bessel_pdf_(const Series& x, double nu, double a) {
  PLUSS_CHECK(nu > -1.0 && a > 0.0, "subsp::stat::distr::bessel pdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    const auto k = static_cast<std::int64_t>(std::floor(y));
    if (y != static_cast<double>(k)) return 0.0;
    const double lp = bessel_logpmf_scalar_(k, nu, a);
    return std::isfinite(lp) ? std::exp(lp) : 0.0;
  });
}

Series bessel_logpdf_(const Series& x, double nu, double a) {
  PLUSS_CHECK(nu > -1.0 && a > 0.0, "subsp::stat::distr::bessel logpdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    const auto k = static_cast<std::int64_t>(std::floor(y));
    return (y != static_cast<double>(k)) ? -std::numeric_limits<double>::infinity() : bessel_logpmf_scalar_(k, nu, a);
  });
}

Series bessel_cdf_(const Series& x, double nu, double a) {
  PLUSS_CHECK(nu > -1.0 && a > 0.0, "subsp::stat::distr::bessel cdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    const auto k = static_cast<std::int64_t>(std::floor(y));
    if (k < 0) return 0.0;
    double acc = 0.0;
    for (std::int64_t i = 0; i <= k; ++i) acc += std::exp(bessel_logpmf_scalar_(i, nu, a));
    return std::min(1.0, acc);
  });
}

Series bessel_sample_(double nu, double a, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(nu > -1.0 && a > 0.0, "subsp::stat::distr::bessel sample: invalid parameters");
  return sample_host_<std::int64_t>(sizes, cfg, ScalarType::Int64, seed, [=](std::mt19937_64& rng) {
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    const double u = uniform(rng);
    double acc = 0.0;
    for (std::int64_t k = 0; k < 100000; ++k) {
      acc += std::exp(bessel_logpmf_scalar_(k, nu, a));
      if (acc >= u || 1.0 - acc < 1e-12) return k;
    }
    return static_cast<std::int64_t>(0);
  });
}

Series bradford_pdf_(const Series& x, double b) {
  PLUSS_CHECK(b > -1.0 && std::abs(b) > kEps, "subsp::stat::distr::bradford pdf: invalid b");
  const double den = std::log1p(b);
  return map_series_f64_(x, [=](double y) {
    if (y < 0.0 || y > 1.0) return 0.0;
    return b / (den * (1.0 + b * y));
  });
}

Series bradford_logpdf_(const Series& x, double b) {
  PLUSS_CHECK(b > -1.0 && std::abs(b) > kEps, "subsp::stat::distr::bradford logpdf: invalid b");
  const double den = std::log1p(b);
  return map_series_f64_(x, [=](double y) {
    if (y < 0.0 || y > 1.0) return -std::numeric_limits<double>::infinity();
    return std::log(b) - std::log(den) - std::log(1.0 + b * y);
  });
}

Series bradford_cdf_(const Series& x, double b) {
  PLUSS_CHECK(b > -1.0 && std::abs(b) > kEps, "subsp::stat::distr::bradford cdf: invalid b");
  const double den = std::log1p(b);
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    if (y >= 1.0) return 1.0;
    return std::log1p(b * y) / den;
  });
}

Series bradford_sample_(double b, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(b > -1.0 && std::abs(b) > kEps, "subsp::stat::distr::bradford sample: invalid b");
  return map_series_f64_(unit_uniform_(sizes, cfg, seed), [=](double u) {
    return (std::pow(1.0 + b, u) - 1.0) / b;
  }).to(cfg.platform, natural_cfg_(cfg, ScalarType::Float64).dtype);
}

Series burr_pdf_(const Series& x, double c, double d) {
  PLUSS_CHECK(c > 0.0 && d > 0.0, "subsp::stat::distr::burr pdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    return c * d * std::pow(y, -c - 1.0) * std::pow(1.0 + std::pow(y, -c), -d - 1.0);
  });
}

Series burr_logpdf_(const Series& x, double c, double d) {
  PLUSS_CHECK(c > 0.0 && d > 0.0, "subsp::stat::distr::burr logpdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(c * d) + (-c - 1.0) * std::log(y) + (-d - 1.0) * std::log1p(std::pow(y, -c));
  });
}

Series burr_cdf_(const Series& x, double c, double d) {
  PLUSS_CHECK(c > 0.0 && d > 0.0, "subsp::stat::distr::burr cdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    return std::pow(1.0 + std::pow(y, -c), -d);
  });
}

Series burr_sample_(double c, double d, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(c > 0.0 && d > 0.0, "subsp::stat::distr::burr sample: invalid parameters");
  return map_series_f64_(unit_uniform_(sizes, cfg, seed), [=](double u) {
    return std::pow(std::pow(u, -1.0 / d) - 1.0, -1.0 / c);
  }).to(cfg.platform, natural_cfg_(cfg, ScalarType::Float64).dtype);
}

Series chi_pdf_(const Series& x, double a, double b, double nu) {
  PLUSS_CHECK(b > 0.0 && nu > 0.0, "subsp::stat::distr::chi pdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= a) return 0.0;
    const double z = (y - a) / b;
    return std::exp(-0.5 * z * z + (nu - 1.0) * std::log(z) - (0.5 * nu - 1.0) * std::log(2.0) - std::lgamma(0.5 * nu) - std::log(b));
  });
}

Series chi_logpdf_(const Series& x, double a, double b, double nu) {
  PLUSS_CHECK(b > 0.0 && nu > 0.0, "subsp::stat::distr::chi logpdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= a) return -std::numeric_limits<double>::infinity();
    const double z = (y - a) / b;
    return -0.5 * z * z + (nu - 1.0) * std::log(z) - (0.5 * nu - 1.0) * std::log(2.0) - std::lgamma(0.5 * nu) - std::log(b);
  });
}

Series chi_cdf_(const Series& x, double a, double b, double nu) {
  PLUSS_CHECK(b > 0.0 && nu > 0.0, "subsp::stat::distr::chi cdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= a) return 0.0;
    const double z = (y - a) / b;
    return regularized_gamma_p_scalar_(0.5 * nu, 0.5 * z * z);
  });
}

Series chi_sample_(double a, double b, double nu, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(b > 0.0 && nu > 0.0, "subsp::stat::distr::chi sample: invalid parameters");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::gamma_distribution<double> dist(0.5 * nu, 2.0);
    return a + b * std::sqrt(dist(rng));
  });
}

Series chisqnc_pdf_(const Series& x, double lambda, std::int64_t r) {
  PLUSS_CHECK(lambda >= 0.0 && r > 0, "subsp::stat::distr::chisqnc pdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    return poisson_mix_scalar_(0.5 * lambda, [=](int j) {
      const double df = static_cast<double>(r + 2 * j);
      const double logp = (0.5 * df - 1.0) * std::log(y) - 0.5 * y - 0.5 * df * std::log(2.0) - std::lgamma(0.5 * df);
      return std::exp(logp);
    });
  });
}

Series chisqnc_logpdf_(const Series& x, double lambda, std::int64_t r) {
  return map_series_f64_(chisqnc_pdf_(x, lambda, r), [](double y) {
    return (y <= 0.0) ? -std::numeric_limits<double>::infinity() : std::log(y);
  });
}

Series chisqnc_cdf_(const Series& x, double lambda, std::int64_t r) {
  PLUSS_CHECK(lambda >= 0.0 && r > 0, "subsp::stat::distr::chisqnc cdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    return std::clamp(poisson_mix_scalar_(0.5 * lambda, [=](int j) {
      return regularized_gamma_p_scalar_(0.5 * static_cast<double>(r + 2 * j), 0.5 * y);
    }), 0.0, 1.0);
  });
}

Series chisqnc_sample_(double lambda, std::int64_t r, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(lambda >= 0.0 && r > 0, "subsp::stat::distr::chisqnc sample: invalid parameters");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::normal_distribution<double> normal(std::sqrt(lambda), 1.0);
    std::gamma_distribution<double> chisq(0.5 * static_cast<double>(r - 1), 2.0);
    const double z = normal(rng);
    return z * z + ((r > 1) ? chisq(rng) : 0.0);
  });
}

Series cosine_pdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::cosine pdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    if (y < t - s || y > t + s) return 0.0;
    return (1.0 + std::cos(std::numbers::pi_v<double> * (y - t) / s)) / (2.0 * s);
  });
}

Series cosine_logpdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::cosine logpdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    if (y < t - s || y > t + s) return -std::numeric_limits<double>::infinity();
    return std::log1p(std::cos(std::numbers::pi_v<double> * (y - t) / s)) - std::log(2.0 * s);
  });
}

Series cosine_cdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::cosine cdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    if (y <= t - s) return 0.0;
    if (y >= t + s) return 1.0;
    const double z = (y - t) / s;
    return (std::numbers::pi_v<double> + std::numbers::pi_v<double> * z + std::sin(std::numbers::pi_v<double> * z)) / (2.0 * std::numbers::pi_v<double>);
  });
}

Series cosine_sample_(double t, double s, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::cosine sample: expected s > 0");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::uniform_real_distribution<double> uni_x(t - s, t + s);
    std::uniform_real_distribution<double> uni01(0.0, 1.0);
    while (true) {
      const double y = uni_x(rng);
      if (uni01(rng) <= 0.5 * (1.0 + std::cos(std::numbers::pi_v<double> * (y - t) / s))) return y;
    }
  });
}

Series erlang_pdf_(const Series& x, double a, std::int64_t n) { return gamma_pdf_(x, 0.0, a, static_cast<double>(n)); }
Series erlang_logpdf_(const Series& x, double a, std::int64_t n) { return gamma_logpdf_(x, 0.0, a, static_cast<double>(n)); }
Series erlang_cdf_(const Series& x, double a, std::int64_t n) { return gamma_cdf_(x, 0.0, a, static_cast<double>(n)); }
Series erlang_sample_(double a, std::int64_t n, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  return gamma_sample_(0.0, a, static_cast<double>(n), sizes, cfg, seed);
}

Series extreme_value_pdf_(const Series& x, double mu, double b) {
  PLUSS_CHECK(b > 0.0, "subsp::stat::distr::extrval pdf: expected b > 0");
  return map_series_f64_(x, [=](double y) {
    const double z = (mu - y) / b;
    return std::exp(z - std::exp(z)) / b;
  });
}

Series extreme_value_logpdf_(const Series& x, double mu, double b) {
  PLUSS_CHECK(b > 0.0, "subsp::stat::distr::extrval logpdf: expected b > 0");
  return map_series_f64_(x, [=](double y) {
    const double z = (mu - y) / b;
    return z - std::exp(z) - std::log(b);
  });
}

Series extreme_value_cdf_(const Series& x, double mu, double b) {
  PLUSS_CHECK(b > 0.0, "subsp::stat::distr::extrval cdf: expected b > 0");
  return map_series_f64_(x, [=](double y) { return std::exp(-std::exp((mu - y) / b)); });
}

Series extreme_value_sample_(double mu, double b, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(b > 0.0, "subsp::stat::distr::extrval sample: expected b > 0");
  return map_series_f64_(unit_uniform_(sizes, cfg, seed), [=](double u) { return mu - b * std::log(-std::log(u)); })
      .to(cfg.platform, natural_cfg_(cfg, ScalarType::Float64).dtype);
}

Series f_pdf_(const Series& x, double d1, double d2) {
  PLUSS_CHECK(d1 > 0.0 && d2 > 0.0, "subsp::stat::distr::f pdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    const double z = d1 * y / (d1 * y + d2);
    return std::exp((0.5 * d1) * std::log(z) + (0.5 * d2) * std::log1p(-z) - std::log(y) - log_beta_scalar_(0.5 * d1, 0.5 * d2));
  });
}

Series f_logpdf_(const Series& x, double d1, double d2) {
  PLUSS_CHECK(d1 > 0.0 && d2 > 0.0, "subsp::stat::distr::f logpdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return -std::numeric_limits<double>::infinity();
    const double z = d1 * y / (d1 * y + d2);
    return (0.5 * d1) * std::log(z) + (0.5 * d2) * std::log1p(-z) - std::log(y) - log_beta_scalar_(0.5 * d1, 0.5 * d2);
  });
}

Series f_cdf_(const Series& x, double d1, double d2) {
  PLUSS_CHECK(d1 > 0.0 && d2 > 0.0, "subsp::stat::distr::f cdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    return regularized_beta_scalar_(0.5 * d1, 0.5 * d2, d1 * y / (d1 * y + d2));
  });
}

Series f_sample_(double d1, double d2, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(d1 > 0.0 && d2 > 0.0, "subsp::stat::distr::f sample: invalid parameters");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::gamma_distribution<double> g1(0.5 * d1, 2.0);
    std::gamma_distribution<double> g2(0.5 * d2, 2.0);
    return (g1(rng) / d1) / (g2(rng) / d2);
  });
}

Series fnc_pdf_(const Series& x, double lambda, double d1, double d2) {
  return map_series_f64_(x, [=](double y) {
    return poisson_mix_scalar_(0.5 * lambda, [=](int j) {
      const double df1 = d1 + 2.0 * static_cast<double>(j);
      const double z = d1 * y / (d1 * y + d2);
      return std::exp((0.5 * df1) * std::log(z) + (0.5 * d2) * std::log1p(-z) - std::log(y) - log_beta_scalar_(0.5 * df1, 0.5 * d2));
    });
  });
}

Series fnc_logpdf_(const Series& x, double lambda, double d1, double d2) {
  return map_series_f64_(fnc_pdf_(x, lambda, d1, d2), [](double y) {
    return (y <= 0.0) ? -std::numeric_limits<double>::infinity() : std::log(y);
  });
}

Series fnc_cdf_(const Series& x, double lambda, double d1, double d2) {
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    return std::clamp(poisson_mix_scalar_(0.5 * lambda, [=](int j) {
      return regularized_beta_scalar_(0.5 * (d1 + 2.0 * static_cast<double>(j)), 0.5 * d2, d1 * y / (d1 * y + d2));
    }), 0.0, 1.0);
  });
}

Series fnc_sample_(double lambda, double d1, double d2, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(lambda >= 0.0 && d1 > 0.0 && d2 > 0.0, "subsp::stat::distr::fnc sample: invalid parameters");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::normal_distribution<double> normal(std::sqrt(lambda), 1.0);
    std::gamma_distribution<double> nc_rest(0.5 * std::max(d1 - 1.0, 0.0), 2.0);
    std::gamma_distribution<double> central(0.5 * d2, 2.0);
    const double z = normal(rng);
    const double nc = z * z + ((d1 > 1.0) ? nc_rest(rng) : 0.0);
    return (nc / d1) / (central(rng) / d2);
  });
}

Series gompertz_pdf_(const Series& x, double b, double c) {
  PLUSS_CHECK(b > 0.0 && c > 1.0, "subsp::stat::distr::gompertz pdf: invalid parameters");
  const double logc = std::log(c);
  return map_series_f64_(x, [=](double y) {
    if (y < 0.0) return 0.0;
    return b * std::pow(c, y) * std::exp(-b * (std::pow(c, y) - 1.0) / logc);
  });
}

Series gompertz_logpdf_(const Series& x, double b, double c) {
  PLUSS_CHECK(b > 0.0 && c > 1.0, "subsp::stat::distr::gompertz logpdf: invalid parameters");
  const double logc = std::log(c);
  return map_series_f64_(x, [=](double y) {
    if (y < 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(b) + y * logc - b * (std::pow(c, y) - 1.0) / logc;
  });
}

Series gompertz_cdf_(const Series& x, double b, double c) {
  PLUSS_CHECK(b > 0.0 && c > 1.0, "subsp::stat::distr::gompertz cdf: invalid parameters");
  const double logc = std::log(c);
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    return 1.0 - std::exp(-b * (std::pow(c, y) - 1.0) / logc);
  });
}

Series gompertz_sample_(double b, double c, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(b > 0.0 && c > 1.0, "subsp::stat::distr::gompertz sample: invalid parameters");
  const double logc = std::log(c);
  return map_series_f64_(unit_uniform_(sizes, cfg, seed), [=](double u) {
    return std::log(1.0 - std::log1p(-u) * logc / b) / logc;
  }).to(cfg.platform, natural_cfg_(cfg, ScalarType::Float64).dtype);
}

Series halfcosine_pdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::halfcosine pdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    if (y < t - s || y > t + s) return 0.0;
    return (std::numbers::pi_v<double> / (4.0 * s)) * std::cos(std::numbers::pi_v<double> * (y - t) / (2.0 * s));
  });
}

Series halfcosine_logpdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::halfcosine logpdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    if (y < t - s || y > t + s) return -std::numeric_limits<double>::infinity();
    return std::log(std::numbers::pi_v<double> / (4.0 * s)) + std::log(std::cos(std::numbers::pi_v<double> * (y - t) / (2.0 * s)));
  });
}

Series halfcosine_cdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::halfcosine cdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    if (y <= t - s) return 0.0;
    if (y >= t + s) return 1.0;
    return 0.5 * (1.0 + std::sin(std::numbers::pi_v<double> * (y - t) / (2.0 * s)));
  });
}

Series halfcosine_sample_(double t, double s, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::halfcosine sample: expected s > 0");
  return map_series_f64_(unit_uniform_(sizes, cfg, seed), [=](double u) {
    return t + s * 2.0 / std::numbers::pi_v<double> * std::asin(2.0 * u - 1.0);
  }).to(cfg.platform, natural_cfg_(cfg, ScalarType::Float64).dtype);
}

Series hsec_pdf_(const Series& x, double a, double b) {
  PLUSS_CHECK(b > 0.0, "subsp::stat::distr::hsec pdf: expected b > 0");
  return map_series_f64_(x, [=](double y) { return 1.0 / (std::numbers::pi_v<double> * b * std::cosh((y - a) / b)); });
}

Series hsec_logpdf_(const Series& x, double a, double b) {
  PLUSS_CHECK(b > 0.0, "subsp::stat::distr::hsec logpdf: expected b > 0");
  return map_series_f64_(x, [=](double y) { return -std::log(std::numbers::pi_v<double> * b) - std::log(std::cosh((y - a) / b)); });
}

Series hsec_cdf_(const Series& x, double a, double b) {
  PLUSS_CHECK(b > 0.0, "subsp::stat::distr::hsec cdf: expected b > 0");
  return map_series_f64_(x, [=](double y) { return 2.0 * std::atan(std::exp((y - a) / b)) / std::numbers::pi_v<double>; });
}

Series hsec_sample_(double a, double b, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(b > 0.0, "subsp::stat::distr::hsec sample: expected b > 0");
  return map_series_f64_(unit_uniform_(sizes, cfg, seed), [=](double u) { return a + b * std::log(std::tan(std::numbers::pi_v<double> * 0.5 * u)); })
      .to(cfg.platform, natural_cfg_(cfg, ScalarType::Float64).dtype);
}

Series maxwell_pdf_(const Series& x, double a) { return chi_pdf_(x, 0.0, a, 3.0); }
Series maxwell_logpdf_(const Series& x, double a) { return chi_logpdf_(x, 0.0, a, 3.0); }
Series maxwell_cdf_(const Series& x, double a) { return chi_cdf_(x, 0.0, a, 3.0); }
Series maxwell_sample_(double a, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  return chi_sample_(0.0, a, 3.0, sizes, cfg, seed);
}

Series nakagami_pdf_(const Series& x, double m, double omega) {
  PLUSS_CHECK(m > 0.0 && omega > 0.0, "subsp::stat::distr::nakagami pdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y < 0.0) return 0.0;
    return 2.0 / std::tgamma(m) * std::pow(m / omega, m) * std::pow(y, 2.0 * m - 1.0) * std::exp(-m * y * y / omega);
  });
}

Series nakagami_logpdf_(const Series& x, double m, double omega) {
  PLUSS_CHECK(m > 0.0 && omega > 0.0, "subsp::stat::distr::nakagami logpdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y < 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(2.0) - std::lgamma(m) + m * std::log(m / omega) + (2.0 * m - 1.0) * std::log(std::max(y, kEps)) - m * y * y / omega;
  });
}

Series nakagami_cdf_(const Series& x, double m, double omega) {
  PLUSS_CHECK(m > 0.0 && omega > 0.0, "subsp::stat::distr::nakagami cdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    return regularized_gamma_p_scalar_(m, m * y * y / omega);
  });
}

Series nakagami_sample_(double m, double omega, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(m > 0.0 && omega > 0.0, "subsp::stat::distr::nakagami sample: invalid parameters");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::gamma_distribution<double> dist(m, omega / m);
    return std::sqrt(dist(rng));
  });
}

Series negbin_pdf_(const Series& x, double r, double p) {
  PLUSS_CHECK(r > 0.0 && p > 0.0 && p < 1.0, "subsp::stat::distr::negbinom pdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    const auto k = std::floor(y);
    if (y != k || k < 0.0) return 0.0;
    return std::exp(std::lgamma(r + k) - std::lgamma(k + 1.0) - std::lgamma(r) + r * std::log(p) + k * std::log1p(-p));
  });
}

Series negbin_logpdf_(const Series& x, double r, double p) {
  PLUSS_CHECK(r > 0.0 && p > 0.0 && p < 1.0, "subsp::stat::distr::negbinom logpdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    const auto k = std::floor(y);
    if (y != k || k < 0.0) return -std::numeric_limits<double>::infinity();
    return std::lgamma(r + k) - std::lgamma(k + 1.0) - std::lgamma(r) + r * std::log(p) + k * std::log1p(-p);
  });
}

Series negbin_cdf_(const Series& x, double r, double p) {
  PLUSS_CHECK(r > 0.0 && p > 0.0 && p < 1.0, "subsp::stat::distr::negbinom cdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    const auto k = static_cast<int>(std::floor(y));
    if (k < 0) return 0.0;
    return regularized_beta_scalar_(r, static_cast<double>(k + 1), p);
  });
}

Series negbin_sample_(double r, double p, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(r > 0.0 && p > 0.0 && p < 1.0, "subsp::stat::distr::negbinom sample: invalid parameters");
  return sample_host_<std::int64_t>(sizes, cfg, ScalarType::Int64, seed, [=](std::mt19937_64& rng) {
    std::gamma_distribution<double> gamma(r, (1.0 - p) / p);
    std::poisson_distribution<std::int64_t> pois(gamma(rng));
    return pois(rng);
  });
}

Series planck_pdf_(const Series& x, double a, double b) {
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::planck pdf: invalid parameters");
  const double log_norm = (a + 1.0) * std::log(b) - std::lgamma(a + 1.0) - std::log(zeta_scalar_(a + 1.0));
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    return std::exp(log_norm + a * std::log(y) - std::log(std::exp(b * y) - 1.0));
  });
}

Series planck_logpdf_(const Series& x, double a, double b) {
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::planck logpdf: invalid parameters");
  const double log_norm = (a + 1.0) * std::log(b) - std::lgamma(a + 1.0) - std::log(zeta_scalar_(a + 1.0));
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return -std::numeric_limits<double>::infinity();
    return log_norm + a * std::log(y) - std::log(std::exp(b * y) - 1.0);
  });
}

Series planck_cdf_(const Series& x, double a, double b) {
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::planck cdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    const auto pdf = [=](double t) {
      if (t <= 0.0) return 0.0;
      return std::exp((a + 1.0) * std::log(b) - std::lgamma(a + 1.0) - std::log(zeta_scalar_(a + 1.0)) +
                      a * std::log(t) - std::log(std::exp(b * t) - 1.0));
    };
    return std::clamp(integrate_simpson_scalar_(pdf, 1e-8, y), 0.0, 1.0);
  });
}

Series planck_sample_(double a, double b, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(a > 0.0 && b > 0.0, "subsp::stat::distr::planck sample: invalid parameters");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::gamma_distribution<double> gamma(a + 1.0, 1.0);
    const double au = a + 1.0;
    while (true) {
      std::uniform_real_distribution<double> uniform(0.0, 1.0);
      const double b2 = std::pow(2.0, au - 1.0);
      const double am1 = au - 1.0;
      const double bm1 = b2 - 1.0;
      const double u1 = uniform(rng);
      const double n = std::floor(std::pow(u1, -1.0 / am1));
      const double u2 = uniform(rng);
      const double t = std::pow(1.0 + 1.0 / n, au - 1.0);
      if (u2 * n * (t - 1.0) / bm1 > t / b2) continue;
      if (n >= 1.0) return gamma(rng) / (b * n);
    }
  });
}

Series quadratic_pdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::quadratic pdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    const double z = (y - t) / s;
    if (std::abs(z) > 1.0) return 0.0;
    return 3.0 / (4.0 * s) * (1.0 - z * z);
  });
}

Series quadratic_logpdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::quadratic logpdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    const double z = (y - t) / s;
    if (std::abs(z) > 1.0) return -std::numeric_limits<double>::infinity();
    return std::log(3.0 / (4.0 * s)) + std::log(1.0 - z * z);
  });
}

Series quadratic_cdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::quadratic cdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    const double z = (y - t) / s;
    if (z <= -1.0) return 0.0;
    if (z >= 1.0) return 1.0;
    return 0.5 + 0.75 * z - 0.25 * z * z * z;
  });
}

Series quadratic_sample_(double t, double s, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::quadratic sample: expected s > 0");
  return map_series_f64_(unit_uniform_(sizes, cfg, seed), [=](double u) {
    return t + s * 2.0 * std::sin(std::asin(2.0 * u - 1.0) / 3.0);
  }).to(cfg.platform, natural_cfg_(cfg, ScalarType::Float64).dtype);
}

Series rademacher_pdf_(const Series& x) {
  return map_series_f64_(x, [](double y) { return (y == -1.0 || y == 1.0) ? 0.5 : 0.0; });
}

Series rademacher_logpdf_(const Series& x) {
  return map_series_f64_(x, [](double y) {
    return (y == -1.0 || y == 1.0) ? -std::log(2.0) : -std::numeric_limits<double>::infinity();
  });
}

Series rademacher_cdf_(const Series& x) {
  return map_series_f64_(x, [](double y) {
    if (y < -1.0) return 0.0;
    if (y < 1.0) return 0.5;
    return 1.0;
  });
}

Series rademacher_sample_(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  return sample_host_<std::int64_t>(sizes, cfg, ScalarType::Int64, seed, [](std::mt19937_64& rng) {
    std::bernoulli_distribution dist(0.5);
    return dist(rng) ? std::int64_t{1} : std::int64_t{-1};
  });
}

Series rice_pdf_(const Series& x, double v, double sigma) {
  PLUSS_CHECK(v >= 0.0 && sigma > 0.0, "subsp::stat::distr::rice pdf: invalid parameters");
  const double sigma2 = sigma * sigma;
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    return bessel_i_series_scalar_(0.0, y * v / sigma2) * y / sigma2 * std::exp(-(y * y + v * v) / (2.0 * sigma2));
  });
}

Series rice_logpdf_(const Series& x, double v, double sigma) {
  return map_series_f64_(rice_pdf_(x, v, sigma), [](double y) {
    return (y <= 0.0) ? -std::numeric_limits<double>::infinity() : std::log(y);
  });
}

Series rice_cdf_(const Series& x, double v, double sigma) {
  PLUSS_CHECK(v >= 0.0 && sigma > 0.0, "subsp::stat::distr::rice cdf: invalid parameters");
  return map_series_f64_(x, [=](double y) {
    if (y <= 0.0) return 0.0;
    const auto pdf = [=](double t) {
      if (t <= 0.0) return 0.0;
      const double sigma2 = sigma * sigma;
      return bessel_i_series_scalar_(0.0, t * v / sigma2) * t / sigma2 * std::exp(-(t * t + v * v) / (2.0 * sigma2));
    };
    return std::clamp(integrate_simpson_scalar_(pdf, 1e-8, y), 0.0, 1.0);
  });
}

Series rice_sample_(double v, double sigma, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(v >= 0.0 && sigma > 0.0, "subsp::stat::distr::rice sample: invalid parameters");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::normal_distribution<double> normal(0.0, 1.0);
    const double z1 = v + sigma * normal(rng);
    const double z2 = sigma * normal(rng);
    return std::sqrt(z1 * z1 + z2 * z2);
  });
}

Series semicircle_pdf_(const Series& x, double m, double R) {
  PLUSS_CHECK(R > 0.0, "subsp::stat::distr::semicircle pdf: expected R > 0");
  return map_series_f64_(x, [=](double y) {
    const double z = (y - m) / R;
    if (std::abs(z) > 1.0) return 0.0;
    return 2.0 / (std::numbers::pi_v<double> * R) * std::sqrt(1.0 - z * z);
  });
}

Series semicircle_logpdf_(const Series& x, double m, double R) {
  PLUSS_CHECK(R > 0.0, "subsp::stat::distr::semicircle logpdf: expected R > 0");
  return map_series_f64_(x, [=](double y) {
    const double z = (y - m) / R;
    if (std::abs(z) > 1.0) return -std::numeric_limits<double>::infinity();
    return std::log(2.0 / (std::numbers::pi_v<double> * R)) + 0.5 * std::log(1.0 - z * z);
  });
}

Series semicircle_cdf_(const Series& x, double m, double R) {
  PLUSS_CHECK(R > 0.0, "subsp::stat::distr::semicircle cdf: expected R > 0");
  return map_series_f64_(x, [=](double y) {
    const double z = (y - m) / R;
    if (z <= -1.0) return 0.0;
    if (z >= 1.0) return 1.0;
    return 0.5 + (std::asin(z) + z * std::sqrt(1.0 - z * z)) / std::numbers::pi_v<double>;
  });
}

Series semicircle_sample_(double m, double R, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(R > 0.0, "subsp::stat::distr::semicircle sample: expected R > 0");
  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::gamma_distribution<double> g(1.5, 1.0);
    const double x1 = g(rng);
    const double x2 = g(rng);
    return m + R * (2.0 * (x1 / (x1 + x2)) - 1.0);
  });
}

Series tukeylambda_pdf_(const Series& x, double lambda) {
  if (std::abs(lambda) < 1e-12) return logistic_pdf_(x, 0.0, 1.0);
  return map_series_f64_(x, [=](double y) {
    const auto quant = [=](double u) { return (std::pow(u, lambda) - std::pow(1.0 - u, lambda)) / lambda; };
    const double u = invert_monotone_scalar_(quant, y, kEps, 1.0 - kEps);
    return 1.0 / (std::pow(u, lambda - 1.0) + std::pow(1.0 - u, lambda - 1.0));
  });
}

Series tukeylambda_logpdf_(const Series& x, double lambda) {
  if (std::abs(lambda) < 1e-12) return logistic_logpdf_(x, 0.0, 1.0);
  return map_series_f64_(tukeylambda_pdf_(x, lambda), [](double y) {
    return (y <= 0.0) ? -std::numeric_limits<double>::infinity() : std::log(y);
  });
}

Series tukeylambda_cdf_(const Series& x, double lambda) {
  if (std::abs(lambda) < 1e-12) return logistic_cdf_(x, 0.0, 1.0);
  return map_series_f64_(x, [=](double y) {
    const auto quant = [=](double u) { return (std::pow(u, lambda) - std::pow(1.0 - u, lambda)) / lambda; };
    return invert_monotone_scalar_(quant, y, kEps, 1.0 - kEps);
  });
}

Series tukeylambda_sample_(double lambda, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  if (std::abs(lambda) < 1e-12) return logistic_sample_(0.0, 1.0, sizes, cfg, seed);
  return map_series_f64_(unit_uniform_(sizes, cfg, seed), [=](double u) {
    return (std::pow(u, lambda) - std::pow(1.0 - u, lambda)) / lambda;
  }).to(cfg.platform, natural_cfg_(cfg, ScalarType::Float64).dtype);
}

Series ushape_pdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::u pdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    const double z = (y - t) / s;
    if (std::abs(z) >= 1.0) return 0.0;
    return 1.0 / (std::numbers::pi_v<double> * s * std::sqrt(1.0 - z * z));
  });
}

Series ushape_logpdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::u logpdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    const double z = (y - t) / s;
    if (std::abs(z) >= 1.0) return -std::numeric_limits<double>::infinity();
    return -std::log(std::numbers::pi_v<double> * s) - 0.5 * std::log(1.0 - z * z);
  });
}

Series ushape_cdf_(const Series& x, double t, double s) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::u cdf: expected s > 0");
  return map_series_f64_(x, [=](double y) {
    const double z = (y - t) / s;
    if (z <= -1.0) return 0.0;
    if (z >= 1.0) return 1.0;
    return 0.5 + std::asin(z) / std::numbers::pi_v<double>;
  });
}

Series ushape_sample_(double t, double s, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(s > 0.0, "subsp::stat::distr::u sample: expected s > 0");
  return map_series_f64_(unit_uniform_(sizes, cfg, seed), [=](double u) {
    return t + s * std::sin(std::numbers::pi_v<double> * (u - 0.5));
  }).to(cfg.platform, natural_cfg_(cfg, ScalarType::Float64).dtype);
}

Series vonmises_pdf_(const Series& x, double a, double k) {
  PLUSS_CHECK(k > 0.0, "subsp::stat::distr::vonmises pdf: expected k > 0");
  const double norm = 2.0 * std::numbers::pi_v<double> * bessel_i_series_scalar_(0.0, k);
  return map_series_f64_(x, [=](double y) {
    return std::exp(k * std::cos(wrap_angle_scalar_(y - a))) / norm;
  });
}

Series vonmises_logpdf_(const Series& x, double a, double k) {
  PLUSS_CHECK(k > 0.0, "subsp::stat::distr::vonmises logpdf: expected k > 0");
  const double log_norm = std::log(2.0 * std::numbers::pi_v<double> * bessel_i_series_scalar_(0.0, k));
  return map_series_f64_(x, [=](double y) {
    return k * std::cos(wrap_angle_scalar_(y - a)) - log_norm;
  });
}

Series vonmises_cdf_(const Series& x, double a, double k) {
  PLUSS_CHECK(k > 0.0, "subsp::stat::distr::vonmises cdf: expected k > 0");
  const double left = a - std::numbers::pi_v<double>;
  const auto pdf = [=](double y) {
    return std::exp(k * std::cos(wrap_angle_scalar_(y - a))) /
           (2.0 * std::numbers::pi_v<double> * bessel_i_series_scalar_(0.0, k));
  };
  return map_series_f64_(x, [=](double y) {
    if (y <= left) return 0.0;
    if (y >= left + 2.0 * std::numbers::pi_v<double>) return 1.0;
    return std::clamp(integrate_simpson_scalar_(pdf, left, y), 0.0, 1.0);
  });
}

Series vonmises_sample_(double a, double k, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(k > 0.0, "subsp::stat::distr::vonmises sample: expected k > 0");
  if (k > 700.0) {
    return normal_sample_(a, std::sqrt(1.0 / k), sizes, cfg, seed);
  }

  return sample_host_<double>(sizes, cfg, ScalarType::Float64, seed, [=](std::mt19937_64& rng) {
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    const double ub = uniform(rng);
    double acc = 0.0;
    std::int64_t x = 0;
    for (;; ++x) {
      acc += std::exp(bessel_logpmf_scalar_(x, 0.0, k));
      if (acc >= ub || 1.0 - acc < 1e-12) break;
    }

    const double u2 = uniform(rng);
    double b = 0.0;
    if (x > 0) {
      const double d = std::pow(std::cos(2.0 * std::numbers::pi_v<double> * u2), 2.0);
      const double u1 = uniform(rng);
      b = 1.0 - (1.0 - std::pow(u1, 1.0 / static_cast<double>(x))) * d;
    } else {
      b = std::pow(std::cos(std::numbers::pi_v<double> * u2), 2.0);
    }

    const double angle = (uniform(rng) < 1.0 / (1.0 + std::exp(-2.0 * k * std::sqrt(b))))
                             ? std::acos(std::sqrt(b))
                             : std::acos(-std::sqrt(b));
    const double sign = uniform(rng) < 0.5 ? -1.0 : 1.0;
    return a + sign * angle;
  });
}

Series weibull3_pdf_(const Series& x, double theta, double alpha, double beta) {
  return map_series_f64_(x, [=](double y) {
    if (y <= theta) return 0.0;
    const double z = (y - theta) / beta;
    return alpha / beta * std::pow(z, alpha - 1.0) * std::exp(-std::pow(z, alpha));
  });
}

Series weibull3_logpdf_(const Series& x, double theta, double alpha, double beta) {
  return map_series_f64_(x, [=](double y) {
    if (y <= theta) return -std::numeric_limits<double>::infinity();
    const double z = (y - theta) / beta;
    return std::log(alpha / beta) + (alpha - 1.0) * std::log(z) - std::pow(z, alpha);
  });
}

Series weibull3_cdf_(const Series& x, double theta, double alpha, double beta) {
  return map_series_f64_(x, [=](double y) {
    if (y <= theta) return 0.0;
    return 1.0 - std::exp(-std::pow((y - theta) / beta, alpha));
  });
}

Series weibull3_sample_(double theta, double alpha, double beta, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  return map_series_f64_(unit_uniform_(sizes, cfg, seed), [=](double u) {
    return theta + beta * std::pow(-std::log(u), 1.0 / alpha);
  }).to(cfg.platform, natural_cfg_(cfg, ScalarType::Float64).dtype);
}

Series yule_pdf_(const Series& x, double p) {
  PLUSS_CHECK(p > 1.0, "subsp::stat::distr::yule pdf: expected p > 1");
  return map_series_f64_(x, [=](double y) {
    const auto k = static_cast<std::int64_t>(std::floor(y));
    if (y != static_cast<double>(k) || k < 1) return 0.0;
    return (p - 1.0) * beta_scalar_(static_cast<double>(k), p);
  });
}

Series yule_logpdf_(const Series& x, double p) {
  PLUSS_CHECK(p > 1.0, "subsp::stat::distr::yule logpdf: expected p > 1");
  return map_series_f64_(x, [=](double y) {
    const auto k = static_cast<std::int64_t>(std::floor(y));
    if (y != static_cast<double>(k) || k < 1) return -std::numeric_limits<double>::infinity();
    return std::log(p - 1.0) + log_beta_scalar_(static_cast<double>(k), p);
  });
}

Series yule_cdf_(const Series& x, double p) {
  PLUSS_CHECK(p > 1.0, "subsp::stat::distr::yule cdf: expected p > 1");
  return map_series_f64_(x, [=](double y) {
    const auto k = static_cast<std::int64_t>(std::floor(y));
    if (k < 1) return 0.0;
    return 1.0 - (static_cast<double>(k) + 1.0) * beta_scalar_(static_cast<double>(k) + 1.0, p);
  });
}

Series yule_sample_(double p, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  PLUSS_CHECK(p > 1.0, "subsp::stat::distr::yule sample: expected p > 1");
  return sample_host_<std::int64_t>(sizes, cfg, ScalarType::Int64, seed, [=](std::mt19937_64& rng) {
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    const double u1 = uniform(rng);
    const double u2 = uniform(rng);
    return static_cast<std::int64_t>(std::ceil(std::log(u1) / std::log(1.0 - std::exp(std::log(u2) / (p - 1.0)))));
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
    case Family::Alpha:
      expect_param_count_("alpha sample", params, {2});
      return alpha_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::Anglit:
      expect_param_count_("anglit sample", params, {0, 2});
      return anglit_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, std::numbers::pi_v<double> * 0.25), sizes, cfg, seed);
    case Family::Arcsin:
      expect_param_count_("arcsin sample", params, {0});
      return arcsin_sample_(sizes, cfg, seed);
    case Family::Bessel:
      expect_param_count_("bessel sample", params, {2});
      return bessel_sample_(params[0], params[1], sizes, cfg, seed);
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
    case Family::Bradford:
      expect_param_count_("bradford sample", params, {1});
      return bradford_sample_(params[0], sizes, cfg, seed);
    case Family::Burr:
      expect_param_count_("burr sample", params, {2});
      return burr_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::Cauchy:
      expect_param_count_("cauchy sample", params, {0, 2});
      return cauchy_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, 1.0), sizes, cfg, seed);
    case Family::Chi:
      expect_param_count_("chi sample", params, {1, 3});
      return (params.size() == 1)
                 ? chi_sample_(0.0, 1.0, params[0], sizes, cfg, seed)
                 : chi_sample_(params[0], params[1], params[2], sizes, cfg, seed);
    case Family::ChiSquare:
      expect_param_count_("chisquare sample", params, {1});
      return chisq_sample_(static_cast<std::int64_t>(params[0]), sizes, cfg, seed);
    case Family::ChiSquareNoncentral:
      expect_param_count_("chisqnc sample", params, {2});
      return chisqnc_sample_(params[0], static_cast<std::int64_t>(params[1]), sizes, cfg, seed);
    case Family::Cosine:
      expect_param_count_("cosine sample", params, {0, 2});
      return cosine_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, std::numbers::pi_v<double>), sizes, cfg, seed);
    case Family::Erlang:
      expect_param_count_("erlang sample", params, {2});
      return erlang_sample_(params[0], static_cast<std::int64_t>(params[1]), sizes, cfg, seed);
    case Family::ExtremeValue:
      expect_param_count_("extrval sample", params, {2});
      return extreme_value_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::F:
      expect_param_count_("f sample", params, {2});
      return f_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::FNoncentral:
      expect_param_count_("fnc sample", params, {3});
      return fnc_sample_(params[0], params[1], params[2], sizes, cfg, seed);
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
    case Family::Gompertz:
      expect_param_count_("gompertz sample", params, {2});
      return gompertz_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::HalfCosine:
      expect_param_count_("halfcosine sample", params, {0, 2});
      return halfcosine_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, std::numbers::pi_v<double>), sizes, cfg, seed);
    case Family::HyperbolicSecant:
      expect_param_count_("hsec sample", params, {0, 2});
      return hsec_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, 1.0), sizes, cfg, seed);
    case Family::Hypergeometric:
      expect_param_count_("hypergeom sample", params, {3});
      return hypergeom_sample_(
          static_cast<std::int64_t>(params[0]), static_cast<std::int64_t>(params[1]), static_cast<std::int64_t>(params[2]), sizes, cfg, seed);
    case Family::InverseGaussian:
      expect_param_count_("ig sample", params, {2});
      return inverse_gaussian_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::Maxwell:
      expect_param_count_("maxwell sample", params, {1});
      return maxwell_sample_(params[0], sizes, cfg, seed);
    case Family::Nakagami:
      expect_param_count_("nakagami sample", params, {1, 2});
      return (params.size() == 1) ? nakagami_sample_(params[0], 1.0, sizes, cfg, seed) : nakagami_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::NegativeBinomial:
      expect_param_count_("negbinom sample", params, {2});
      return negbin_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::NormalTrunc:
      expect_param_count_("normaltrunc sample", params, {4});
      return normaltrunc_sample_(params[0], params[1], params[2], params[3], sizes, cfg, seed);
    case Family::Planck:
      expect_param_count_("planck sample", params, {2});
      return planck_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::Poisson:
      expect_param_count_("poisson sample", params, {1});
      return poisson_sample_(params[0], sizes, cfg, seed);
    case Family::Quadratic:
      expect_param_count_("quadratic sample", params, {0, 2});
      return quadratic_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, 1.0), sizes, cfg, seed);
    case Family::Rademacher:
      expect_param_count_("rademacher sample", params, {0});
      return rademacher_sample_(sizes, cfg, seed);
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
    case Family::Rice:
      expect_param_count_("rice sample", params, {2});
      return rice_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::Semicircle:
      expect_param_count_("semicircle sample", params, {2});
      return semicircle_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::Skellam:
      expect_param_count_("skellam sample", params, {2});
      return skellam_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::TukeyLambda:
      expect_param_count_("tukeylambda sample", params, {1});
      return tukeylambda_sample_(params[0], sizes, cfg, seed);
    case Family::UShape:
      expect_param_count_("u sample", params, {0, 2});
      return ushape_sample_(param_or_(params, 0, 0.0), param_or_(params, 1, 1.0), sizes, cfg, seed);
    case Family::VonMises:
      expect_param_count_("vonmises sample", params, {2});
      return vonmises_sample_(params[0], params[1], sizes, cfg, seed);
    case Family::Wald:
      expect_param_count_("wald sample", params, {1});
      return inverse_gaussian_sample_(1.0, params[0], sizes, cfg, seed);
    case Family::Weibull:
      expect_param_count_("weibull sample", params, {2, 3});
      return (params.size() == 2) ? weibull_sample_(params[0], params[1], sizes, cfg, seed)
                                  : weibull3_sample_(params[0], params[1], params[2], sizes, cfg, seed);
    case Family::Yule:
      expect_param_count_("yule sample", params, {1});
      return yule_sample_(params[0], sizes, cfg, seed);
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

Series sample(std::string_view name,
              std::span<const std::int64_t> sizes,
              Config cfg,
              std::uint64_t seed) {
  return sample(name, std::span<const double>{}, sizes, cfg, seed);
}

Series sample(std::string_view name,
              std::initializer_list<std::int64_t> sizes,
              Config cfg,
              std::uint64_t seed) {
  return sample(name, std::span<const double>{}, std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series sample_like(std::string_view name,
                   std::span<const double> params,
                   const Series& like,
                   std::uint64_t seed) {
  PLUSS_CHECK(like.defined(), "subsp::stat::distr::sample_like: expected defined Series");
  const Config cfg = Config{}
                         .with_dtype(like.scalar_type())
                         .with_platform(like.platform())
                         .with_pinned(like.is_pinned().item<bool>());
  return sample(name, params, like.sizes(), cfg, seed);
}

Series sample_like(std::string_view name,
                   std::initializer_list<double> params,
                   const Series& like,
                   std::uint64_t seed) {
  return sample_like(name, std::span<const double>(params.begin(), params.size()), like, seed);
}

Series sample_like(std::string_view name, const Series& like, std::uint64_t seed) {
  return sample_like(name, std::span<const double>{}, like, seed);
}

struct MixtureNormalParams {
  Series weights_cpu;
  Series weights;
  Series means;
  Series stddevs;
  std::int64_t num_components = 0;
};

MixtureNormalParams mixture_normal_params_(const Series& weights,
                                          const Series& means,
                                          const Series& stddevs,
                                          Config cfg,
                                          const char* op,
                                          bool allow_zero_stddev) {
  PLUSS_CHECK(weights.defined(), "subsp::stat::distr::mixture_normal: weights must be defined");
  PLUSS_CHECK(means.defined(), "subsp::stat::distr::mixture_normal: means must be defined");
  PLUSS_CHECK(stddevs.defined(), "subsp::stat::distr::mixture_normal: stddevs must be defined");

  const Series w = weights.to(pluss::pCPU, ScalarType::Float64).reshape({weights.numel()});
  const Series m_cpu = means.to(pluss::pCPU, ScalarType::Float64).reshape({means.numel()});
  const Series s_cpu = stddevs.to(pluss::pCPU, ScalarType::Float64).reshape({stddevs.numel()});

  PLUSS_CHECK(
      w.numel() == m_cpu.numel() && w.numel() == s_cpu.numel(),
      std::string("subsp::stat::distr::") + op + ": weights, means, stddevs must have same numel");
  PLUSS_CHECK(w.numel() > 0, std::string("subsp::stat::distr::") + op + ": expected at least one component");
  PLUSS_CHECK(s_cpu.amin(0, false).item<double>() >= (allow_zero_stddev ? 0.0 : std::numeric_limits<double>::min()),
              std::string("subsp::stat::distr::") + op + (allow_zero_stddev ? ": stddevs must be >= 0"
                                                                              : ": stddevs must be > 0"));

  const double total_weight = w.sum().item<double>();
  PLUSS_CHECK(total_weight > 0.0, std::string("subsp::stat::distr::") + op + ": weights must sum to > 0");
  const Series probs_cpu = w.div(total_weight);

  return MixtureNormalParams{
      .weights_cpu = probs_cpu,
      .weights = probs_cpu.to(cfg.platform, ScalarType::Float64),
      .means = m_cpu.to(cfg.platform, ScalarType::Float64),
      .stddevs = s_cpu.to(cfg.platform, ScalarType::Float64),
      .num_components = w.numel(),
  };
}

Series mixture_normal_component_pdf_(const Series& x,
                                     const Series& means,
                                     const Series& stddevs,
                                     const Series& zero,
                                     const Series& one) {
  const Series safe_std = stddevs.where(stddevs.gt(0.0), one);
  const Series z = x.sub(means).div(safe_std);
  Series out = z.square().mul(-0.5).exp().div(safe_std.mul(std::sqrt(2.0 * std::numbers::pi_v<double>)));
  const Series degenerate = stddevs.eq(0.0);
  if (degenerate.any().item<bool>()) {
    const Series point_mass = full_like_f64_(x, std::numeric_limits<double>::infinity()).where(x.eq(means), zero);
    out = point_mass.where(degenerate, out);
  }
  return out;
}

Series mixture_normal_component_cdf_(const Series& x,
                                     const Series& means,
                                     const Series& stddevs,
                                     const Series& zero,
                                     const Series& one) {
  const Series safe_std = stddevs.where(stddevs.gt(0.0), one);
  Series out = norm_cdf_(x.sub(means).div(safe_std));
  const Series degenerate = stddevs.eq(0.0);
  if (degenerate.any().item<bool>()) {
    const Series step = one.where(x.ge(means), zero);
    out = step.where(degenerate, out);
  }
  return out;
}

Series mixture_normal_component_logcdf_(const Series& x,
                                        const Series& means,
                                        const Series& stddevs,
                                        const Series& zero,
                                        const Series& one) {
  const Series safe_std = stddevs.where(stddevs.gt(0.0), one);
  Series out = norm_logcdf_(x.sub(means).div(safe_std));
  const Series degenerate = stddevs.eq(0.0);
  if (degenerate.any().item<bool>()) {
    const Series step_log = full_like_f64_(x, 0.0).where(x.ge(means), full_like_f64_(x, -std::numeric_limits<double>::infinity()));
    out = step_log.where(degenerate, out);
  }
  return out;
}

Series mixture_normal(const Series& weights,
                      const Series& means,
                      const Series& stddevs,
                      std::span<const std::int64_t> sizes,
                      Config cfg,
                      std::uint64_t seed,
                      bool use_sobol) {
  const MixtureNormalParams params = mixture_normal_params_(weights, means, stddevs, cfg, "mixture_normal", true);

  const std::vector<double> probs = params.weights_cpu.to_vec<double>();
  const std::int64_t k = params.num_components;
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
  const Series components = params.means.reshape({1, k}).add(params.stddevs.reshape({1, k}).mul(base));
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

Series mixture_normal_pdf(const Series& x,
                          const Series& weights,
                          const Series& means,
                          const Series& stddevs) {
  const Series xx = as_f64_(x, "mixture_normal_pdf");
  const Config cfg = float64_cfg_like_(xx);
  const MixtureNormalParams params = mixture_normal_params_(weights, means, stddevs, cfg, "mixture_normal_pdf", true);
  const std::int64_t total = xx.numel();
  const std::int64_t k = params.num_components;
  const Series flat_x = xx.reshape({total, 1});
  const Series means_2d = params.means.reshape({1, k});
  const Series stddevs_2d = params.stddevs.reshape({1, k});
  const Series weights_2d = params.weights.reshape({1, k});
  const Series zero = flat_x.mul(0.0).add(means_2d.mul(0.0));
  const Series one = zero.add(1.0);
  return mixture_normal_component_pdf_(flat_x, means_2d, stddevs_2d, zero, one)
      .mul(weights_2d)
      .sum(1, false)
      .reshape(xx.sizes());
}

Series mixture_normal_logpdf(const Series& x,
                             const Series& weights,
                             const Series& means,
                             const Series& stddevs) {
  const Series probs = mixture_normal_pdf(x, weights, means, stddevs);
  return probs.log().where(probs.gt(0.0), full_like_f64_(probs, -std::numeric_limits<double>::infinity()));
}

Series mixture_normal_cdf(const Series& x,
                          const Series& weights,
                          const Series& means,
                          const Series& stddevs) {
  const Series xx = as_f64_(x, "mixture_normal_cdf");
  const Config cfg = float64_cfg_like_(xx);
  const MixtureNormalParams params = mixture_normal_params_(weights, means, stddevs, cfg, "mixture_normal_cdf", true);
  const std::int64_t total = xx.numel();
  const std::int64_t k = params.num_components;
  const Series flat_x = xx.reshape({total, 1});
  const Series means_2d = params.means.reshape({1, k});
  const Series stddevs_2d = params.stddevs.reshape({1, k});
  const Series weights_2d = params.weights.reshape({1, k});
  const Series zero = flat_x.mul(0.0).add(means_2d.mul(0.0));
  const Series one = zero.add(1.0);
  return mixture_normal_component_cdf_(flat_x, means_2d, stddevs_2d, zero, one)
      .mul(weights_2d)
      .sum(1, false)
      .reshape(xx.sizes());
}

Series mixture_normal_logcdf(const Series& x,
                             const Series& weights,
                             const Series& means,
                             const Series& stddevs) {
  const Series xx = as_f64_(x, "mixture_normal_logcdf");
  const Config cfg = float64_cfg_like_(xx);
  const MixtureNormalParams params = mixture_normal_params_(weights, means, stddevs, cfg, "mixture_normal_logcdf", true);
  const std::int64_t total = xx.numel();
  const std::int64_t k = params.num_components;
  const Series flat_x = xx.reshape({total, 1});
  const Series means_2d = params.means.reshape({1, k});
  const Series stddevs_2d = params.stddevs.reshape({1, k});
  const Series weights_2d = params.weights.reshape({1, k});
  const Series zero = flat_x.mul(0.0).add(means_2d.mul(0.0));
  const Series one = zero.add(1.0);
  const Series log_weights = weights_2d.log().where(weights_2d.gt(0.0), full_like_f64_(weights_2d, -std::numeric_limits<double>::infinity()));
  return mixture_normal_component_logcdf_(flat_x, means_2d, stddevs_2d, zero, one)
      .add(log_weights)
      .logsumexp(1, false)
      .reshape(xx.sizes());
}

Series pdf(std::string_view name, const Series& x, std::span<const double> params) {
  switch (resolve_(name)) {
    case Family::Alpha:
      expect_param_count_("alpha pdf", params, {2});
      return alpha_pdf_(x, params[0], params[1]);
    case Family::Anglit:
      expect_param_count_("anglit pdf", params, {0, 2});
      return anglit_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, std::numbers::pi_v<double> * 0.25));
    case Family::Arcsin:
      expect_param_count_("arcsin pdf", params, {0});
      return arcsin_pdf_(x);
    case Family::Bessel:
      expect_param_count_("bessel pdf", params, {2});
      return bessel_pdf_(x, params[0], params[1]);
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
    case Family::Bradford:
      expect_param_count_("bradford pdf", params, {1});
      return bradford_pdf_(x, params[0]);
    case Family::Burr:
      expect_param_count_("burr pdf", params, {2});
      return burr_pdf_(x, params[0], params[1]);
    case Family::Cauchy:
      expect_param_count_("cauchy pdf", params, {0, 2});
      return cauchy_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Chi:
      expect_param_count_("chi pdf", params, {1, 3});
      return (params.size() == 1) ? chi_pdf_(x, 0.0, 1.0, params[0]) : chi_pdf_(x, params[0], params[1], params[2]);
    case Family::ChiSquare:
      expect_param_count_("chisquare pdf", params, {1});
      return chisq_pdf_(x, static_cast<std::int64_t>(params[0]));
    case Family::ChiSquareNoncentral:
      expect_param_count_("chisqnc pdf", params, {2});
      return chisqnc_pdf_(x, params[0], static_cast<std::int64_t>(params[1]));
    case Family::Cosine:
      expect_param_count_("cosine pdf", params, {0, 2});
      return cosine_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, std::numbers::pi_v<double>));
    case Family::Erlang:
      expect_param_count_("erlang pdf", params, {2});
      return erlang_pdf_(x, params[0], static_cast<std::int64_t>(params[1]));
    case Family::ExtremeValue:
      expect_param_count_("extrval pdf", params, {2});
      return extreme_value_pdf_(x, params[0], params[1]);
    case Family::F:
      expect_param_count_("f pdf", params, {2});
      return f_pdf_(x, params[0], params[1]);
    case Family::FNoncentral:
      expect_param_count_("fnc pdf", params, {3});
      return fnc_pdf_(x, params[0], params[1], params[2]);
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
    case Family::Gompertz:
      expect_param_count_("gompertz pdf", params, {2});
      return gompertz_pdf_(x, params[0], params[1]);
    case Family::HalfCosine:
      expect_param_count_("halfcosine pdf", params, {0, 2});
      return halfcosine_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, std::numbers::pi_v<double>));
    case Family::HyperbolicSecant:
      expect_param_count_("hsec pdf", params, {0, 2});
      return hsec_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Hypergeometric:
      expect_param_count_("hypergeom pdf", params, {3});
      return hypergeom_pdf_(x, static_cast<std::int64_t>(params[0]), static_cast<std::int64_t>(params[1]), static_cast<std::int64_t>(params[2]));
    case Family::InverseGaussian:
      expect_param_count_("ig pdf", params, {2});
      return inverse_gaussian_pdf_(x, params[0], params[1]);
    case Family::Maxwell:
      expect_param_count_("maxwell pdf", params, {1});
      return maxwell_pdf_(x, params[0]);
    case Family::Nakagami:
      expect_param_count_("nakagami pdf", params, {1, 2});
      return (params.size() == 1) ? nakagami_pdf_(x, params[0], 1.0) : nakagami_pdf_(x, params[0], params[1]);
    case Family::NegativeBinomial:
      expect_param_count_("negbinom pdf", params, {2});
      return negbin_pdf_(x, params[0], params[1]);
    case Family::NormalTrunc:
      expect_param_count_("normaltrunc pdf", params, {4});
      return normaltrunc_pdf_(x, params[0], params[1], params[2], params[3]);
    case Family::Planck:
      expect_param_count_("planck pdf", params, {2});
      return planck_pdf_(x, params[0], params[1]);
    case Family::Poisson:
      expect_param_count_("poisson pdf", params, {1});
      return poisson_pdf_(x, params[0]);
    case Family::Quadratic:
      expect_param_count_("quadratic pdf", params, {0, 2});
      return quadratic_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Rademacher:
      expect_param_count_("rademacher pdf", params, {0});
      return rademacher_pdf_(x);
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
    case Family::Rice:
      expect_param_count_("rice pdf", params, {2});
      return rice_pdf_(x, params[0], params[1]);
    case Family::Semicircle:
      expect_param_count_("semicircle pdf", params, {2});
      return semicircle_pdf_(x, params[0], params[1]);
    case Family::Skellam:
      expect_param_count_("skellam pdf", params, {2});
      return skellam_pdf_(x, params[0], params[1]);
    case Family::TukeyLambda:
      expect_param_count_("tukeylambda pdf", params, {1});
      return tukeylambda_pdf_(x, params[0]);
    case Family::UShape:
      expect_param_count_("u pdf", params, {0, 2});
      return ushape_pdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::VonMises:
      expect_param_count_("vonmises pdf", params, {2});
      return vonmises_pdf_(x, params[0], params[1]);
    case Family::Wald:
      expect_param_count_("wald pdf", params, {1});
      return inverse_gaussian_pdf_(x, 1.0, params[0]);
    case Family::Weibull:
      expect_param_count_("weibull pdf", params, {2, 3});
      return (params.size() == 2) ? weibull_pdf_(x, params[0], params[1]) : weibull3_pdf_(x, params[0], params[1], params[2]);
    case Family::Yule:
      expect_param_count_("yule pdf", params, {1});
      return yule_pdf_(x, params[0]);
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
    case Family::Alpha:
      expect_param_count_("alpha logpdf", params, {2});
      return alpha_logpdf_(x, params[0], params[1]);
    case Family::Anglit:
      expect_param_count_("anglit logpdf", params, {0, 2});
      return anglit_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, std::numbers::pi_v<double> * 0.25));
    case Family::Arcsin:
      expect_param_count_("arcsin logpdf", params, {0});
      return arcsin_logpdf_(x);
    case Family::Bessel:
      expect_param_count_("bessel logpdf", params, {2});
      return bessel_logpdf_(x, params[0], params[1]);
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
    case Family::Bradford:
      expect_param_count_("bradford logpdf", params, {1});
      return bradford_logpdf_(x, params[0]);
    case Family::Burr:
      expect_param_count_("burr logpdf", params, {2});
      return burr_logpdf_(x, params[0], params[1]);
    case Family::Cauchy:
      expect_param_count_("cauchy logpdf", params, {0, 2});
      return cauchy_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Chi:
      expect_param_count_("chi logpdf", params, {1, 3});
      return (params.size() == 1) ? chi_logpdf_(x, 0.0, 1.0, params[0]) : chi_logpdf_(x, params[0], params[1], params[2]);
    case Family::ChiSquare:
      expect_param_count_("chisquare logpdf", params, {1});
      return chisq_logpdf_(x, static_cast<std::int64_t>(params[0]));
    case Family::ChiSquareNoncentral:
      expect_param_count_("chisqnc logpdf", params, {2});
      return chisqnc_logpdf_(x, params[0], static_cast<std::int64_t>(params[1]));
    case Family::Cosine:
      expect_param_count_("cosine logpdf", params, {0, 2});
      return cosine_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, std::numbers::pi_v<double>));
    case Family::Erlang:
      expect_param_count_("erlang logpdf", params, {2});
      return erlang_logpdf_(x, params[0], static_cast<std::int64_t>(params[1]));
    case Family::ExtremeValue:
      expect_param_count_("extrval logpdf", params, {2});
      return extreme_value_logpdf_(x, params[0], params[1]);
    case Family::F:
      expect_param_count_("f logpdf", params, {2});
      return f_logpdf_(x, params[0], params[1]);
    case Family::FNoncentral:
      expect_param_count_("fnc logpdf", params, {3});
      return fnc_logpdf_(x, params[0], params[1], params[2]);
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
    case Family::Gompertz:
      expect_param_count_("gompertz logpdf", params, {2});
      return gompertz_logpdf_(x, params[0], params[1]);
    case Family::HalfCosine:
      expect_param_count_("halfcosine logpdf", params, {0, 2});
      return halfcosine_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, std::numbers::pi_v<double>));
    case Family::HyperbolicSecant:
      expect_param_count_("hsec logpdf", params, {0, 2});
      return hsec_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Hypergeometric:
      expect_param_count_("hypergeom logpdf", params, {3});
      return hypergeom_logpdf_(x, static_cast<std::int64_t>(params[0]), static_cast<std::int64_t>(params[1]), static_cast<std::int64_t>(params[2]));
    case Family::InverseGaussian:
      expect_param_count_("ig logpdf", params, {2});
      return inverse_gaussian_logpdf_(x, params[0], params[1]);
    case Family::Maxwell:
      expect_param_count_("maxwell logpdf", params, {1});
      return maxwell_logpdf_(x, params[0]);
    case Family::Nakagami:
      expect_param_count_("nakagami logpdf", params, {1, 2});
      return (params.size() == 1) ? nakagami_logpdf_(x, params[0], 1.0) : nakagami_logpdf_(x, params[0], params[1]);
    case Family::NegativeBinomial:
      expect_param_count_("negbinom logpdf", params, {2});
      return negbin_logpdf_(x, params[0], params[1]);
    case Family::NormalTrunc:
      expect_param_count_("normaltrunc logpdf", params, {4});
      return normaltrunc_logpdf_(x, params[0], params[1], params[2], params[3]);
    case Family::Planck:
      expect_param_count_("planck logpdf", params, {2});
      return planck_logpdf_(x, params[0], params[1]);
    case Family::Poisson:
      expect_param_count_("poisson logpdf", params, {1});
      return poisson_logpdf_(x, params[0]);
    case Family::Quadratic:
      expect_param_count_("quadratic logpdf", params, {0, 2});
      return quadratic_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Rademacher:
      expect_param_count_("rademacher logpdf", params, {0});
      return rademacher_logpdf_(x);
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
    case Family::Rice:
      expect_param_count_("rice logpdf", params, {2});
      return rice_logpdf_(x, params[0], params[1]);
    case Family::Semicircle:
      expect_param_count_("semicircle logpdf", params, {2});
      return semicircle_logpdf_(x, params[0], params[1]);
    case Family::Skellam:
      expect_param_count_("skellam logpdf", params, {2});
      return skellam_logpdf_(x, params[0], params[1]);
    case Family::TukeyLambda:
      expect_param_count_("tukeylambda logpdf", params, {1});
      return tukeylambda_logpdf_(x, params[0]);
    case Family::UShape:
      expect_param_count_("u logpdf", params, {0, 2});
      return ushape_logpdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::VonMises:
      expect_param_count_("vonmises logpdf", params, {2});
      return vonmises_logpdf_(x, params[0], params[1]);
    case Family::Wald:
      expect_param_count_("wald logpdf", params, {1});
      return inverse_gaussian_logpdf_(x, 1.0, params[0]);
    case Family::Weibull:
      expect_param_count_("weibull logpdf", params, {2, 3});
      return (params.size() == 2) ? weibull_logpdf_(x, params[0], params[1]) : weibull3_logpdf_(x, params[0], params[1], params[2]);
    case Family::Yule:
      expect_param_count_("yule logpdf", params, {1});
      return yule_logpdf_(x, params[0]);
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
    case Family::Alpha:
      expect_param_count_("alpha cdf", params, {2});
      return alpha_cdf_(x, params[0], params[1]);
    case Family::Anglit:
      expect_param_count_("anglit cdf", params, {0, 2});
      return anglit_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, std::numbers::pi_v<double> * 0.25));
    case Family::Arcsin:
      expect_param_count_("arcsin cdf", params, {0});
      return arcsin_cdf_(x);
    case Family::Bessel:
      expect_param_count_("bessel cdf", params, {2});
      return bessel_cdf_(x, params[0], params[1]);
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
    case Family::Bradford:
      expect_param_count_("bradford cdf", params, {1});
      return bradford_cdf_(x, params[0]);
    case Family::Burr:
      expect_param_count_("burr cdf", params, {2});
      return burr_cdf_(x, params[0], params[1]);
    case Family::Cauchy:
      expect_param_count_("cauchy cdf", params, {0, 2});
      return cauchy_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Chi:
      expect_param_count_("chi cdf", params, {1, 3});
      return (params.size() == 1) ? chi_cdf_(x, 0.0, 1.0, params[0]) : chi_cdf_(x, params[0], params[1], params[2]);
    case Family::ChiSquare:
      expect_param_count_("chisquare cdf", params, {1});
      return chisq_cdf_(x, static_cast<std::int64_t>(params[0]));
    case Family::ChiSquareNoncentral:
      expect_param_count_("chisqnc cdf", params, {2});
      return chisqnc_cdf_(x, params[0], static_cast<std::int64_t>(params[1]));
    case Family::Cosine:
      expect_param_count_("cosine cdf", params, {0, 2});
      return cosine_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, std::numbers::pi_v<double>));
    case Family::Erlang:
      expect_param_count_("erlang cdf", params, {2});
      return erlang_cdf_(x, params[0], static_cast<std::int64_t>(params[1]));
    case Family::ExtremeValue:
      expect_param_count_("extrval cdf", params, {2});
      return extreme_value_cdf_(x, params[0], params[1]);
    case Family::F:
      expect_param_count_("f cdf", params, {2});
      return f_cdf_(x, params[0], params[1]);
    case Family::FNoncentral:
      expect_param_count_("fnc cdf", params, {3});
      return fnc_cdf_(x, params[0], params[1], params[2]);
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
    case Family::Gompertz:
      expect_param_count_("gompertz cdf", params, {2});
      return gompertz_cdf_(x, params[0], params[1]);
    case Family::HalfCosine:
      expect_param_count_("halfcosine cdf", params, {0, 2});
      return halfcosine_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, std::numbers::pi_v<double>));
    case Family::HyperbolicSecant:
      expect_param_count_("hsec cdf", params, {0, 2});
      return hsec_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Hypergeometric:
      expect_param_count_("hypergeom cdf", params, {3});
      return hypergeom_cdf_(x, static_cast<std::int64_t>(params[0]), static_cast<std::int64_t>(params[1]), static_cast<std::int64_t>(params[2]));
    case Family::InverseGaussian:
      expect_param_count_("ig cdf", params, {2});
      return inverse_gaussian_cdf_(x, params[0], params[1]);
    case Family::Maxwell:
      expect_param_count_("maxwell cdf", params, {1});
      return maxwell_cdf_(x, params[0]);
    case Family::Nakagami:
      expect_param_count_("nakagami cdf", params, {1, 2});
      return (params.size() == 1) ? nakagami_cdf_(x, params[0], 1.0) : nakagami_cdf_(x, params[0], params[1]);
    case Family::NegativeBinomial:
      expect_param_count_("negbinom cdf", params, {2});
      return negbin_cdf_(x, params[0], params[1]);
    case Family::NormalTrunc:
      expect_param_count_("normaltrunc cdf", params, {4});
      return normaltrunc_cdf_(x, params[0], params[1], params[2], params[3]);
    case Family::Planck:
      expect_param_count_("planck cdf", params, {2});
      return planck_cdf_(x, params[0], params[1]);
    case Family::Poisson:
      expect_param_count_("poisson cdf", params, {1});
      return poisson_cdf_(x, params[0]);
    case Family::Quadratic:
      expect_param_count_("quadratic cdf", params, {0, 2});
      return quadratic_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Rademacher:
      expect_param_count_("rademacher cdf", params, {0});
      return rademacher_cdf_(x);
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
    case Family::Rice:
      expect_param_count_("rice cdf", params, {2});
      return rice_cdf_(x, params[0], params[1]);
    case Family::Semicircle:
      expect_param_count_("semicircle cdf", params, {2});
      return semicircle_cdf_(x, params[0], params[1]);
    case Family::Skellam:
      expect_param_count_("skellam cdf", params, {2});
      return skellam_cdf_(x, params[0], params[1]);
    case Family::TukeyLambda:
      expect_param_count_("tukeylambda cdf", params, {1});
      return tukeylambda_cdf_(x, params[0]);
    case Family::UShape:
      expect_param_count_("u cdf", params, {0, 2});
      return ushape_cdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::VonMises:
      expect_param_count_("vonmises cdf", params, {2});
      return vonmises_cdf_(x, params[0], params[1]);
    case Family::Wald:
      expect_param_count_("wald cdf", params, {1});
      return inverse_gaussian_cdf_(x, 1.0, params[0]);
    case Family::Weibull:
      expect_param_count_("weibull cdf", params, {2, 3});
      return (params.size() == 2) ? weibull_cdf_(x, params[0], params[1]) : weibull3_cdf_(x, params[0], params[1], params[2]);
    case Family::Yule:
      expect_param_count_("yule cdf", params, {1});
      return yule_cdf_(x, params[0]);
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

Series logcdf(std::string_view name, const Series& x, std::span<const double> params) {
  switch (resolve_(name)) {
    case Family::Normal:
      expect_param_count_("normal logcdf", params, {0, 2});
      return normal_logcdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Lognormal:
      expect_param_count_("lognormal logcdf", params, {0, 2});
      return lognormal_logcdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Uniform:
      expect_param_count_("uniform logcdf", params, {2});
      return uniform_logcdf_(x, params[0], params[1]);
    case Family::Cauchy:
      expect_param_count_("cauchy logcdf", params, {0, 2});
      return cauchy_logcdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Laplace:
      expect_param_count_("laplace logcdf", params, {0, 2});
      return laplace_logcdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Logistic:
      expect_param_count_("logistic logcdf", params, {0, 2});
      return logistic_logcdf_(x, param_or_(params, 0, 0.0), param_or_(params, 1, 1.0));
    case Family::Exponential: {
      expect_param_count_("exponential logcdf", params, {0, 1});
      const double rate = param_or_(params, 0, 1.0);
      PLUSS_CHECK(rate > 0.0, "subsp::stat::distr::exponential logcdf: rate must be > 0");
      const Series xx = as_f64_(x, "logcdf");
      const Series pos = xx.gt(0.0);
      const Series core = xx.mul(-rate).exp().neg().log1p();
      return core.where(pos, full_like_f64_(xx, -std::numeric_limits<double>::infinity()));
    }
    case Family::Rayleigh:
      expect_param_count_("rayleigh logcdf", params, {0, 1});
      return rayleigh_logcdf_(x, param_or_(params, 0, 1.0));
    case Family::NormalTrunc:
      expect_param_count_("normaltrunc logcdf", params, {4});
      return normaltrunc_logcdf_(x, params[0], params[1], params[2], params[3]);
    default:
      return cdf(name, x, params).log();
  }
}

Series logcdf(std::string_view name, const Series& x, std::initializer_list<double> params) {
  return logcdf(name, x, std::span<const double>(params.begin(), params.size()));
}

} // namespace pluss::subsp::stat::distr
