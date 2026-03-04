#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
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
using namespace pluss::subsp::stat::distr;

constexpr double kTol = 1e-12;

double as_f64_scalar(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

std::vector<double> as_f64_vec(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
}

std::vector<std::int64_t> as_shape_vec(const Series& x) {
  return std::vector<std::int64_t>(x.sizes().begin(), x.sizes().end());
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
                               std::string_view name,
                               const Series& x,
                               const ParamArray& params,
                               double tol = kTol) {
  require_allclose(std::string(name) + "::pdf", dist.pdf(x), pdf(name, x, params), tol);
  require_allclose(std::string(name) + "::logpdf", dist.logpdf(x), logpdf(name, x, params), tol);
  require_allclose(std::string(name) + "::cdf", dist.cdf(x), cdf(name, x, params), tol);
  require_allclose(std::string(name) + "::logcdf", dist.logcdf(x), logcdf(name, x, params), tol);
}

double uniform_cgf_expected(double low, double high, double t) {
  if (t == 0.0) {
    return 0.0;
  }
  return low * t + std::log(std::expm1((high - low) * t) / ((high - low) * t));
}

} // namespace

static_assert(std::is_same_v<decltype(sample("normal",
                                             std::initializer_list<double>{0.0, 1.0},
                                             std::initializer_list<std::int64_t>{2},
                                             Config{},
                                             0ULL)),
                             Series>);
static_assert(std::is_same_v<decltype(sample_like("normal",
                                                  std::initializer_list<double>{0.0, 1.0},
                                                  std::declval<const Series&>(),
                                                  0ULL)),
                             Series>);
static_assert(std::is_same_v<decltype(pdf("normal", std::declval<const Series&>(), std::initializer_list<double>{0.0, 1.0})),
                             Series>);
static_assert(std::is_same_v<decltype(logpdf("normal", std::declval<const Series&>(), std::initializer_list<double>{0.0, 1.0})),
                             Series>);
static_assert(std::is_same_v<decltype(cdf("normal", std::declval<const Series&>(), std::initializer_list<double>{0.0, 1.0})),
                             Series>);
static_assert(std::is_same_v<decltype(logcdf("normal", std::declval<const Series&>(), std::initializer_list<double>{0.0, 1.0})),
                             Series>);
static_assert(std::is_same_v<decltype(mixture_normal_logcdf(std::declval<const Series&>(),
                                                            std::declval<const Series&>(),
                                                            std::declval<const Series&>(),
                                                            std::declval<const Series&>())),
                             Series>);

TEST_CASE("distribution objects stay aligned with functional pdf/logpdf/cdf routes", "[distr][objects][parity]") {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  require_functional_parity(Normal{0.5, 1.25},
                            "normal",
                            pluss::make_series(std::vector<double>{-1.0, 0.0, 1.0}, {3}, f64),
                            std::initializer_list<double>{0.5, 1.25});
  require_functional_parity(Bernoulli{0.25},
                            "bernoulli",
                            pluss::make_series(std::vector<double>{0.0, 1.0}, {2}, f64),
                            std::initializer_list<double>{0.25});
  require_functional_parity(Binomial{4, 0.25},
                            "binomial",
                            pluss::make_series(std::vector<double>{0.0, 1.0, 2.0}, {3}, f64),
                            std::initializer_list<double>{4.0, 0.25});
  require_functional_parity(Gamma{2.0, 1.5, 0.25},
                            "gamma",
                            pluss::make_series(std::vector<double>{0.5, 1.0, 2.0}, {3}, f64),
                            std::initializer_list<double>{0.25, 1.5, 2.0});
  require_functional_parity(Uniform{-2.0, 3.0},
                            "uniform",
                            pluss::make_series(std::vector<double>{-2.0, 0.0, 2.0}, {3}, f64),
                            std::initializer_list<double>{-2.0, 3.0});
  require_functional_parity(Exponential{2.0},
                            "exponential",
                            pluss::make_series(std::vector<double>{0.0, 0.5, 1.0}, {3}, f64),
                            std::initializer_list<double>{2.0});
  require_functional_parity(Skellam{4.0, 1.5},
                            "skellam",
                            pluss::make_series(std::vector<double>{-1.0, 0.0, 2.0}, {3}, f64),
                            std::initializer_list<double>{4.0, 1.5},
                            1e-9);
  require_functional_parity(Nig{2.0, 0.5, 0.0, 1.0},
                            "nig",
                            pluss::make_series(std::vector<double>{-1.0, 0.0, 1.0}, {3}, f64),
                            std::initializer_list<double>{2.0, 0.5, 0.0, 1.0},
                            1e-8);

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
  const Series like = pluss::zeros(
      {2, 3},
      Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU).with_pinned(true));
  const auto like_shape = as_shape_vec(like);

  const auto normal_like = Normal{0.0, 1.0}.sample_like(like, 11ULL);
  const auto bernoulli_like = Bernoulli{0.4}.sample_like(like, 13ULL);
  const auto gamma_like = Gamma{2.0, 1.5, 0.0}.sample_like(like, 17ULL);

  REQUIRE(normal_like.scalar_type() == ScalarType::Float32);
  REQUIRE(bernoulli_like.scalar_type() == ScalarType::Float32);
  REQUIRE(gamma_like.scalar_type() == ScalarType::Float32);
  REQUIRE(normal_like.platform() == pluss::pCPU);
  REQUIRE(bernoulli_like.platform() == pluss::pCPU);
  REQUIRE(gamma_like.platform() == pluss::pCPU);
  REQUIRE(as_shape_vec(normal_like) == like_shape);
  REQUIRE(as_shape_vec(bernoulli_like) == like_shape);
  REQUIRE(as_shape_vec(gamma_like) == like_shape);

  REQUIRE(Bernoulli{0.4}.sample({4}, Config{}.with_platform(pluss::pCPU), 19ULL).scalar_type() == ScalarType::Bool);
  REQUIRE(Binomial{5, 0.4}.sample({4}, Config{}.with_platform(pluss::pCPU), 23ULL).scalar_type() == ScalarType::Int64);
  REQUIRE(Skellam{1.5, 0.75}.sample({4}, Config{}.with_platform(pluss::pCPU), 29ULL).scalar_type() == ScalarType::Int64);
  REQUIRE(Nig{2.0, 0.5, 0.0, 1.0}.sample({4}, Config{}.with_platform(pluss::pCPU), 31ULL).scalar_type() == ScalarType::Float64);

  const GaussianMixture gm{
      pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU)),
      pluss::make_series(std::vector<double>{0.0, 1.0}, {2}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU)),
      pluss::make_series(std::vector<double>{1.0, 0.5}, {2}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU)),
  };
  const auto mixture_like = gm.sample_like(like, 37ULL);
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
  REQUIRE(std::isfinite(gamma_edge));
  REQUIRE(std::isfinite(exponential_edge));
  REQUIRE(std::abs(gamma_edge - (0.25 * gamma_edge_t - 2.0 * std::log1p(-1.5 * gamma_edge_t))) <= 1e-10);
  REQUIRE(std::abs(exponential_edge - (-std::log1p(-exponential_edge_t / 2.0))) <= 1e-10);
}

