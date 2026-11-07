#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <pluss/ma.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using namespace pluss::subsp::stats::distr;

constexpr double kTol = 1e-12;

double as_f64_scalar(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

std::vector<double> as_f64_vec(const Series& x) {
  const Series cpu = x.to(pluss::pCPU, ScalarType::Float64);
  return cpu.reshape({cpu.numel()}).pull<double>();
}

std::vector<std::int64_t> as_shape_vec(const Series& x) {
  return std::vector<std::int64_t>(x.sizes().begin(), x.sizes().end());
}

template <typename Dist>
void require_cumulant_close(const Dist& dist, std::int64_t order, double expected, double tol = kTol) {
  const double actual = as_f64_scalar(dist.cumulant(order));
  INFO("order=" << order << " actual=" << actual << " expected=" << expected);
  REQUIRE(std::isfinite(actual));
  REQUIRE(std::abs(actual - expected) <= tol);
}

void require_allclose(std::string_view label, const Series& actual, const Series& expected, double tol = kTol) {
  const auto av = as_f64_vec(actual);
  const auto ev = as_f64_vec(expected);
  REQUIRE(av.size() == ev.size());
  for (std::size_t i = 0; i < av.size(); ++i) {
    INFO("label=" << label << " index=" << i << " actual=" << av[i] << " expected=" << ev[i]);
    if (!std::isfinite(ev[i])) {
      REQUIRE(std::isinf(av[i]));
      REQUIRE(std::signbit(av[i]) == std::signbit(ev[i]));
      continue;
    }
    REQUIRE(std::isfinite(av[i]));
    REQUIRE(std::abs(av[i] - ev[i]) <= tol);
  }
}

template <typename Dist, typename ParamArray>
void require_functional_parity(const Dist& dist,
                               dst::Kind kind,
                               const Series& x,
                               const ParamArray& params,
                               double tol = kTol) {
  const std::string label = "kind=" + std::to_string(static_cast<int>(kind));
  require_allclose(label + "::pdf", dist.pdf(x), pdf(kind, x, params), tol);
  require_allclose(label + "::logpdf", dist.logpdf(x), logpdf(kind, x, params), tol);
  require_allclose(label + "::cdf", dist.cdf(x), cdf(kind, x, params), tol);
  require_allclose(label + "::logcdf", dist.logcdf(x), logcdf(kind, x, params), tol);
}

double uniform_cgf_expected(double low, double high, double t) {
  if (t == 0.0) {
    return 0.0;
  }
  return low * t + std::log(std::expm1((high - low) * t) / ((high - low) * t));
}

double nig_cumulant_expected(double alpha, double beta, double location, double scale, std::int64_t order) {
  const long double alpha_ld = static_cast<long double>(alpha);
  const long double beta_ld = static_cast<long double>(beta);
  const long double location_ld = static_cast<long double>(location);
  const long double scale_ld = static_cast<long double>(scale);
  const long double gamma = std::sqrt(alpha_ld * alpha_ld - beta_ld * beta_ld);
  std::vector<long double> coeffs(static_cast<std::size_t>(order) + 1, 0.0L);
  coeffs[0] = gamma;
  for (std::int64_t n = 1; n <= order; ++n) {
    long double acc = 0.0L;
    for (std::int64_t k = 1; k < n; ++k) {
      acc += coeffs[static_cast<std::size_t>(k)] * coeffs[static_cast<std::size_t>(n - k)];
    }
    const long double rhs = n == 1 ? -2.0L * beta_ld : (n == 2 ? -1.0L : 0.0L);
    coeffs[static_cast<std::size_t>(n)] = (rhs - acc) / (2.0L * gamma);
  }
  long double factorial = 1.0L;
  for (std::int64_t i = 2; i <= order; ++i) {
    factorial *= static_cast<long double>(i);
  }
  long double cumulant = -scale_ld * factorial * coeffs[static_cast<std::size_t>(order)];
  if (order == 1) {
    cumulant += location_ld;
  }
  return static_cast<double>(cumulant);
}

double rayleigh_cumulant_expected(double sigma, std::int64_t order) {
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) {
    raw[static_cast<std::size_t>(n)] =
        std::pow(sigma, static_cast<double>(n)) *
        std::pow(2.0, 0.5 * static_cast<double>(n)) *
        std::tgamma(1.0 + 0.5 * static_cast<double>(n));
  }
  std::vector<double> cumulants(raw.size(), 0.0);
  for (std::size_t n = 1; n < raw.size(); ++n) {
    double value = raw[n];
    for (std::size_t m = 1; m < n; ++m) {
      double binom = 1.0;
      for (std::size_t i = 1; i <= m - 1; ++i) {
        binom *= static_cast<double>(n - i);
        binom /= static_cast<double>(i);
      }
      value -= binom * cumulants[m] * raw[n - m];
    }
    cumulants[n] = value;
  }
  return cumulants[static_cast<std::size_t>(order)];
}

double stirling_second_kind_expected(std::int64_t n, std::int64_t k) {
  if (n < 0 || k < 0 || k > n) return 0.0;
  std::vector<long double> row(static_cast<std::size_t>(k) + 1, 0.0L);
  row[0] = 1.0L;
  for (std::int64_t i = 1; i <= n; ++i) {
    const std::int64_t upper = std::min(i, k);
    for (std::int64_t j = upper; j >= 1; --j) {
      row[static_cast<std::size_t>(j)] =
          static_cast<long double>(j) * row[static_cast<std::size_t>(j)] +
          row[static_cast<std::size_t>(j - 1)];
    }
    row[0] = 0.0L;
  }
  return static_cast<double>(row[static_cast<std::size_t>(k)]);
}

double factorial_expected(std::int64_t n) {
  double out = 1.0;
  for (std::int64_t i = 2; i <= n; ++i) out *= static_cast<double>(i);
  return out;
}

double negbinom_cumulant_expected(double r, double p, std::int64_t order) {
  const double q = 1.0 - p;
  if (order == 1) {
    return r * q / p;
  }
  double total = 0.0;
  for (std::int64_t k = 1; k <= order - 1; ++k) {
    total += stirling_second_kind_expected(order - 1, k) * factorial_expected(k) *
             std::pow(q, static_cast<double>(k)) /
             std::pow(p, static_cast<double>(k + 1));
  }
  return r * total;
}

double std_normal_pdf_expected(double x) {
  return std::exp(-0.5 * x * x) / std::sqrt(2.0 * std::numbers::pi_v<double>);
}

double std_normal_cdf_expected(double x) {
  return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

double normaltrunc_cumulant_expected(double lower, double upper, double mean, double stddev, std::int64_t order) {
  const double alpha = (lower - mean) / stddev;
  const double beta = (upper - mean) / stddev;
  const double z = std_normal_cdf_expected(beta) - std_normal_cdf_expected(alpha);
  const double phi_alpha = std_normal_pdf_expected(alpha);
  const double phi_beta = std_normal_pdf_expected(beta);
  std::vector<double> std_raw(static_cast<std::size_t>(order) + 1, 0.0);
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  std_raw[0] = 1.0;
  if (order >= 1) {
    std_raw[1] = (phi_alpha - phi_beta) / z;
  }
  for (std::int64_t n = 2; n <= order; ++n) {
    std_raw[static_cast<std::size_t>(n)] =
        static_cast<double>(n - 1) * std_raw[static_cast<std::size_t>(n - 2)] +
        (std::pow(alpha, static_cast<double>(n - 1)) * phi_alpha -
         std::pow(beta, static_cast<double>(n - 1)) * phi_beta) / z;
  }
  for (std::int64_t n = 1; n <= order; ++n) {
    double moment = 0.0;
    for (std::int64_t k = 0; k <= n; ++k) {
      double binom = 1.0;
      for (std::int64_t i = 1; i <= k; ++i) {
        binom *= static_cast<double>(n - k + i);
        binom /= static_cast<double>(i);
      }
      moment += binom * std::pow(mean, static_cast<double>(n - k)) *
                std::pow(stddev, static_cast<double>(k)) *
                std_raw[static_cast<std::size_t>(k)];
    }
    raw[static_cast<std::size_t>(n)] = moment;
  }
  std::vector<double> cumulants(raw.size(), 0.0);
  for (std::size_t n = 1; n < raw.size(); ++n) {
    double value = raw[n];
    for (std::size_t m = 1; m < n; ++m) {
      double binom = 1.0;
      for (std::size_t i = 1; i <= m - 1; ++i) {
        binom *= static_cast<double>(n - i);
        binom /= static_cast<double>(i);
      }
      value -= binom * cumulants[m] * raw[n - m];
    }
    cumulants[n] = value;
  }
  return cumulants[static_cast<std::size_t>(order)];
}

double inverse_gaussian_cumulant_expected(double theta, double chi, std::int64_t order) {
  double odd_df = 1.0;
  for (std::int64_t k = 2 * order - 3; k >= 1; k -= 2) {
    odd_df *= static_cast<double>(k);
  }
  return odd_df * std::pow(theta, static_cast<double>(2 * order - 1)) /
         std::pow(chi, static_cast<double>(order - 1));
}

double student_t_even_raw_moment_expected(double nu, std::int64_t order) {
  if (order % 2 == 1) return 0.0;
  const double m = 0.5 * static_cast<double>(order);
  return std::pow(nu, m) *
         std::tgamma(m + 0.5) *
         std::tgamma(0.5 * (nu - static_cast<double>(order))) /
         (std::sqrt(std::numbers::pi_v<double>) * std::tgamma(0.5 * nu));
}

double beta_raw_moment_expected(double low, double high, double alpha, double beta, std::int64_t order) {
  const double width = high - low;
  double moment = 0.0;
  for (std::int64_t k = 0; k <= order; ++k) {
    double rising_alpha = 1.0;
    double rising_sum = 1.0;
    for (std::int64_t i = 0; i < k; ++i) {
      rising_alpha *= alpha + static_cast<double>(i);
      rising_sum *= alpha + beta + static_cast<double>(i);
    }
    moment += std::tgamma(static_cast<double>(order + 1)) /
              (std::tgamma(static_cast<double>(k + 1)) * std::tgamma(static_cast<double>(order - k + 1))) *
              std::pow(low, static_cast<double>(order - k)) *
              std::pow(width, static_cast<double>(k)) *
              rising_alpha / rising_sum;
  }
  return moment;
}

} // namespace

static_assert(std::is_same_v<decltype(sample(dst::Normal,
                                             std::initializer_list<double>{0.0, 1.0},
                                             std::initializer_list<std::int64_t>{2},
                                             Config{},
                                             0ULL)),
                             Series>);
static_assert(std::is_same_v<decltype(sample_like(dst::Normal,
                                                  std::initializer_list<double>{0.0, 1.0},
                                                  std::declval<const Series&>(),
                                                  0ULL)),
                             Series>);
static_assert(std::is_same_v<decltype(pdf(dst::Normal, std::declval<const Series&>(), std::initializer_list<double>{0.0, 1.0})),
                             Series>);
static_assert(std::is_same_v<decltype(logpdf(dst::Normal, std::declval<const Series&>(), std::initializer_list<double>{0.0, 1.0})),
                             Series>);
static_assert(std::is_same_v<decltype(cdf(dst::Normal, std::declval<const Series&>(), std::initializer_list<double>{0.0, 1.0})),
                             Series>);
static_assert(std::is_same_v<decltype(logcdf(dst::Normal, std::declval<const Series&>(), std::initializer_list<double>{0.0, 1.0})),
                             Series>);
static_assert(std::is_same_v<decltype(mixture_normal_logcdf(std::declval<const Series&>(),
                                                            std::declval<const Series&>(),
                                                            std::declval<const Series&>(),
                                                            std::declval<const Series&>())),
                             Series>);