TEST_CASE("cgf rejects out-of-domain t values for gamma and exponential", "[distr][objects][cgf][domain]") {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series gamma_bad = pluss::make_series(std::vector<double>{0.1, 0.5}, {2}, f64);
  const Series exponential_bad = pluss::make_series(std::vector<double>{0.0, 2.0}, {2}, f64);
  const Gamma gamma{2.0, 2.0, 0.0};
  const Exponential exponential{2.0};
  const Nig nig{1.0, 0.5, 0.0, 1.0};

  REQUIRE_THROWS_AS(gamma.cgf(gamma_bad), std::domain_error);
  REQUIRE_THROWS_AS(exponential.cgf(exponential_bad), std::domain_error);
  REQUIRE_THROWS_AS(nig.cgf(pluss::make_series(0.6, f64)), std::domain_error);
}

TEST_CASE("functional and object normal logcdf stay finite in the far left tail", "[distr][objects][logcdf][tail]") {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series x = pluss::make_series(-12.0, f64);

  const double functional = as_f64_scalar(logcdf("normal", x, std::initializer_list<double>{}));
  const double object = as_f64_scalar(Normal{}.logcdf(x));
  const double shortcut = as_f64_scalar(x.logcdf());
  const double naive = as_f64_scalar(cdf("normal", x, std::initializer_list<double>{}).log());

  REQUIRE(std::isfinite(functional));
  REQUIRE(std::isfinite(object));
  REQUIRE(std::isfinite(shortcut));
  REQUIRE(std::isinf(naive));
  REQUIRE(functional < -70.0);
  REQUIRE(std::abs(functional - object) <= 1e-12);
  REQUIRE(std::abs(functional - shortcut) <= 1e-12);
}

TEST_CASE("additional functional logcdf routes and gaussian mixture logcdf avoid naive underflow", "[distr][objects][logcdf][mixture]") {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  REQUIRE(std::isfinite(as_f64_scalar(logcdf("logistic", pluss::make_series(-50.0, f64), {0.0, 1.0}))));
  REQUIRE(std::isfinite(as_f64_scalar(logcdf("laplace", pluss::make_series(-50.0, f64), {0.0, 1.0}))));
  REQUIRE(std::isfinite(as_f64_scalar(logcdf("rayleigh", pluss::make_series(10.0, f64), {1.0}))));
  REQUIRE(std::isfinite(as_f64_scalar(logcdf("normaltrunc", pluss::make_series(-0.999, f64), {-1.0, 1.0, 0.0, 1.0}))));

  const Series weights = pluss::make_series(std::vector<double>{1.0}, {1}, f64);
  const Series means = pluss::make_series(std::vector<double>{0.0}, {1}, f64);
  const Series stddevs = pluss::make_series(std::vector<double>{1.0}, {1}, f64);
  const Series x = pluss::make_series(-12.0, f64);
  const GaussianMixture gm{weights, means, stddevs};

  const double stable = as_f64_scalar(gm.logcdf(x));
  const double naive = as_f64_scalar(gm.cdf(x).log());
  REQUIRE(std::isfinite(stable));
  REQUIRE(std::isinf(naive));
  REQUIRE(std::abs(stable - as_f64_scalar(logcdf("normal", x, std::initializer_list<double>{}))) <= 1e-5);
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