TEST_CASE("distribution objects stay aligned with functional pdf/logpdf/cdf routes", "[distr][objects][parity]") {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  require_functional_parity(Normal{0.5, 1.25},
                            dst::Normal,
                            pluss::make_series(std::vector<double>{-1.0, 0.0, 1.0}, {3}, f64),
                            std::initializer_list<double>{0.5, 1.25});
  require_functional_parity(Bernoulli{0.25},
                            dst::Bernoulli,
                            pluss::make_series(std::vector<double>{0.0, 1.0}, {2}, f64),
                            std::initializer_list<double>{0.25});
  require_functional_parity(Binomial{4, 0.25},
                            dst::Binomial,
                            pluss::make_series(std::vector<double>{0.0, 1.0, 2.0}, {3}, f64),
                            std::initializer_list<double>{4.0, 0.25});
  require_functional_parity(Gamma{2.0, 1.5, 0.25},
                            dst::Gamma,
                            pluss::make_series(std::vector<double>{0.5, 1.0, 2.0}, {3}, f64),
                            std::initializer_list<double>{0.25, 1.5, 2.0});
  require_functional_parity(ChiSquare{5},
                            dst::ChiSquare,
                            pluss::make_series(std::vector<double>{0.5, 2.0, 4.0}, {3}, f64),
                            std::initializer_list<double>{5.0});
  require_functional_parity(InverseGaussian{1.5, 3.0},
                            dst::InverseGaussian,
                            pluss::make_series(std::vector<double>{0.5, 1.5, 3.0}, {3}, f64),
                            std::initializer_list<double>{1.5, 3.0},
                            1e-10);
  require_functional_parity(Lognormal{0.1, 0.4},
                            dst::Lognormal,
                            pluss::make_series(std::vector<double>{0.5, 1.0, 2.0}, {3}, f64),
                            std::initializer_list<double>{0.1, 0.4},
                            1e-10);
  require_functional_parity(Uniform{-2.0, 3.0},
                            dst::Uniform,
                            pluss::make_series(std::vector<double>{-2.0, 0.0, 2.0}, {3}, f64),
                            std::initializer_list<double>{-2.0, 3.0});
  require_functional_parity(Exponential{2.0},
                            dst::Exponential,
                            pluss::make_series(std::vector<double>{0.0, 0.5, 1.0}, {3}, f64),
                            std::initializer_list<double>{2.0});
  require_functional_parity(Laplace{0.5, 0.75},
                            dst::Laplace,
                            pluss::make_series(std::vector<double>{-1.0, 0.5, 2.0}, {3}, f64),
                            std::initializer_list<double>{0.5, 0.75});
  require_functional_parity(Logistic{-0.25, 0.8},
                            dst::Logistic,
                            pluss::make_series(std::vector<double>{-2.0, 0.0, 2.0}, {3}, f64),
                            std::initializer_list<double>{-0.25, 0.8});
  require_functional_parity(Rayleigh{1.25},
                            dst::Rayleigh,
                            pluss::make_series(std::vector<double>{0.25, 1.0, 2.5}, {3}, f64),
                            std::initializer_list<double>{1.25});
  require_functional_parity(NormalTrunc{-1.0, 1.0, 0.0, 1.0},
                            dst::NormalTrunc,
                            pluss::make_series(std::vector<double>{-1.0, 0.0, 0.5}, {3}, f64),
                            std::initializer_list<double>{-1.0, 1.0, 0.0, 1.0},
                            1e-10);
  require_functional_parity(StudentT{9.0, 0.25, 1.5},
                            dst::StudentT,
                            pluss::make_series(std::vector<double>{-1.0, 0.25, 2.0}, {3}, f64),
                            std::initializer_list<double>{9.0, 0.25, 1.5},
                            1e-10);
  require_functional_parity(FDistribution{5.0, 12.0},
                            dst::F,
                            pluss::make_series(std::vector<double>{0.25, 1.0, 3.0}, {3}, f64),
                            std::initializer_list<double>{5.0, 12.0},
                            1e-10);
  require_functional_parity(Weibull{2.0, 1.5},
                            dst::Weibull,
                            pluss::make_series(std::vector<double>{0.25, 1.0, 3.0}, {3}, f64),
                            std::initializer_list<double>{2.0, 1.5},
                            1e-10);
  require_functional_parity(Beta{0.0, 1.0, 2.0, 3.0},
                            dst::Beta,
                            pluss::make_series(std::vector<double>{0.1, 0.5, 0.9}, {3}, f64),
                            std::initializer_list<double>{0.0, 1.0, 2.0, 3.0},
                            1e-10);
  require_functional_parity(Skellam{4.0, 1.5},
                            dst::Skellam,
                            pluss::make_series(std::vector<double>{-1.0, 0.0, 2.0}, {3}, f64),
                            std::initializer_list<double>{4.0, 1.5},
                            1e-9);
  require_functional_parity(Poisson{2.5},
                            dst::Poisson,
                            pluss::make_series(std::vector<double>{0.0, 1.0, 4.0}, {3}, f64),
                            std::initializer_list<double>{2.5},
                            1e-12);
  require_functional_parity(Geometric{0.25},
                            dst::Geometric,
                            pluss::make_series(std::vector<double>{1.0, 2.0, 5.0}, {3}, f64),
                            std::initializer_list<double>{0.25},
                            1e-12);
  require_functional_parity(NegativeBinomial{3.0, 0.4},
                            dst::NegativeBinomial,
                            pluss::make_series(std::vector<double>{0.0, 1.0, 4.0}, {3}, f64),
                            std::initializer_list<double>{3.0, 0.4},
                            1e-12);
  require_functional_parity(Nig{2.0, 0.5, 0.0, 1.0},
                            dst::Nig,
                            pluss::make_series(std::vector<double>{-1.0, 0.0, 1.0}, {3}, f64),
                            std::initializer_list<double>{2.0, 0.5, 0.0, 1.0},
                            1e-8);
  require_functional_parity(Gh{1.0, 2.0, 0.5, 0.0, 1.0},
                            dst::Gh,
                            pluss::make_series(std::vector<double>{-1.0, 0.0, 1.0}, {3}, f64),
                            std::initializer_list<double>{1.0, 2.0, 0.5, 0.0, 1.0},
                            1e-8);

  const Gh gh_as_nig{-0.5, 2.0, 0.5, 0.0, 1.0};
  const Nig nig{2.0, 0.5, 0.0, 1.0};
  const Series x_gh_nig = pluss::make_series(std::vector<double>{-1.0, 0.0, 1.0}, {3}, f64);
  require_allclose("gh(lambda=-0.5)::pdf vs nig", gh_as_nig.pdf(x_gh_nig), nig.pdf(x_gh_nig), 1e-7);
  require_allclose("gh(lambda=-0.5)::cdf vs nig", gh_as_nig.cdf(x_gh_nig), nig.cdf(x_gh_nig), 1e-7);

  const Series mix_weights = pluss::make_series(std::vector<double>{0.7, 0.3}, {2}, f64);
  const Series mix_means = pluss::make_series(std::vector<double>{-1.0, 2.0}, {2}, f64);
  const Series mix_stds = pluss::make_series(std::vector<double>{0.5, 1.25}, {2}, f64);
  const GaussianMixture gm{mix_weights, mix_means, mix_stds};
  const Series x_mix = pluss::make_series(std::vector<double>{-2.0, 0.0, 3.0}, {3}, f64);
  require_allclose("gaussian_mixture::pdf", gm.pdf(x_mix), mixture_normal_pdf(x_mix, mix_weights, mix_means, mix_stds), 1e-12);
  require_allclose("gaussian_mixture::logpdf", gm.logpdf(x_mix), mixture_normal_logpdf(x_mix, mix_weights, mix_means, mix_stds), 1e-12);
  require_allclose("gaussian_mixture::cdf", gm.cdf(x_mix), mixture_normal_cdf(x_mix, mix_weights, mix_means, mix_stds), 1e-12);
  require_allclose("gaussian_mixture::logcdf", gm.logcdf(x_mix), mixture_normal_logcdf(x_mix, mix_weights, mix_means, mix_stds), 1e-12);
}

TEST_CASE("distribution objects preserve sample_like shape platform and requested visible dtype", "[distr][objects][sample_like]") {
  const Series like = pluss::series::zeros(
      {2, 3},
      Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU).with_pinned(true));
  const auto like_shape = as_shape_vec(like);

  const auto normal_like = Normal{0.0, 1.0}.sample_like(like, 11ULL);
  const auto bernoulli_like = Bernoulli{0.4}.sample_like(like, 13ULL);
  const auto gamma_like = Gamma{2.0, 1.5, 0.0}.sample_like(like, 17ULL);
  const auto chisq_like = ChiSquare{5}.sample_like(like, 18ULL);
  const auto ig_like = InverseGaussian{1.5, 3.0}.sample_like(like, 18ULL);
  const auto lognormal_like = Lognormal{0.1, 0.4}.sample_like(like, 18ULL);
  const auto laplace_like = Laplace{0.0, 0.5}.sample_like(like, 19ULL);
  const auto logistic_like = Logistic{0.0, 0.75}.sample_like(like, 23ULL);
  const auto rayleigh_like = Rayleigh{1.25}.sample_like(like, 29ULL);
  const auto poisson_like = Poisson{2.5}.sample_like(like, 31ULL);
  const auto geometric_like = Geometric{0.25}.sample_like(like, 33ULL);
  const auto negbin_like = NegativeBinomial{3.0, 0.4}.sample_like(like, 35ULL);
  const auto normaltrunc_like = NormalTrunc{-1.0, 1.0, 0.0, 1.0}.sample_like(like, 36ULL);
  const auto student_like = StudentT{9.0, 0.0, 1.0}.sample_like(like, 36ULL);
  const auto f_like = FDistribution{5.0, 12.0}.sample_like(like, 36ULL);
  const auto weibull_like = Weibull{2.0, 1.5}.sample_like(like, 36ULL);
  const auto beta_like = Beta{0.0, 1.0, 2.0, 3.0}.sample_like(like, 36ULL);
  const auto gh_like = Gh{1.0, 2.0, 0.5, 0.0, 1.0}.sample_like(like, 36ULL);

  REQUIRE(normal_like.scalar_type() == ScalarType::Float32);
  REQUIRE(bernoulli_like.scalar_type() == ScalarType::Float32);
  REQUIRE(gamma_like.scalar_type() == ScalarType::Float32);
  REQUIRE(chisq_like.scalar_type() == ScalarType::Float32);
  REQUIRE(ig_like.scalar_type() == ScalarType::Float32);
  REQUIRE(lognormal_like.scalar_type() == ScalarType::Float32);
  REQUIRE(laplace_like.scalar_type() == ScalarType::Float32);
  REQUIRE(logistic_like.scalar_type() == ScalarType::Float32);
  REQUIRE(rayleigh_like.scalar_type() == ScalarType::Float32);
  REQUIRE(poisson_like.scalar_type() == ScalarType::Float32);
  REQUIRE(geometric_like.scalar_type() == ScalarType::Float32);
  REQUIRE(negbin_like.scalar_type() == ScalarType::Float32);
  REQUIRE(normaltrunc_like.scalar_type() == ScalarType::Float32);
  REQUIRE(student_like.scalar_type() == ScalarType::Float32);
  REQUIRE(f_like.scalar_type() == ScalarType::Float32);
  REQUIRE(weibull_like.scalar_type() == ScalarType::Float32);
  REQUIRE(beta_like.scalar_type() == ScalarType::Float32);
  REQUIRE(gh_like.scalar_type() == ScalarType::Float32);
  REQUIRE(normal_like.platform() == pluss::pCPU);
  REQUIRE(bernoulli_like.platform() == pluss::pCPU);
  REQUIRE(gamma_like.platform() == pluss::pCPU);
  REQUIRE(chisq_like.platform() == pluss::pCPU);
  REQUIRE(ig_like.platform() == pluss::pCPU);
  REQUIRE(lognormal_like.platform() == pluss::pCPU);
  REQUIRE(laplace_like.platform() == pluss::pCPU);
  REQUIRE(logistic_like.platform() == pluss::pCPU);
  REQUIRE(rayleigh_like.platform() == pluss::pCPU);
  REQUIRE(poisson_like.platform() == pluss::pCPU);
  REQUIRE(geometric_like.platform() == pluss::pCPU);
  REQUIRE(negbin_like.platform() == pluss::pCPU);
  REQUIRE(normaltrunc_like.platform() == pluss::pCPU);
  REQUIRE(student_like.platform() == pluss::pCPU);
  REQUIRE(f_like.platform() == pluss::pCPU);
  REQUIRE(weibull_like.platform() == pluss::pCPU);
  REQUIRE(beta_like.platform() == pluss::pCPU);
  REQUIRE(gh_like.platform() == pluss::pCPU);
  REQUIRE(as_shape_vec(normal_like) == like_shape);
  REQUIRE(as_shape_vec(bernoulli_like) == like_shape);
  REQUIRE(as_shape_vec(gamma_like) == like_shape);
  REQUIRE(as_shape_vec(chisq_like) == like_shape);
  REQUIRE(as_shape_vec(ig_like) == like_shape);
  REQUIRE(as_shape_vec(lognormal_like) == like_shape);
  REQUIRE(as_shape_vec(laplace_like) == like_shape);
  REQUIRE(as_shape_vec(logistic_like) == like_shape);
  REQUIRE(as_shape_vec(rayleigh_like) == like_shape);
  REQUIRE(as_shape_vec(poisson_like) == like_shape);
  REQUIRE(as_shape_vec(geometric_like) == like_shape);
  REQUIRE(as_shape_vec(negbin_like) == like_shape);
  REQUIRE(as_shape_vec(normaltrunc_like) == like_shape);
  REQUIRE(as_shape_vec(student_like) == like_shape);
  REQUIRE(as_shape_vec(f_like) == like_shape);
  REQUIRE(as_shape_vec(weibull_like) == like_shape);
  REQUIRE(as_shape_vec(beta_like) == like_shape);
  REQUIRE(as_shape_vec(gh_like) == like_shape);

  REQUIRE(Bernoulli{0.4}.sample({4}, Config{}.with_platform(pluss::pCPU), 37ULL).scalar_type() == ScalarType::Bool);
  REQUIRE(Binomial{5, 0.4}.sample({4}, Config{}.with_platform(pluss::pCPU), 41ULL).scalar_type() == ScalarType::Int64);
  REQUIRE(Poisson{2.5}.sample({4}, Config{}.with_platform(pluss::pCPU), 43ULL).scalar_type() == ScalarType::Int64);
  REQUIRE(Skellam{1.5, 0.75}.sample({4}, Config{}.with_platform(pluss::pCPU), 47ULL).scalar_type() == ScalarType::Int64);
  REQUIRE(Nig{2.0, 0.5, 0.0, 1.0}.sample({4}, Config{}.with_platform(pluss::pCPU), 53ULL).scalar_type() == ScalarType::Float64);
  REQUIRE(Geometric{0.25}.sample({4}, Config{}.with_platform(pluss::pCPU), 57ULL).scalar_type() == ScalarType::Int64);
  REQUIRE(NegativeBinomial{3.0, 0.4}.sample({4}, Config{}.with_platform(pluss::pCPU), 61ULL).scalar_type() == ScalarType::Int64);
  REQUIRE(ChiSquare{5}.sample({4}, Config{}.with_platform(pluss::pCPU), 63ULL).scalar_type() == ScalarType::Float64);
  REQUIRE(InverseGaussian{1.5, 3.0}.sample({4}, Config{}.with_platform(pluss::pCPU), 67ULL).scalar_type() == ScalarType::Float64);
  REQUIRE(StudentT{9.0, 0.0, 1.0}.sample({4}, Config{}.with_platform(pluss::pCPU), 71ULL).scalar_type() == ScalarType::Float64);
  REQUIRE(FDistribution{5.0, 12.0}.sample({4}, Config{}.with_platform(pluss::pCPU), 73ULL).scalar_type() == ScalarType::Float64);

  const GaussianMixture gm{
      pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU)),
      pluss::make_series(std::vector<double>{0.0, 1.0}, {2}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU)),
      pluss::make_series(std::vector<double>{1.0, 0.5}, {2}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU)),
  };
  const auto mixture_like = gm.sample_like(like, 59ULL);
  REQUIRE(mixture_like.scalar_type() == ScalarType::Float32);
  REQUIRE(as_shape_vec(mixture_like) == like_shape);
}

TEST_CASE("cgf implementations stay stable near zero and near domain boundaries", "[distr][objects][cgf][stability]") {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series tiny_t = pluss::make_series(std::vector<double>{-1e-10, 0.0, 1e-10}, {3}, f64);
  const auto tiny_t_vals = as_f64_vec(tiny_t);

  const auto bernoulli_vals = as_f64_vec(Bernoulli{0.3}.cgf(tiny_t));
  const auto binomial_vals = as_f64_vec(Binomial{7, 0.3}.cgf(tiny_t));
  const auto uniform_vals = as_f64_vec(Uniform{2.0, 5.0}.cgf(tiny_t));

  for (std::size_t i = 0; i < bernoulli_vals.size(); ++i) {
    const double t = tiny_t_vals[i];
    REQUIRE(std::isfinite(bernoulli_vals[i]));
    REQUIRE(std::isfinite(binomial_vals[i]));
    REQUIRE(std::isfinite(uniform_vals[i]));
    REQUIRE(std::abs(bernoulli_vals[i] - std::log1p(0.3 * std::expm1(t))) <= 1e-15);
    REQUIRE(std::abs(binomial_vals[i] - 7.0 * std::log1p(0.3 * std::expm1(t))) <= 1e-15);
    REQUIRE(std::abs(uniform_vals[i] - uniform_cgf_expected(2.0, 5.0, t)) <= 1e-15);
  }

  const double gamma_edge_t = 1.0 / 1.5 - 1e-12;
  const double exponential_edge_t = 2.0 - 1e-12;
  const double gamma_edge = as_f64_scalar(Gamma{2.0, 1.5, 0.25}.cgf(pluss::make_series(gamma_edge_t, f64)));
  const double exponential_edge = as_f64_scalar(Exponential{2.0}.cgf(pluss::make_series(exponential_edge_t, f64)));
  const double chisq_edge = as_f64_scalar(ChiSquare{5}.cgf(pluss::make_series(0.5 - 1e-12, f64)));
  const double ig_edge = as_f64_scalar(InverseGaussian{1.5, 3.0}.cgf(pluss::make_series(3.0 / (2.0 * 1.5 * 1.5) - 1e-12, f64)));
  const double lognormal_neg = as_f64_scalar(Lognormal{0.1, 0.4}.cgf(pluss::make_series(-0.25, f64)));
  REQUIRE(std::isfinite(gamma_edge));
  REQUIRE(std::isfinite(exponential_edge));
  REQUIRE(std::isfinite(chisq_edge));
  REQUIRE(std::isfinite(ig_edge));
  REQUIRE(std::isfinite(lognormal_neg));
  REQUIRE(std::abs(gamma_edge - (0.25 * gamma_edge_t - 2.0 * std::log1p(-1.5 * gamma_edge_t))) <= 1e-10);
  REQUIRE(std::abs(exponential_edge - (-std::log1p(-exponential_edge_t / 2.0))) <= 1e-10);
  REQUIRE(std::abs(chisq_edge - (-2.5 * std::log1p(-2.0 * (0.5 - 1e-12)))) <= 1e-10);
}

TEST_CASE("cumulant returns exact low-order cumulants for starter object families", "[distr][objects][cumulant]") {
  require_cumulant_close(Normal{1.5, 2.0}, 1, 1.5);
  require_cumulant_close(Normal{1.5, 2.0}, 2, 4.0);
  require_cumulant_close(Normal{1.5, 2.0}, 3, 0.0);
  require_cumulant_close(Normal{1.5, 2.0}, 4, 0.0);

  require_cumulant_close(Bernoulli{0.25}, 1, 0.25);
  require_cumulant_close(Bernoulli{0.25}, 2, 0.25 * 0.75);
  require_cumulant_close(Bernoulli{0.25}, 3, 0.25 * 0.75 * (1.0 - 2.0 * 0.25));
  require_cumulant_close(Bernoulli{0.25}, 4, 0.25 * 0.75 * (1.0 - 6.0 * 0.25 * 0.75), 1e-11);

  require_cumulant_close(Binomial{4, 0.25}, 1, 1.0);
  require_cumulant_close(Binomial{4, 0.25}, 2, 4.0 * 0.25 * 0.75);
  require_cumulant_close(Binomial{4, 0.25}, 3, 4.0 * 0.25 * 0.75 * (1.0 - 2.0 * 0.25));
  require_cumulant_close(Binomial{4, 0.25}, 4, 4.0 * 0.25 * 0.75 * (1.0 - 6.0 * 0.25 * 0.75), 1e-11);

  require_cumulant_close(Gamma{2.0, 1.5, 0.25}, 1, 0.25 + 2.0 * 1.5);
  require_cumulant_close(Gamma{2.0, 1.5, 0.25}, 2, 2.0 * 1.5 * 1.5);
  require_cumulant_close(Gamma{2.0, 1.5, 0.25}, 3, 2.0 * 2.0 * 1.5 * 1.5 * 1.5);
  require_cumulant_close(Gamma{2.0, 1.5, 0.25}, 4, 2.0 * 6.0 * std::pow(1.5, 4.0));

  require_cumulant_close(ChiSquare{5}, 1, 5.0);
  require_cumulant_close(ChiSquare{5}, 2, 10.0);
  require_cumulant_close(ChiSquare{5}, 4, 240.0);

  require_cumulant_close(InverseGaussian{1.5, 3.0}, 1, 1.5);
  require_cumulant_close(InverseGaussian{1.5, 3.0}, 2, 1.125);
  require_cumulant_close(InverseGaussian{1.5, 3.0}, 5, inverse_gaussian_cumulant_expected(1.5, 3.0, 5), 1e-11);

  require_cumulant_close(Lognormal{0.1, 0.4}, 1, std::exp(0.1 + 0.5 * 0.16), 1e-12);
  require_cumulant_close(Lognormal{0.1, 0.4}, 2, (std::exp(0.16) - 1.0) * std::exp(2.0 * 0.1 + 0.16), 1e-12);

  require_cumulant_close(Uniform{0.0, 1.0}, 1, 0.5);
  require_cumulant_close(Uniform{0.0, 1.0}, 2, 1.0 / 12.0);
  require_cumulant_close(Uniform{0.0, 1.0}, 3, 0.0, 1e-12);
  require_cumulant_close(Uniform{0.0, 1.0}, 4, -1.0 / 120.0, 1e-12);

  require_cumulant_close(Exponential{2.0}, 1, 0.5);
  require_cumulant_close(Exponential{2.0}, 2, 0.25);
  require_cumulant_close(Exponential{2.0}, 3, 0.25);
  require_cumulant_close(Exponential{2.0}, 4, 0.375);

  require_cumulant_close(Laplace{0.5, 0.5}, 1, 0.5);
  require_cumulant_close(Laplace{0.5, 0.5}, 2, 0.5);
  require_cumulant_close(Laplace{0.5, 0.5}, 3, 0.0);
  require_cumulant_close(Laplace{0.5, 0.5}, 6, 3.75);

  require_cumulant_close(Logistic{0.25, 0.5}, 1, 0.25);
  require_cumulant_close(Logistic{0.25, 0.5}, 2, std::numbers::pi_v<double> * std::numbers::pi_v<double> / 12.0);
  require_cumulant_close(Logistic{0.25, 0.5}, 3, 0.0);
  require_cumulant_close(Logistic{0.25, 0.5}, 6, std::pow(std::numbers::pi_v<double>, 6.0) / 252.0, 1e-10);

  require_cumulant_close(Rayleigh{1.25}, 1, 1.25 * std::sqrt(std::numbers::pi_v<double> / 2.0));
  require_cumulant_close(Rayleigh{1.25}, 2, ((4.0 - std::numbers::pi_v<double>) / 2.0) * 1.25 * 1.25, 1e-11);
  require_cumulant_close(Rayleigh{1.25}, 5, rayleigh_cumulant_expected(1.25, 5), 1e-9);
  require_cumulant_close(NormalTrunc{-1.0, 1.0, 0.0, 1.0}, 1, 0.0, 1e-12);
  require_cumulant_close(NormalTrunc{-1.0, 1.0, 0.0, 1.0}, 2, normaltrunc_cumulant_expected(-1.0, 1.0, 0.0, 1.0, 2), 1e-11);
  require_cumulant_close(NormalTrunc{-1.0, 1.0, 0.0, 1.0}, 5, 0.0, 1e-10);

  require_cumulant_close(StudentT{9.0, 0.0, 1.0}, 1, 0.0);
  require_cumulant_close(StudentT{9.0, 0.0, 1.0}, 2, 9.0 / 7.0, 1e-12);
  require_cumulant_close(StudentT{9.0, 0.0, 1.0}, 4, 243.0 / 35.0 - 3.0 * std::pow(9.0 / 7.0, 2.0), 1e-10);

  require_cumulant_close(FDistribution{5.0, 12.0}, 1, 12.0 / 10.0, 1e-12);
  require_cumulant_close(FDistribution{5.0, 12.0}, 2, 1.08, 1e-12);

  require_cumulant_close(Weibull{2.0, 1.5}, 1, 1.5 * std::tgamma(1.5), 1e-12);
  require_cumulant_close(Weibull{2.0, 1.5}, 2, 2.25 * (1.0 - std::numbers::pi_v<double> / 4.0), 1e-12);

  require_cumulant_close(Beta{0.0, 1.0, 2.0, 3.0}, 1, 0.4, 1e-12);
  require_cumulant_close(Beta{0.0, 1.0, 2.0, 3.0}, 2, 0.04, 1e-12);

  require_cumulant_close(Skellam{2.0, 1.0}, 1, 1.0);
  require_cumulant_close(Skellam{2.0, 1.0}, 2, 3.0);
  require_cumulant_close(Skellam{2.0, 1.0}, 3, 1.0);
  require_cumulant_close(Skellam{2.0, 1.0}, 4, 3.0);

  require_cumulant_close(Poisson{2.5}, 1, 2.5);
  require_cumulant_close(Poisson{2.5}, 2, 2.5);
  require_cumulant_close(Poisson{2.5}, 7, 2.5);
  require_cumulant_close(Geometric{0.25}, 1, 4.0);
  require_cumulant_close(Geometric{0.25}, 2, 12.0);
  require_cumulant_close(Geometric{0.25}, 5, negbinom_cumulant_expected(1.0, 0.25, 5), 1e-8);
  require_cumulant_close(NegativeBinomial{3.0, 0.4}, 1, 3.0 * 0.6 / 0.4);
  require_cumulant_close(NegativeBinomial{3.0, 0.4}, 2, 3.0 * 0.6 / (0.4 * 0.4));
  require_cumulant_close(NegativeBinomial{3.0, 0.4}, 5, negbinom_cumulant_expected(3.0, 0.4, 5), 1e-8);

  const double alpha = 2.0;
  const double beta = 0.5;
  const double scale = 1.0;
  const double gamma = std::sqrt(alpha * alpha - beta * beta);
  require_cumulant_close(Nig{alpha, beta, 0.0, scale}, 1, scale * beta / gamma);
  require_cumulant_close(Nig{alpha, beta, 0.0, scale}, 2, scale * alpha * alpha / std::pow(gamma, 3.0));
  require_cumulant_close(Nig{alpha, beta, 0.0, scale}, 3, 3.0 * scale * beta * alpha * alpha / std::pow(gamma, 5.0), 1e-11);
  require_cumulant_close(Nig{alpha, beta, 0.0, scale}, 4, 3.0 * scale * alpha * alpha * (alpha * alpha + 4.0 * beta * beta) / std::pow(gamma, 7.0), 1e-10);
  require_cumulant_close(Nig{alpha, beta, 0.0, scale}, 5, nig_cumulant_expected(alpha, beta, 0.0, scale, 5), 1e-10);
  require_cumulant_close(Nig{alpha, beta, 0.0, scale}, 6, nig_cumulant_expected(alpha, beta, 0.0, scale, 6), 1e-9);
  require_cumulant_close(Gh{-0.5, alpha, beta, 0.0, scale}, 2, scale * alpha * alpha / std::pow(gamma, 3.0), 1e-8);

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const GaussianMixture gm{
      pluss::make_series(std::vector<double>{1.0, 0.0}, {2}, f64),
      pluss::make_series(std::vector<double>{0.0, 5.0}, {2}, f64),
      pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, f64),
  };
  require_cumulant_close(gm, 1, 0.0);
  require_cumulant_close(gm, 2, 1.0);
  require_cumulant_close(gm, 3, 0.0, 1e-10);
  require_cumulant_close(gm, 4, 0.0, 1e-10);
}

TEST_CASE("cgf rejects out-of-domain t values for gamma and exponential", "[distr][objects][cgf][domain]") {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series gamma_bad = pluss::make_series(std::vector<double>{0.1, 0.5}, {2}, f64);
  const Series exponential_bad = pluss::make_series(std::vector<double>{0.0, 2.0}, {2}, f64);
  const Gamma gamma{2.0, 2.0, 0.0};
  const Exponential exponential{2.0};
  const Laplace laplace{0.0, 1.0};
  const Logistic logistic{0.0, 1.0};
  const Lognormal lognormal{0.1, 0.4};
  const StudentT student_t{9.0, 0.0, 1.0};
  const FDistribution f_dist{5.0, 12.0};
  const Geometric geometric{0.0};
  const NegativeBinomial negbin{3.0, 1.0};
  const Nig nig{1.0, 0.5, 0.0, 1.0};
  const Nig invalid_nig{1.0, 1.0, 0.0, 1.0};
  const NormalTrunc invalid_trunc{1.0, -1.0, 0.0, 1.0};
  const Gh invalid_gh{1.0, 1.0, 1.0, 0.0, 1.0};

  REQUIRE_THROWS_AS(gamma.cgf(gamma_bad), std::domain_error);
  REQUIRE_THROWS_AS(exponential.cgf(exponential_bad), std::domain_error);
  REQUIRE_THROWS_AS(laplace.cgf(pluss::make_series(1.0, f64)), std::domain_error);
  REQUIRE_THROWS_AS(logistic.cgf(pluss::make_series(1.0, f64)), std::domain_error);
  REQUIRE_THROWS_AS(lognormal.cgf(pluss::make_series(0.1, f64)), std::domain_error);
  REQUIRE_THROWS_AS(student_t.cgf(pluss::make_series(0.1, f64)), std::domain_error);
  REQUIRE_THROWS_AS(f_dist.cgf(pluss::make_series(0.1, f64)), std::domain_error);
  REQUIRE_THROWS_AS((StudentT{4.0, 0.0, 1.0}.cumulant(4)), std::domain_error);
  REQUIRE_THROWS_AS((FDistribution{5.0, 6.0}.cumulant(3)), std::domain_error);
  REQUIRE_THROWS_AS(geometric.cumulant(1), std::domain_error);
  REQUIRE_THROWS_AS(negbin.cumulant(1), std::domain_error);
  REQUIRE_THROWS_AS(nig.cgf(pluss::make_series(0.6, f64)), std::domain_error);
  REQUIRE_THROWS_AS(Normal{}.cumulant(0), std::invalid_argument);
  REQUIRE_THROWS_AS(invalid_nig.cumulant(2), std::domain_error);
  REQUIRE_THROWS_AS(invalid_trunc.cumulant(2), std::domain_error);
  REQUIRE_THROWS_AS(invalid_gh.cumulant(2), std::domain_error);
}

TEST_CASE("functional and object normal logcdf stay finite in the far left tail", "[distr][objects][logcdf][tail]") {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series x = pluss::make_series(-12.0, f64);

  const double functional = as_f64_scalar(logcdf(dst::Normal, x, std::initializer_list<double>{}));
  const double object = as_f64_scalar(Normal{}.logcdf(x));
  const double shortcut = as_f64_scalar(x.stat.logcdf());
  const double from_cdf = as_f64_scalar(cdf(dst::Normal, x, std::initializer_list<double>{}).log());

  REQUIRE(std::isfinite(functional));
  REQUIRE(std::isfinite(object));
  REQUIRE(std::isfinite(shortcut));
  REQUIRE(std::isfinite(from_cdf));
  REQUIRE(functional < -70.0);
  REQUIRE(std::abs(functional - object) <= 1e-12);
  REQUIRE(std::abs(functional - shortcut) <= 1e-12);
  REQUIRE(std::abs(functional - from_cdf) <= 1e-9);
}

TEST_CASE("additional functional logcdf routes and gaussian mixture logcdf retain tail parity", "[distr][objects][logcdf][mixture]") {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  REQUIRE(std::isfinite(as_f64_scalar(logcdf(dst::Logistic, pluss::make_series(-50.0, f64), {0.0, 1.0}))));
  REQUIRE(std::isfinite(as_f64_scalar(logcdf(dst::Laplace, pluss::make_series(-50.0, f64), {0.0, 1.0}))));
  REQUIRE(std::isfinite(as_f64_scalar(logcdf(dst::Rayleigh, pluss::make_series(10.0, f64), {1.0}))));
  REQUIRE(std::isfinite(as_f64_scalar(logcdf(dst::NormalTrunc, pluss::make_series(-0.999, f64), {-1.0, 1.0, 0.0, 1.0}))));

  const Series weights = pluss::make_series(std::vector<double>{1.0}, {1}, f64);
  const Series means = pluss::make_series(std::vector<double>{0.0}, {1}, f64);
  const Series stddevs = pluss::make_series(std::vector<double>{1.0}, {1}, f64);
  const Series x = pluss::make_series(-12.0, f64);
  const GaussianMixture gm{weights, means, stddevs};

  const double stable = as_f64_scalar(gm.logcdf(x));
  const double from_cdf = as_f64_scalar(gm.cdf(x).log());
  REQUIRE(std::isfinite(stable));
  REQUIRE(std::isfinite(from_cdf));
  REQUIRE(std::abs(stable - from_cdf) <= 1e-9);
  REQUIRE(std::abs(stable - as_f64_scalar(logcdf(dst::Normal, x, std::initializer_list<double>{}))) <= 1e-5);
}

TEST_CASE("gaussian mixture validates component metadata before delegation", "[distr][objects][mixture][validation]") {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series x = pluss::make_series(0.0, f64);

  const GaussianMixture mismatch{
      pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, f64),
      pluss::make_series(std::vector<double>{0.0}, {1}, f64),
      pluss::make_series(std::vector<double>{1.0, 1.0}, {2}, f64),
  };
  REQUIRE_THROWS(mismatch.pdf(x));
  REQUIRE_THROWS(mismatch.sample({4}, f64, 41ULL));

  const GaussianMixture zero_weight_sum{
      pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, f64),
      pluss::make_series(std::vector<double>{0.0, 1.0}, {2}, f64),
      pluss::make_series(std::vector<double>{1.0, 1.0}, {2}, f64),
  };
  REQUIRE_THROWS(zero_weight_sum.pdf(x));
  REQUIRE_THROWS(zero_weight_sum.cgf(pluss::make_series(0.0, f64)));
}

// Reference values from independent maximum-likelihood fits (scipy-style optimisation of the exact log-likelihood; EM run to convergence).
TEST_CASE("maximum likelihood fits reach the exact optimum", "[distr][mle][reference]") {
  using namespace pluss::subsp::stats::distr;
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto series = [&](std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}, f64); };

  const auto g = series({0.8, 1.1, 1.9, 2.7, 3.6, 4.9, 2.2, 1.4, 3.1, 0.6});
  const auto gfit = Gamma{1.0, 1.0, 0.0}.MLE(g);
  REQUIRE(gfit.shape == Catch::Approx(2.791118015).epsilon(1e-8));
  REQUIRE(gfit.scale == Catch::Approx(0.798962992).epsilon(1e-8));
  REQUIRE(gfit.loglik(g).item<double>() == Catch::Approx(-15.772395065).epsilon(1e-9));
  const auto gse = gfit.std_err(g).host<double>();
  REQUIRE(gse[2] == 0.0);  // the location is held fixed
  const auto gv = gfit.vcov(g).host<double>();
  REQUIRE(gv[1] != 0.0);   // the shape and the scale are correlated: the full covariance, not only its diagonal

  const auto b = series({0.10, 0.22, 0.35, 0.41, 0.57, 0.68, 0.81, 0.30, 0.62, 0.45});
  const auto bfit = Beta{0.0, 1.0, 1.0, 1.0}.MLE(b);
  REQUIRE(bfit.alpha == Catch::Approx(2.189731620).epsilon(1e-8));
  REQUIRE(bfit.beta == Catch::Approx(2.685464131).epsilon(1e-8));

  const auto c = series({0, 0, 1, 2, 4, 7, 3, 6, 0, 1, 5, 2});
  const auto nfit = NegativeBinomial{1.0, 0.5}.MLE(c);
  REQUIRE(nfit.r == Catch::Approx(1.620101531).epsilon(1e-7));
  REQUIRE(nfit.p == Catch::Approx(0.385423252).epsilon(1e-7));

  // Laplace: the location has the standard error scale / sqrt(n), not a placeholder
  const auto lfit = Laplace{0.0, 1.0}.MLE(g);
  REQUIRE(lfit.std_err(g).host<double>()[0] == Catch::Approx(lfit.scale / std::sqrt(10.0)).epsilon(1e-12));

  // Gaussian mixture: EM converges to the same optimum from different starts
  std::vector<double> mix;
  for (int i = 0; i < 60; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    mix.push_back((i % 3 == 0 ? -2.0 : 1.5) + (u - 0.5) * 2.0);
  }
  const auto xs = series(mix);
  const auto w0 = series({0.5, 0.5}), s0 = series({1.0, 1.0});
  const auto fit_a = GaussianMixture{w0, series({-1.0, 1.0}), s0}.MLE(xs);
  const auto fit_b = GaussianMixture{w0, series({-3.0, 3.0}), s0}.MLE(xs);
  REQUIRE(fit_a.loglik(xs).item<double>() == Catch::Approx(-85.76929012).epsilon(1e-9));
  REQUIRE(fit_b.loglik(xs).item<double>() == Catch::Approx(-85.76929012).epsilon(1e-9));
  REQUIRE(fit_a.weights.host<double>()[0] == Catch::Approx(0.33334178).epsilon(1e-6));
}

TEST_CASE("generalised hyperbolic and NIG densities are accurate", "[distr][bessel][reference]") {
  using namespace pluss::subsp::stats::distr;
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto x = pluss::make_series(std::vector<double>{-2.5, -1.0, 0.4, 3.0}, {4}, f64);
  // exact values (arbitrary precision) of the NIG(alpha 2, beta 0.5, mu 0.1, delta 1.2) density
  const auto p = Nig{2.0, 0.5, 0.1, 1.2}.pdf(x).host<double>();
  const std::vector<double> expected{0.00134548612717506, 0.08192507289386157, 0.5594923065755876, 0.01053281724362568};
  for (std::size_t i = 0; i < 4; ++i) REQUIRE(p[i] == Catch::Approx(expected[i]).epsilon(1e-9));
  // the Gh with lambda = -1/2 is the NIG: they agree to rounding
  const auto q = Gh{-0.5, 2.0, 0.5, 0.1, 1.2}.pdf(x).host<double>();
  for (std::size_t i = 0; i < 4; ++i) REQUIRE(q[i] == Catch::Approx(p[i]).epsilon(1e-10));
}

// Every family: the density (mass) integrated (summed) over a range equals the difference of the distribution function at its ends,
// and the fraction of draws that fall in the range matches it. This pins pdf, cdf and the sampler against each other.
TEST_CASE("distr: pdf, cdf and the sampler agree for every kind", "[distr][kinds][consistency]") {
    const auto f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  struct Case { dst::Kind kind; std::vector<double> params; double a, b; bool discrete; };
  const std::vector<Case> cases = {
      {dst::Uniform, {1.0, 3.0}, 1.5, 2.5, false},
      {dst::Beta, {2.5, 4.0}, 0.1, 0.7, false},
      {dst::Bradford, {2.0}, 0.1, 0.9, false},
      {dst::Triangular, {0.0, 1.0, 4.0}, 0.5, 3.0, false},
      {dst::NormalTrunc, {-1.0, 2.0, 0.0, 1.0}, -0.5, 1.5, false},
      {dst::Arcsin, {}, 0.1, 0.9, false},
      {dst::Anglit, {}, -0.6, 0.6, false},
      {dst::Cosine, {}, -2.5, 2.5, false},
      {dst::HalfCosine, {}, -2.5, 2.5, false},
      {dst::Quadratic, {}, -0.8, 0.8, false},
      {dst::UShape, {}, -0.8, 0.8, false},
      {dst::Semicircle, {0.0, 1.0}, -0.8, 0.8, false},
      {dst::VonMises, {0.5, 2.0}, -1.5, 2.5, false},
      {dst::Bernoulli, {0.3}, 0.0, 1.0, true},
      {dst::Binomial, {12.0, 0.35}, 2.0, 6.0, true},
      {dst::Geometric, {0.3}, 1.0, 6.0, true},
      {dst::NegativeBinomial, {3.5, 0.4}, 1.0, 10.0, true},
      {dst::Hypergeometric, {50.0, 20.0, 10.0}, 1.0, 8.0, true},
      {dst::Skellam, {2.0, 1.5}, -3.0, 6.0, true},
      {dst::Bessel, {1.0, 4.0}, 0.0, 8.0, true},
      {dst::Yule, {3.0}, 1.0, 10.0, true},
      {dst::Zeta, {3.5}, 1.0, 10.0, true},
      {dst::Rademacher, {}, -1.0, 1.0, true},
      {dst::Normal, {1.5, 2.0}, -0.5, 4.0, false},
      {dst::Weibull, {1.7, 2.1}, 0.5, 2.0, false},
      {dst::Poisson, {3.2}, 0.0, 6.0, true},
      {dst::Gamma, {0.0, 1.3, 2.5}, 1.0, 6.0, false},
      {dst::Lognormal, {0.0, 0.5}, 0.5, 3.0, false},
      {dst::Exponential, {0.7}, 0.5, 3.0, false},
      {dst::ChiSquare, {5.0}, 1.0, 9.0, false},
      {dst::ChiSquareNoncentral, {2.0, 4.0}, 1.0, 12.0, false},
      {dst::Chi, {3.0}, 0.5, 3.0, false},
      {dst::F, {5.0, 10.0}, 0.5, 4.0, false},
      {dst::FNoncentral, {2.0, 5.0, 10.0}, 0.5, 5.0, false},
      {dst::Rayleigh, {1.5}, 0.5, 4.0, false},
      {dst::Maxwell, {1.2}, 0.5, 4.0, false},
      {dst::Rice, {2.0, 1.0}, 0.5, 4.5, false},
      {dst::Nakagami, {2.0, 1.5}, 0.3, 2.5, false},
      {dst::InverseGaussian, {1.0, 2.0}, 0.3, 2.5, false},
      {dst::Wald, {2.0}, 0.3, 2.5, false},
      {dst::Gig, {1.0, 2.0, 3.0}, 0.2, 3.0, false},
      {dst::Pareto, {1.0, 3.0}, 1.0, 6.0, false},
      {dst::Lomax, {1.5, 3.0}, 0.5, 6.0, false},
      {dst::Burr, {2.0, 3.0}, 0.4, 3.0, false},
      {dst::Gompertz, {0.8, 2.0}, 0.2, 2.0, false},
      {dst::Erlang, {1.5, 3.0}, 1.0, 8.0, false},
      {dst::Alpha, {2.0, 1.0}, 0.2, 6.0, false},
      {dst::Planck, {2.0, 1.0}, 0.3, 6.0, false},
      {dst::Cauchy, {0.5, 2.0}, -5.0, 6.0, false},
      {dst::Laplace, {1.0, 1.5}, -1.0, 4.0, false},
      {dst::Logistic, {0.5, 1.2}, -2.0, 4.0, false},
      {dst::StudentT, {5.0, 0.0, 1.0}, -3.0, 3.0, false},
      {dst::ExtremeValue, {0.0, 1.0}, -1.0, 5.0, false},
      {dst::HyperbolicSecant, {0.0, 1.0}, -3.0, 4.0, false},
      {dst::TukeyLambda, {0.14}, -3.0, 3.0, false},
      {dst::Nig, {1.5, -0.4, 0.1, 0.8}, -3.0, 3.0, false},
      {dst::Gh, {1.0, 1.5, -0.4, 0.1, 0.8}, -3.0, 3.0, false},
  };
  for (const auto& c : cases) {
    INFO("kind index " << static_cast<int>(c.kind));
    double integral = 0.0, between = 0.0;
    if (c.discrete) {
      const auto n = static_cast<std::int64_t>(c.b - c.a + 1.0);
      const auto grid = pluss::series::linspace(c.a, c.b, n, f64).round();
      integral = pdf(c.kind, grid, c.params).sum().item<double>();
      const auto edges = pluss::make_series({c.a - 1.0, c.b}, f64);
      const auto cd = cdf(c.kind, edges, c.params).host<double>();
      between = cd[1] - cd[0];
    } else {
      const auto grid = pluss::series::linspace(c.a, c.b, 4001, f64);
      integral = pdf(c.kind, grid, c.params).antidiff(grid).trapezoid().item<double>();
      const auto edges = pluss::make_series({c.a, c.b}, f64);
      const auto cd = cdf(c.kind, edges, c.params).host<double>();
      between = cd[1] - cd[0];
    }
    REQUIRE(integral == Catch::Approx(between).margin(2e-4));
    const auto draws = sample(c.kind, c.params, {200000}, f64, 7ULL);
    const double fraction = ((draws >= c.a).to(ScalarType::Float64) * (draws <= c.b).to(ScalarType::Float64)).mean().item<double>();
    const double se = std::sqrt(std::max(between * (1.0 - between), 1e-9) / 200000.0);
    REQUIRE(std::abs(fraction - between) < 5.0 * se + 1e-4);
  }
}

TEST_CASE("distr: Yule cdf has the closed form 1 - 2/((k+1)(k+2)) for p = 3", "[distr][yule]") {
    const auto f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto k = pluss::make_series({1.0, 2.0, 3.0, 10.0}, f64);
  const auto c = cdf(dst::Yule, k, {3.0}).host<double>();
  const double ks[] = {1.0, 2.0, 3.0, 10.0};
  for (int i = 0; i < 4; ++i) REQUIRE(c[i] == Catch::Approx(1.0 - 2.0 / ((ks[i] + 1.0) * (ks[i] + 2.0))).margin(1e-12));
}

TEST_CASE("distr: the anglit density is cos(2x) on its support and integrates to one", "[distr][anglit]") {
    const auto f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto x = pluss::make_series({-0.7, -0.2, 0.0, 0.5}, f64);
  const auto d = pdf(dst::Anglit, x, {}).host<double>();
  const double xs[] = {-0.7, -0.2, 0.0, 0.5};
  for (int i = 0; i < 4; ++i) REQUIRE(d[i] == Catch::Approx(std::cos(2.0 * xs[i])).margin(1e-12));
}

TEST_CASE("distr: generalised hyperbolic sampler matches the exact mean and variance", "[distr][gh][sample]") {
    const auto f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  for (double lambda : {-1.0, 1.0, 3.0}) {
    const Gh d{lambda, 1.5, -0.4, 0.1, 0.8};
    const auto s = d.sample({400000}, f64, 5ULL);
    const double m = s.mean().item<double>();
    const double v = ((s - m) * (s - m)).mean().item<double>();
    REQUIRE(m == Catch::Approx(d.cumulant(1).item<double>()).margin(0.02));
    REQUIRE(v == Catch::Approx(d.cumulant(2).item<double>()).epsilon(0.03));
  }
}
